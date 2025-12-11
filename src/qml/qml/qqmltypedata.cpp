// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <private/qqmlcomponentandaliasresolver_p.h>
#include <private/qqmlengine_p.h>
#include <private/qqmlirbuilder_p.h>
#include <private/qqmlirloader_p.h>
#include <private/qqmlpropertycachecreator_p.h>
#include <private/qqmlpropertyvalidator_p.h>
#include <private/qqmlscriptblob_p.h>
#include <private/qqmlscriptdata_p.h>
#include <private/qqmltypecompiler_p.h>
#include <private/qqmltypedata_p.h>
#include <private/qqmltypeloaderqmldircontent_p.h>

#include <QtCore/qloggingcategory.h>
#include <QtCore/qcryptographichash.h>

#include <memory>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcCycle, "qt.qml.typeresolution.cycle", QtWarningMsg)

QString QQmlTypeData::TypeReference::qualifiedName() const
{
    QString result;
    if (!prefix.isEmpty()) {
        result = prefix + QLatin1Char('.');
    }
    result.append(type.qmlTypeName());
    return result;
}

QQmlTypeData::QQmlTypeData(const QUrl &url, QQmlTypeLoader *manager)
    : QQmlNotifyingBlob(url, QmlFile, manager),
      m_typesResolved(false), m_implicitImportLoaded(false)
{

}

QQmlTypeData::~QQmlTypeData()
{
    m_scripts.clear();
    m_compositeSingletons.clear();
    m_resolvedTypes.clear();
}

QV4::CompiledData::CompilationUnit *QQmlTypeData::compilationUnit() const
{
    return m_compiledData.data();
}

QQmlType QQmlTypeData::qmlType(const QString &inlineComponentName) const
{
    if (inlineComponentName.isEmpty())
        return m_qmlType;
    return m_inlineComponentData[inlineComponentName].qmlType;
}

bool QQmlTypeData::tryLoadFromDiskCache()
{
    assertTypeLoaderThread();

    if (!m_backupSourceCode.isCacheable())
        return false;

    if (!m_typeLoader->readCacheFile())
        return false;

    auto unit = QQml::makeRefPointer<QV4::CompiledData::CompilationUnit>();
    {
        QString error;
        if (!unit->loadFromDisk(url(), m_backupSourceCode.sourceTimeStamp(), &error)) {
            qCDebug(DBG_DISK_CACHE) << "Error loading" << urlString() << "from disk cache:" << error;
            return false;
        }
    }

    if (unit->unitData()->flags & QV4::CompiledData::Unit::PendingTypeCompilation) {
        restoreIR(unit);
        return true;
    }

    return loadFromDiskCache(unit);
}

bool QQmlTypeData::loadFromDiskCache(const QQmlRefPointer<QV4::CompiledData::CompilationUnit> &unit)
{
    assertTypeLoaderThread();

    m_compiledData = unit;

    QVector<QV4::CompiledData::InlineComponent> ics;
    for (int i = 0, count = m_compiledData->objectCount(); i < count; ++i) {
        auto object = m_compiledData->objectAt(i);
        m_typeReferences.collectFromObject(object);
        const auto inlineComponentTable = object->inlineComponentTable();
        for (auto i = 0; i != object->nInlineComponents; ++i) {
            ics.push_back(inlineComponentTable[i]);
        }
    }

    m_importCache->setBaseUrl(finalUrl(), finalUrlString());

    // For remote URLs, we don't delay the loading of the implicit import
    // because the loading probably requires an asynchronous fetch of the
    // qmldir (so we can't load it just in time).
    if (!finalUrl().scheme().isEmpty()) {
        QUrl qmldirUrl = finalUrl().resolved(QUrl(QLatin1String("qmldir")));
        if (!QQmlImports::isLocal(qmldirUrl)) {
            if (!loadImplicitImport())
                return false;

            // find the implicit import
            for (quint32 i = 0, count = m_compiledData->importCount(); i < count; ++i) {
                const QV4::CompiledData::Import *import = m_compiledData->importAt(i);
                if (m_compiledData->stringAt(import->uriIndex) == QLatin1String(".")
                    && import->qualifierIndex == 0
                    && !import->version.hasMajorVersion()
                    && !import->version.hasMinorVersion()) {
                    QList<QQmlError> errors;
                    auto pendingImport = std::make_shared<PendingImport>(
                                this, import, QQmlImports::ImportNoFlag);
                    pendingImport->precedence = QQmlImportInstance::Implicit;
                    if (!fetchQmldir(qmldirUrl, std::move(pendingImport), 1, &errors)) {
                        setError(errors);
                        return false;
                    }
                    break;
                }
            }
        }
    }

    for (int i = 0, count = m_compiledData->importCount(); i < count; ++i) {
        const QV4::CompiledData::Import *import = m_compiledData->importAt(i);
        QList<QQmlError> errors;
        if (!addImport(import, {}, &errors)) {
            Q_ASSERT(errors.size());
            QQmlError error(errors.takeFirst());
            error.setUrl(m_importCache->baseUrl());
            error.setLine(qmlConvertSourceCoordinate<quint32, int>(import->location.line()));
            error.setColumn(qmlConvertSourceCoordinate<quint32, int>(import->location.column()));
            errors.prepend(error); // put it back on the list after filling out information.
            setError(errors);
            return false;
        }
    }

    for (auto&& ic: ics) {
        QString const nameString = m_compiledData->stringAt(ic.nameIndex);
        auto importUrl = finalUrl();
        importUrl.setFragment(nameString);
        auto import = new QQmlImportInstance();
        m_importCache->addInlineComponentImport(import, nameString, importUrl);
    }

    return true;
}

template<>
void QQmlComponentAndAliasResolver<QV4::CompiledData::CompilationUnit>::allocateNamedObjects(
        const QV4::CompiledData::Object *object) const
{
    Q_UNUSED(object);
}

template<>
bool QQmlComponentAndAliasResolver<QV4::CompiledData::CompilationUnit>::markAsComponent(int index) const
{
    return m_compiler->objectAt(index)->hasFlag(QV4::CompiledData::Object::IsComponent);
}

template<>
void QQmlComponentAndAliasResolver<QV4::CompiledData::CompilationUnit>::setObjectId(int index) const
{
    Q_UNUSED(index)
    // we cannot sanity-check the index here because bindings are sorted in a different order
    // in the CU vs the IR.
}

template<>
void QQmlComponentAndAliasResolver<QV4::CompiledData::CompilationUnit>::resolveGeneralizedGroupProperty(
        const CompiledObject &component, CompiledBinding *binding)
{
    // We cannot make it fail here. It might be a custom-parsed property
    for (int i = 0, count = component.namedObjectsInComponentCount(); i < count; ++i) {
        const int candidateIndex = component.namedObjectsInComponentTable()[i];
        if (m_compiler->objectAt(candidateIndex)->idNameIndex == binding->propertyNameIndex) {
            m_propertyCaches->set(binding->value.objectIndex, m_propertyCaches->at(candidateIndex));
            return;
        }
    }
}

template<>
typename QQmlComponentAndAliasResolver<QV4::CompiledData::CompilationUnit>::AliasResolutionResult
QQmlComponentAndAliasResolver<QV4::CompiledData::CompilationUnit>::resolveAliasesInObject(
        const CompiledObject &component, int objectIndex,
        QQmlPropertyCacheAliasCreator<QV4::CompiledData::CompilationUnit> *aliasCacheCreator,
        QQmlError *error)
{
    const CompiledObject *obj = m_compiler->objectAt(objectIndex);
    int aliasIndex = 0;
    const auto doAppendAlias = [&](const QV4::CompiledData::Alias *alias, int encodedIndex) {
        return appendAliasToPropertyCache(
                &component, alias, objectIndex, aliasIndex++, encodedIndex, aliasCacheCreator,
                error);
    };

    for (auto alias = obj->aliasesBegin(), end = obj->aliasesEnd(); alias != end; ++alias) {
        if (resolvedAliases.contains(alias)) {
            ++aliasIndex;
            continue;
        }

        if (alias->isAliasToLocalAlias()) {
            if (doAppendAlias(alias, -1))
                continue;
            return SomeAliasesResolved;
        }

        const int targetObjectIndex
                = objectForId(m_compiler, component, alias->targetObjectId());
        const QV4::CompiledData::Object *targetObject = m_compiler->objectAt(targetObjectIndex);

        QStringView property;
        QStringView subProperty;

        const QString aliasPropertyValue = stringAt(alias->propertyNameIndex);
        const int propertySeparator = aliasPropertyValue.indexOf(QLatin1Char('.'));
        if (propertySeparator != -1) {
            property = QStringView{aliasPropertyValue}.left(propertySeparator);
            subProperty = QStringView{aliasPropertyValue}.mid(propertySeparator + 1);
        } else {
            property = QStringView(aliasPropertyValue);
        }

        if (property.isEmpty()) {
            if (doAppendAlias(alias, -1))
                continue;
            return SomeAliasesResolved;
        }

        Q_ASSERT(!property.isEmpty());
        QQmlPropertyCache::ConstPtr targetCache = m_propertyCaches->at(targetObjectIndex);
        Q_ASSERT(targetCache);

        const QQmlPropertyResolver resolver(targetCache);
        const QQmlPropertyData *targetProperty = resolver.property(property.toString());
        if (!targetProperty)
            return SomeAliasesResolved;

        const int coreIndex = targetProperty->coreIndex();
        if (subProperty.isEmpty()) {
            if (doAppendAlias(alias, QQmlPropertyIndex(coreIndex).toEncoded()))
                continue;
            return SomeAliasesResolved;
        }

        if (const QMetaObject *valueTypeMetaObject
                = QQmlMetaType::metaObjectForValueType(targetProperty->propType())) {
            const int valueTypeIndex = valueTypeMetaObject->indexOfProperty(
                    subProperty.toString().toUtf8().constData());
            if (valueTypeIndex == -1)
                return SomeAliasesResolved;

            if (doAppendAlias(alias, QQmlPropertyIndex(coreIndex, valueTypeIndex).toEncoded()))
                continue;

            return SomeAliasesResolved;
        }

        Q_ASSERT(subProperty.at(0).isLower());

        bool isDeepAlias = false;
        for (auto it = targetObject->bindingsBegin(); it != targetObject->bindingsEnd(); ++it) {
            if (m_compiler->stringAt(it->propertyNameIndex) != property)
                continue;

            const QQmlPropertyResolver resolver
                    = QQmlPropertyResolver(m_propertyCaches->at(it->value.objectIndex));
            const QQmlPropertyData *actualProperty = resolver.property(subProperty.toString());
            if (!actualProperty)
                continue;

            if (doAppendAlias(
                        alias, QQmlPropertyIndex(coreIndex, actualProperty->coreIndex()).toEncoded())) {
                isDeepAlias = true;
                break;
            }

            return SomeAliasesResolved;
        }

        if (!isDeepAlias)
            return SomeAliasesResolved;
    }

    return AllAliasesResolved;
}

template<>
bool QQmlComponentAndAliasResolver<QV4::CompiledData::CompilationUnit>::wrapImplicitComponent(
        const QV4::CompiledData::Binding *binding)
{
    // This should have been done when creating the CU.
    Q_UNUSED(binding);
    return false;
}

QQmlError QQmlTypeData::createTypeAndPropertyCaches(
        const QQmlRefPointer<QQmlTypeNameCache> &typeNameCache,
        const QV4::CompiledData::ResolvedTypeReferenceMap &resolvedTypeCache)
{
    assertTypeLoaderThread();

    Q_ASSERT(m_compiledData);
    m_compiledData->typeNameCache = typeNameCache;
    m_compiledData->resolvedTypes = resolvedTypeCache;
    m_compiledData->inlineComponentData = m_inlineComponentData;

    QQmlPendingGroupPropertyBindings pendingGroupPropertyBindings;

    {
        QQmlPropertyCacheCreator<QV4::CompiledData::CompilationUnit> propertyCacheCreator(
                &m_compiledData->propertyCaches, &pendingGroupPropertyBindings, m_typeLoader,
                m_compiledData.data(), m_importCache.data(), typeClassName());

        QQmlError error = propertyCacheCreator.verifyNoICCycle();
        if (error.isValid())
            return error;

        QQmlPropertyCacheCreatorBase::IncrementalResult result;
        do {
            result = propertyCacheCreator.buildMetaObjectsIncrementally();
            if (result.error.isValid()) {
                return result.error;
            } else {
                QQmlComponentAndAliasResolver resolver(
                        m_compiledData.data(), &m_compiledData->propertyCaches);
                if (const QQmlError error = resolver.resolve(result.processedRoot);
                        error.isValid()) {
                    return error;
                }
                pendingGroupPropertyBindings.resolveMissingPropertyCaches(
                        &m_compiledData->propertyCaches);
                pendingGroupPropertyBindings.clear(); // anything that can be processed is now processed
            }

        } while (result.canResume);
    }

    pendingGroupPropertyBindings.resolveMissingPropertyCaches(&m_compiledData->propertyCaches);
    return QQmlError();
}

// local helper function for inline components
namespace  {
using InlineComponentData = QV4::CompiledData::InlineComponentData;

template<typename ObjectContainer>
void setupICs(
        const ObjectContainer &container, QHash<QString, InlineComponentData> *icData,
        const QUrl &baseUrl,
        const QQmlRefPointer<QV4::CompiledData::CompilationUnit> &compilationUnit) {
    Q_ASSERT(icData->empty());
    for (int i = 0; i != container->objectCount(); ++i) {
        auto root = container->objectAt(i);
        for (auto it = root->inlineComponentsBegin(); it != root->inlineComponentsEnd(); ++it) {
            // We cannot re-use a previously finalized inline component type here. We need our own.
            // We can and should re-use speculative type references, though.
            InlineComponentData icDatum(
                    QQmlMetaType::findInlineComponentType(
                            baseUrl, container->stringAt(it->nameIndex), compilationUnit),
                int(it->objectIndex), int(it->nameIndex));

            icData->insert(container->stringAt(it->nameIndex), icDatum);
        }
    }
};
}

bool QQmlTypeData::checkScripts()
{
    // Check all script dependencies for errors
    for (int ii = 0; ii < m_scripts.size(); ++ii) {
        const ScriptReference &script = m_scripts.at(ii);
        Q_ASSERT(script.script->isCompleteOrError());
        if (script.script->isError()) {
            createError(
                    script,
                    QQmlTypeLoader::tr("Script %1 unavailable").arg(script.script->urlString()));
            return false;
        }
    }
    return true;
}

void QQmlTypeData::createError(const TypeReference &type, const QString &message)
{
    createError(type, message, type.typeData ? type.typeData->errors() : QList<QQmlError>());
}

void QQmlTypeData::createError(const ScriptReference &script, const QString &message)
{
    createError(script, message, script.script ? script.script->errors() : QList<QQmlError>());
}

bool QQmlTypeData::checkDependencies()
{
    // Check all type dependencies for errors
    for (auto it = std::as_const(m_resolvedTypes).begin(), end = std::as_const(m_resolvedTypes).end();
         it != end; ++it) {
        const TypeReference &type = *it;
        Q_ASSERT(!type.typeData
                 || type.typeData->isCompleteOrError()
                 || type.type.isInlineComponentType());

        if (type.type.isInlineComponentType()) {
            const QUrl url = type.type.sourceUrl();
            if (!QQmlMetaType::equalBaseUrls(url, finalUrl())
                    && !QQmlMetaType::obtainCompilationUnit(type.type.typeId())) {
                const QString &typeName = stringAt(it.key());
                int lastDot = typeName.lastIndexOf(u'.');
                createError(
                        type,
                        QQmlTypeLoader::tr("Type %1 has no inline component type called %2")
                                .arg(QStringView{typeName}.left(lastDot), type.type.elementName()));
                return false;
            }
        }
        if (type.typeData && type.typeData->isError()) {
            const QString &typeName = stringAt(it.key());
            createError(type, QQmlTypeLoader::tr("Type %1 unavailable").arg(typeName));
            return false;
        }
    }

    return true;
}

bool QQmlTypeData::checkCompositeSingletons()
{
    // Check all composite singleton type dependencies for errors
    for (int ii = 0; ii < m_compositeSingletons.size(); ++ii) {
        const TypeReference &type = m_compositeSingletons.at(ii);
        Q_ASSERT(!type.typeData || type.typeData->isCompleteOrError());
        if (type.typeData && type.typeData->isError()) {
            QString typeName = type.type.qmlTypeName();
            createError(type, QQmlTypeLoader::tr("Type %1 unavailable").arg(typeName));
            return false;
        }
    }

    return true;
}

void QQmlTypeData::createQQmlType()
{
    if (QQmlPropertyCacheCreatorBase::canCreateClassNameTypeByUrl(finalUrl())) {
        const bool isSingleton = m_document
            ? m_document.data()->isSingleton()
            : (m_compiledData->unitData()->flags & QV4::CompiledData::Unit::IsSingleton);
        m_qmlType = QQmlMetaType::findCompositeType(
                url(), m_compiledData, isSingleton
                        ? QQmlMetaType::Singleton
                        : QQmlMetaType::NonSingleton);
        m_typeClassName = QByteArray(m_qmlType.typeId().name()).chopped(1);
    }
}

bool QQmlTypeData::rebuildFromSource()
{
    // Clear and re-build everything.

    m_typeReferences.clear();
    m_scripts.clear();
    m_namespaces.clear();
    m_compositeSingletons.clear();

    m_resolvedTypes.clear();
    m_typesResolved = false;

    m_qmlType = QQmlType();
    m_typeClassName.clear();

    m_inlineComponentData.clear();
    m_compiledData.reset();

    m_implicitImportLoaded = false;

    m_importCache.adopt(new QQmlImports);
    m_unresolvedImports.clear();

    if (!loadFromSource())
        return false;

    continueLoadFromIR();

    if (!resolveTypes())
        return false;

    if (!checkScripts())
        return false;

    if (!checkDependencies())
        return false;

    if (!checkCompositeSingletons())
        return false;

    createQQmlType();

    setupICs(m_document, &m_inlineComponentData, finalUrl(), m_compiledData);
    return true;
}

void QQmlTypeData::done()
{
    assertTypeLoaderThread();

    auto cleanup = qScopeGuard([this]{
        m_backupSourceCode = SourceCodeData();
        m_document.reset();
        m_typeReferences.clear();
        if (isError()) {
            const auto encounteredErrors = errors();
            for (const QQmlError &e : encounteredErrors)
                qCDebug(DBG_DISK_CACHE) << e.toString();
            m_compiledData.reset();
        }
    });

    if (isError())
        return;

    if (!checkScripts())
        return;

    if (!checkDependencies())
        return;

    if (!checkCompositeSingletons())
        return;

    createQQmlType();

    if (m_document)
        setupICs(m_document, &m_inlineComponentData, finalUrl(), m_compiledData);
    else
        setupICs(m_compiledData, &m_inlineComponentData, finalUrl(), m_compiledData);

    QV4::CompiledData::ResolvedTypeReferenceMap resolvedTypeCache;
    QQmlRefPointer<QQmlTypeNameCache> typeNameCache;

    // If we've pulled the CU from the memory cache, we don't need to do any verification.
    const bool verifyCaches = !m_compiledData
            || (m_compiledData->resolvedTypes.isEmpty() && !m_compiledData->typeNameCache);

    if (verifyCaches) {
        QQmlError error = buildTypeResolutionCaches(&typeNameCache, &resolvedTypeCache);
        if (error.isValid()) {
            setError(error);
            qDeleteAll(resolvedTypeCache);
            return;
        }
    }

    const auto dependencyHasher = [&resolvedTypeCache, this]() {
        return typeLoader()->hashDependencies(&resolvedTypeCache, m_compositeSingletons);
    };

    // verify if any dependencies changed if we're using a cache
    if (m_document.isNull() && verifyCaches) {
        const QQmlError error = createTypeAndPropertyCaches(typeNameCache, resolvedTypeCache);
        if (error.isValid() || !m_compiledData->verifyChecksum(dependencyHasher)) {

            if (error.isValid()) {
                qCDebug(DBG_DISK_CACHE)
                        << "Failed to create property caches for"
                        << m_compiledData->fileName()
                        << "because" << error.description();
            } else {
                qCDebug(DBG_DISK_CACHE)
                        << "Checksum mismatch for cached version of"
                        << m_compiledData->fileName();
            }

            resolvedTypeCache.clear();
            typeNameCache.reset();

            if (!rebuildFromSource())
                return;

            const QQmlError error = buildTypeResolutionCaches(&typeNameCache, &resolvedTypeCache);
            if (error.isValid()) {
                setError(error);
                qDeleteAll(resolvedTypeCache);
                return;
            }
        }
    }

    if (!m_document.isNull()) {
        Q_ASSERT(verifyCaches);
        // Compile component
        compile(typeNameCache, &resolvedTypeCache, dependencyHasher);
        if (isError())
            return;
    }

    {
        m_compiledData->inlineComponentData = m_inlineComponentData;
        {
            // Sanity check property bindings
            QQmlPropertyValidator validator(typeLoader(), m_importCache.data(), m_compiledData);
            QVector<QQmlError> errors = validator.validate();
            if (!errors.isEmpty()) {
                setError(errors);
                return;
            }
        }

        m_compiledData->finalizeCompositeType(qmlType());
    }

    {
        QQmlType type = QQmlMetaType::qmlType(finalUrl());
        if (m_compiledData && m_compiledData->unitData()->flags & QV4::CompiledData::Unit::IsSingleton) {
            if (!type.isValid()) {
                QQmlError error;
                error.setDescription(QQmlTypeLoader::tr("No matching type found, pragma Singleton files cannot be used by QQmlComponent."));
                setError(error);
                return;
            } else if (!type.isCompositeSingleton()) {
                QQmlError error;
                error.setDescription(QQmlTypeLoader::tr("pragma Singleton used with a non composite singleton type %1").arg(type.qmlTypeName()));
                setError(error);
                return;
            }
        } else {
            // If the type is CompositeSingleton but there was no pragma Singleton in the
            // QML file, lets report an error.
            if (type.isValid() && type.isCompositeSingleton()) {
                QString typeName = type.qmlTypeName();
                setError(QQmlTypeLoader::tr("qmldir defines type as singleton, but no pragma Singleton found in type %1.").arg(typeName));
                return;
            }
        }
    }

    {
        // Collect imported scripts
        m_compiledData->dependentScripts.reserve(m_scripts.size());
        for (int scriptIndex = 0; scriptIndex < m_scripts.size(); ++scriptIndex) {
            const QQmlTypeData::ScriptReference &script = m_scripts.at(scriptIndex);

            QStringView qualifier(script.qualifier);
            QString enclosingNamespace;

            const int lastDotIndex = qualifier.lastIndexOf(QLatin1Char('.'));
            if (lastDotIndex != -1) {
                enclosingNamespace = qualifier.left(lastDotIndex).toString();
                qualifier = qualifier.mid(lastDotIndex+1);
            }

            m_compiledData->typeNameCache->add(
                    qualifier.toString(), scriptIndex, enclosingNamespace);
            QQmlRefPointer<QQmlScriptData> scriptData = script.script->scriptData();
            m_compiledData->dependentScripts << scriptData;
        }
    }
}

bool QQmlTypeData::loadImplicitImport()
{
    assertTypeLoaderThread();

    m_implicitImportLoaded = true; // Even if we hit an error, count as loaded (we'd just keep hitting the error)

    m_importCache->setBaseUrl(finalUrl(), finalUrlString());

    // For local urls, add an implicit import "." as most overridden lookup.
    // This will also trigger the loading of the qmldir and the import of any native
    // types from available plugins.
    QList<QQmlError> implicitImportErrors;
    QString localQmldir;
    m_importCache->addImplicitImport(typeLoader(), &localQmldir, &implicitImportErrors);

    // When loading with QQmlImports::ImportImplicit, the imports are _appended_ to the namespace
    // in the order they are loaded. Therefore, the addImplicitImport above gets the highest
    // precedence. This is in contrast to normal priority imports. Those are _prepended_ in the
    // order they are loaded.
    if (!localQmldir.isEmpty()) {
        const QQmlTypeLoaderQmldirContent qmldir = typeLoader()->qmldirContent(localQmldir);
        const QList<QQmlDirParser::Import> moduleImports
                = QQmlMetaType::moduleImports(qmldir.typeNamespace(), QTypeRevision())
                + qmldir.imports();
        loadDependentImports(moduleImports, QString(), QTypeRevision(),
                             QQmlImportInstance::Implicit + 1, QQmlImports::ImportNoFlag,
                             &implicitImportErrors);
    }

    if (!implicitImportErrors.isEmpty()) {
        setError(implicitImportErrors);
        return false;
    }

    return true;
}

void QQmlTypeData::dataReceived(const SourceCodeData &data)
{
    assertTypeLoaderThread();

    m_backupSourceCode = data;

    if (tryLoadFromDiskCache())
        return;

    if (isError())
        return;

    if (!m_backupSourceCode.exists() || m_backupSourceCode.isEmpty()) {
        if (m_cachedUnitStatus == QQmlMetaType::CachedUnitLookupError::VersionMismatch)
            setError(QQmlTypeLoader::tr("File was compiled ahead of time with an incompatible version of Qt and the original file cannot be found. Please recompile"));
        else if (!m_backupSourceCode.exists())
            setError(QQmlTypeLoader::tr("No such file or directory"));
        else
            setError(QQmlTypeLoader::tr("File is empty"));
        return;
    }

    if (!loadFromSource())
        return;

    continueLoadFromIR();
}

void QQmlTypeData::initializeFromCachedUnit(const QQmlPrivate::CachedQmlUnit *unit)
{
    assertTypeLoaderThread();

    if (unit->qmlData->qmlUnit()->nObjects == 0) {
        setError(QQmlTypeLoader::tr("Cached QML Unit has no objects"));
        return;
    }

    m_document.reset(
            new QmlIR::Document(urlString(), finalUrlString(), m_typeLoader->isDebugging()));
    QQmlIRLoader loader(unit->qmlData, m_document.data());
    loader.load();
    m_document->javaScriptCompilationUnit
            = QQmlRefPointer<QV4::CompiledData::CompilationUnit>(
                new QV4::CompiledData::CompilationUnit(unit->qmlData, unit->aotCompiledFunctions),
                QQmlRefPointer<QV4::CompiledData::CompilationUnit>::Adopt);
    continueLoadFromIR();
}

bool QQmlTypeData::loadFromSource()
{
    assertTypeLoaderThread();

    m_document.reset(
            new QmlIR::Document(urlString(), finalUrlString(), m_typeLoader->isDebugging()));
    m_document->jsModule.sourceTimeStamp = m_backupSourceCode.sourceTimeStamp();
    QmlIR::IRBuilder compiler;

    QString sourceError;
    const QString source = m_backupSourceCode.readAll(&sourceError);
    if (!sourceError.isEmpty()) {
        setError(sourceError);
        return false;
    }

    if (!compiler.generateFromQml(source, finalUrlString(), m_document.data())) {
        QList<QQmlError> errors;
        errors.reserve(compiler.errors.size());
        for (const QQmlJS::DiagnosticMessage &msg : std::as_const(compiler.errors)) {
            QQmlError e;
            e.setUrl(url());
            e.setLine(qmlConvertSourceCoordinate<quint32, int>(msg.loc.startLine));
            e.setColumn(qmlConvertSourceCoordinate<quint32, int>(msg.loc.startColumn));
            e.setDescription(msg.message);
            errors << e;
        }
        setError(errors);
        return false;
    }
    return true;
}

void QQmlTypeData::restoreIR(const QQmlRefPointer<QV4::CompiledData::CompilationUnit> &unit)
{
    assertTypeLoaderThread();

    m_document.reset(
            new QmlIR::Document(urlString(), finalUrlString(), m_typeLoader->isDebugging()));
    QQmlIRLoader loader(unit->unitData(), m_document.data());
    loader.load();
    m_document->javaScriptCompilationUnit = unit;
    continueLoadFromIR();
}

void QQmlTypeData::continueLoadFromIR()
{
    assertTypeLoaderThread();

    for (auto const& object: m_document->objects) {
        for (auto it = object->inlineComponentsBegin(); it != object->inlineComponentsEnd(); ++it) {
            QString const nameString = m_document->stringAt(it->nameIndex);
            auto importUrl = finalUrl();
            importUrl.setFragment(nameString);
            auto import = new QQmlImportInstance(); // Note: The cache takes ownership of the QQmlImportInstance
            m_importCache->addInlineComponentImport(import, nameString, importUrl);
        }
    }

    m_typeReferences.collectFromObjects(m_document->objects.constBegin(), m_document->objects.constEnd());
    m_importCache->setBaseUrl(finalUrl(), finalUrlString());

    // For remote URLs, we don't delay the loading of the implicit import
    // because the loading probably requires an asynchronous fetch of the
    // qmldir (so we can't load it just in time).
    if (!finalUrl().scheme().isEmpty()) {
        QUrl qmldirUrl = finalUrl().resolved(QUrl(QLatin1String("qmldir")));
        if (!QQmlImports::isLocal(qmldirUrl)) {
            if (!loadImplicitImport())
                return;
            // This qmldir is for the implicit import
            auto implicitImport = std::make_shared<PendingImport>();
            implicitImport->uri = QLatin1String(".");
            implicitImport->version = QTypeRevision();
            QList<QQmlError> errors;

            if (!fetchQmldir(qmldirUrl, implicitImport, 1, &errors)) {
                setError(errors);
                return;
            }
        }
    }

    QList<QQmlError> errors;

    for (const QV4::CompiledData::Import *import : std::as_const(m_document->imports)) {
        if (!addImport(import, {}, &errors)) {
            Q_ASSERT(errors.size());

            // We're only interested in the chronoligically last error. The previous
            // errors might be from unsuccessfully trying to load a module from the
            // resource file system.
            QQmlError error = errors.first();
            error.setUrl(m_importCache->baseUrl());
            error.setLine(qmlConvertSourceCoordinate<quint32, int>(import->location.line()));
            error.setColumn(qmlConvertSourceCoordinate<quint32, int>(import->location.column()));
            setError(error);
            return;
        }
    }
}

void QQmlTypeData::allDependenciesDone()
{
    assertTypeLoaderThread();

    QQmlTypeLoader::Blob::allDependenciesDone();

    if (!m_typesResolved)
        resolveTypes();
}

QString QQmlTypeData::stringAt(int index) const
{
    if (m_compiledData)
        return m_compiledData->stringAt(index);
    return m_document->jsGenerator.stringTable.stringForIndex(index);
}

void QQmlTypeData::compile(const QQmlRefPointer<QQmlTypeNameCache> &typeNameCache,
                           QV4::CompiledData::ResolvedTypeReferenceMap *resolvedTypeCache,
                           const QV4::CompiledData::DependentTypesHasher &dependencyHasher)
{
    assertTypeLoaderThread();

    Q_ASSERT(m_compiledData.isNull());

    const bool typeRecompilation = m_document
            && m_document->javaScriptCompilationUnit
            && m_document->javaScriptCompilationUnit->unitData()
            && (m_document->javaScriptCompilationUnit->unitData()->flags
                & QV4::CompiledData::Unit::PendingTypeCompilation);

    QQmlTypeCompiler compiler(
            typeLoader(), this, m_document.data(), resolvedTypeCache, dependencyHasher);
    auto compilationUnit = compiler.compile();
    if (!compilationUnit) {
        qDeleteAll(*resolvedTypeCache);
        resolvedTypeCache->clear();
        setError(compiler.compilationErrors());
        return;
    }

    const bool trySaveToDisk = m_typeLoader->writeCacheFile() && !typeRecompilation;
    if (trySaveToDisk) {
        QString errorString;
        if (compilationUnit->saveToDisk(url(), &errorString)) {
            QString error;
            if (!compilationUnit->loadFromDisk(url(), m_backupSourceCode.sourceTimeStamp(), &error)) {
                // ignore error, keep using the in-memory compilation unit.
            }
        } else {
            qCDebug(DBG_DISK_CACHE) << "Error saving cached version of"
                                    << compilationUnit->fileName() << "to disk:" << errorString;
        }
    }

    m_compiledData = std::move(compilationUnit);
    m_compiledData->typeNameCache = typeNameCache;
    m_compiledData->resolvedTypes = *resolvedTypeCache;
    m_compiledData->propertyCaches = std::move(*compiler.propertyCaches());
    Q_ASSERT(m_compiledData->propertyCaches.count()
             == static_cast<int>(m_compiledData->objectCount()));
}

bool QQmlTypeData::resolveTypes()
{
    assertTypeLoaderThread();

    Q_ASSERT(!m_typesResolved);

    // Check that all imports were resolved
    QList<QQmlError> errors;
    auto it = m_unresolvedImports.constBegin(), end = m_unresolvedImports.constEnd();
    for ( ; it != end; ++it) {
        const PendingImportPtr &import = *it;
        if (import->priority != 0)
            continue;

        // If the import was potentially remote and all the network requests have failed,
        // we now know that there is no qmldir. We can register its types.
        if (registerPendingTypes(import))
            continue;

        // This import was not resolved
        QQmlError error;
        error.setDescription(QQmlTypeLoader::tr("module \"%1\" is not installed").arg(import->uri));
        error.setUrl(m_importCache->baseUrl());
        error.setLine(qmlConvertSourceCoordinate<quint32, int>(
                import->location.line()));
        error.setColumn(qmlConvertSourceCoordinate<quint32, int>(
                import->location.column()));
        errors.prepend(error);
    }

    if (errors.size()) {
        setError(errors);
        return false;
    }

    // Load the implicit import since it may have additional scripts.
    if (!m_implicitImportLoaded && !loadImplicitImport())
        return false;

    // Add any imported scripts to our resolved set
    const auto resolvedScripts = m_importCache->resolvedScripts();
    for (const QQmlImports::ScriptReference &script : resolvedScripts) {
        QQmlRefPointer<QQmlScriptBlob> blob
                = typeLoader()->getScript(script.location, script.fileName);
        addDependency(blob.data());

        ScriptReference ref;
        //ref.location = ...
        if (!script.qualifier.isEmpty())
        {
            ref.qualifier = script.qualifier + QLatin1Char('.') + script.nameSpace;
            // Add a reference to the enclosing namespace
            m_namespaces.insert(script.qualifier);
        } else {
            ref.qualifier = script.nameSpace;
        }

        ref.script = blob;
        m_scripts << ref;
    }

    // Lets handle resolved composite singleton types
    const auto resolvedCompositeSingletons = m_importCache->resolvedCompositeSingletons();
    for (const QQmlImports::CompositeSingletonReference &csRef : resolvedCompositeSingletons) {
        TypeReference ref;
        QString typeName;
        if (!csRef.prefix.isEmpty()) {
            typeName = csRef.prefix + QLatin1Char('.') + csRef.typeName;
            // Add a reference to the enclosing namespace
            m_namespaces.insert(csRef.prefix);
        } else {
            typeName = csRef.typeName;
        }

        QTypeRevision version = csRef.version;
        if (!resolveType(typeName, version, ref, -1, -1, true, QQmlType::CompositeSingletonType))
            return false;

        if (ref.type.isCompositeSingleton()) {
            ref.typeData = typeLoader()->getType(ref.type.sourceUrl());
            if (ref.typeData->isWaiting() || m_waitingOnMe.contains(ref.typeData.data())) {
                qCDebug(lcCycle) << "Possible cyclic dependency detected between"
                                 << ref.typeData->urlString() << "and" << urlString();
                continue;
            }
            addDependency(ref.typeData.data());
            ref.prefix = csRef.prefix;

            m_compositeSingletons << ref;
        }
    }

    for (auto unresolvedRef = m_typeReferences.constBegin(), end = m_typeReferences.constEnd();
         unresolvedRef != end; ++unresolvedRef) {

        TypeReference ref; // resolved reference

        const bool reportErrors = unresolvedRef->errorWhenNotFound;

        QTypeRevision version;

        const QString name = stringAt(unresolvedRef.key());

        bool *selfReferenceDetection = unresolvedRef->needsCreation ? nullptr : &ref.selfReference;

        if (!resolveType(name, version, ref, unresolvedRef->location.line(),
                         unresolvedRef->location.column(), reportErrors,
                         QQmlType::AnyRegistrationType, selfReferenceDetection) && reportErrors)
            return false;

        if (ref.type.isComposite() && !ref.selfReference) {
            ref.typeData = typeLoader()->getType(ref.type.sourceUrl());
            addDependency(ref.typeData.data());
        }
        if (ref.type.isInlineComponentType()) {
            QUrl containingTypeUrl = ref.type.sourceUrl();
            if (!containingTypeUrl.isEmpty()
                    && !QQmlMetaType::equalBaseUrls(finalUrl(), containingTypeUrl)) {
                containingTypeUrl.setFragment(QString());
                auto typeData = typeLoader()->getType(containingTypeUrl);
                if (typeData.data() != this) {
                    ref.typeData = typeData;
                    addDependency(typeData.data());
                }
            }
        }

        ref.version = version;
        ref.location = unresolvedRef->location;
        ref.needsCreation = unresolvedRef->needsCreation;
        m_resolvedTypes.insert(unresolvedRef.key(), ref);
    }

    m_typesResolved = true;
    return true;
}

QQmlError QQmlTypeData::buildTypeResolutionCaches(
        QQmlRefPointer<QQmlTypeNameCache> *typeNameCache,
        QV4::CompiledData::ResolvedTypeReferenceMap *resolvedTypeCache) const
{
    assertTypeLoaderThread();

    typeNameCache->adopt(new QQmlTypeNameCache(m_importCache));

    for (const QString &ns: m_namespaces)
        (*typeNameCache)->add(ns);

    // Add any Composite Singletons that were used to the import cache
    for (const QQmlTypeData::TypeReference &singleton: m_compositeSingletons)
        (*typeNameCache)->add(singleton.type.qmlTypeName(), singleton.type.sourceUrl(), singleton.prefix);

    m_importCache->populateCache(typeNameCache->data());

    for (auto resolvedType = m_resolvedTypes.constBegin(), end = m_resolvedTypes.constEnd(); resolvedType != end; ++resolvedType) {
        auto ref = std::make_unique<QV4::ResolvedTypeReference>();
        QQmlType qmlType = resolvedType->type;
        ref->setType(qmlType);
        ref->setIsSelfReference(resolvedType->selfReference);
        if (resolvedType->typeData) {
            if (resolvedType->needsCreation && qmlType.isCompositeSingleton()) {
                return qQmlCompileError(resolvedType->location, tr("Composite Singleton Type %1 is not creatable.").arg(qmlType.qmlTypeName()));
            }
            const auto compilationUnit = resolvedType->typeData->compilationUnit();
            if (qmlType.isInlineComponentType()) {
                // Inline component which is part of an already resolved type
                QString icName = qmlType.elementName();
                Q_ASSERT(!icName.isEmpty());

                ref->setTypePropertyCache(compilationUnit->propertyCaches.at(
                    compilationUnit->inlineComponentId(icName)));
                Q_ASSERT(ref->type().isInlineComponentType());
            } else {
                ref->setTypePropertyCache(compilationUnit->rootPropertyCache());
            }
            if (!resolvedType->selfReference && resolvedType->needsCreation)
                ref->setCompilationUnit(compilationUnit);
        } else if (qmlType.isInlineComponentType()) {
            // Inline component
            // If it's defined in the same file we're currently compiling, we don't want to use it.
            // We're going to fill in the property caches later after all.
            if (QQmlMetaType::equalBaseUrls(finalUrl(), qmlType.sourceUrl())) {
                ref->setIsSelfReference(true);
            } else {
                // this is required for inline components in singletons
                const QMetaType type = qmlType.typeId();
                ref->setTypePropertyCache(QQmlMetaType::propertyCacheForType(type));
                if (resolvedType->needsCreation) {
                    // TODO: Obtaining a compilation unit from the type registry is OK-ish
                    //       here because the type is unique to the engine. What we actually
                    //       want is to have a nullptr CU if the respective inline component
                    //       doesn't exist. There should be a simpler way to do this.
                    ref->setCompilationUnit(QQmlMetaType::obtainCompilationUnit(type));
                }
            }
        } else if (qmlType.isValid() && !resolvedType->selfReference) {
            Q_ASSERT(ref->type().isValid());

            if (resolvedType->needsCreation && !qmlType.isCreatable()) {
                QString reason = qmlType.noCreationReason();
                if (reason.isEmpty())
                    reason = tr("Element is not creatable.");
                return qQmlCompileError(resolvedType->location, reason);
            }

            if (qmlType.containsRevisionedAttributes()) {
                // It can only have (revisioned) properties or methods if it has a metaobject
                Q_ASSERT(qmlType.metaObject());
                ref->setTypePropertyCache(
                    QQmlMetaType::propertyCache(qmlType, resolvedType->version));
            }
        }
        ref->setVersion(resolvedType->version);
        ref->doDynamicTypeCheck();
        resolvedTypeCache->insert(resolvedType.key(), ref.release());
    }
    QQmlError noError;
    return noError;
}

bool QQmlTypeData::resolveType(const QString &typeName, QTypeRevision &version,
                               TypeReference &ref, int lineNumber, int columnNumber,
                               bool reportErrors, QQmlType::RegistrationType registrationType,
                               bool *typeRecursionDetected)
{
    assertTypeLoaderThread();

    QQmlImportNamespace *typeNamespace = nullptr;
    QList<QQmlError> errors;

    bool typeFound = m_importCache->resolveType(
            typeLoader(), typeName, &ref.type, &version, &typeNamespace, &errors, registrationType,
            typeRecursionDetected);
    if (!typeNamespace && !typeFound && !m_implicitImportLoaded) {
        // Lazy loading of implicit import
        if (loadImplicitImport()) {
            // Try again to find the type
            errors.clear();
            typeFound = m_importCache->resolveType(
                    typeLoader(), typeName, &ref.type, &version, &typeNamespace, &errors,
                    registrationType, typeRecursionDetected);
        } else {
            return false; //loadImplicitImport() hit an error, and called setError already
        }
    }

    if ((!typeFound || typeNamespace) && reportErrors) {
        // Known to not be a type:
        //  - known to be a namespace (Namespace {})
        //  - type with unknown namespace (UnknownNamespace.SomeType {})
        QQmlError error;
        if (typeNamespace) {
            error.setDescription(QQmlTypeLoader::tr("Namespace %1 cannot be used as a type").arg(typeName));
        } else {
            if (errors.size()) {
                error = errors.takeFirst();
            } else {
                // this should not be possible!
                // Description should come from error provided by addImport() function.
                error.setDescription(QQmlTypeLoader::tr("Unreported error adding script import to import database"));
            }
            error.setUrl(m_importCache->baseUrl());
            error.setDescription(QQmlTypeLoader::tr("%1 %2").arg(typeName).arg(error.description()));
        }

        if (lineNumber != -1)
            error.setLine(lineNumber);
        if (columnNumber != -1)
            error.setColumn(columnNumber);

        errors.prepend(error);
        setError(errors);
        return false;
    }

    return true;
}

void QQmlTypeData::scriptImported(
        const QQmlRefPointer<QQmlScriptBlob> &blob, const QV4::CompiledData::Location &location,
        const QString &nameSpace, const QString &qualifier)
{
    assertTypeLoaderThread();

    ScriptReference ref;
    ref.script = blob;
    ref.location = location;
    ref.qualifier = qualifier.isEmpty() ? nameSpace : qualifier + QLatin1Char('.') + nameSpace;

    m_scripts << ref;
}

QT_END_NAMESPACE
