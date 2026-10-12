// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#include "qqmltypecompiler_p.h"

#include <private/qqmlobjectcreator_p.h>
#include <private/qqmlcustomparser_p.h>
#include <private/qqmlvmemetaobject_p.h>
#include <private/qqmlcomponent_p.h>
#include <private/qqmlpropertyresolver_p.h>
#include <private/qqmlcomponentandaliasresolver_p.h>
#include <private/qqmlsignalnames_p.h>

#include <QtCore/qtyperevision.h>

#define COMPILE_EXCEPTION(token, desc) \
    { \
        recordError((token)->location, desc); \
        return false; \
    }

QT_BEGIN_NAMESPACE

DEFINE_BOOL_CONFIG_OPTION(
        disableInternalDeferredProperties, QML_DISABLE_INTERNAL_DEFERRED_PROPERTIES);

Q_LOGGING_CATEGORY(lcQmlTypeCompiler, "qt.qml.typecompiler");

QQmlTypeCompiler::QQmlTypeCompiler(
        QQmlTypeLoader *typeLoader, QQmlTypeData *typeData, QmlIR::Document *parsedQML,
        QV4::CompiledData::ResolvedTypeReferenceMap *resolvedTypeCache,
        const QV4::CompiledData::DependentTypesHasher &dependencyHasher)
    : resolvedTypes(resolvedTypeCache)
    , loader(typeLoader)
    , dependencyHasher(dependencyHasher)
    , document(parsedQML)
    , typeData(typeData)
{
}

QQmlRefPointer<QV4::CompiledData::CompilationUnit> QQmlTypeCompiler::compile()
{
    // Build property caches and VME meta object data

    for (auto it = resolvedTypes->constBegin(), end = resolvedTypes->constEnd();
         it != end; ++it) {
        QQmlCustomParser *customParser = (*it)->type().customParser();
        if (customParser)
            customParsers.insert(it.key(), customParser);
    }

    QQmlPendingGroupedPropertyBindings pendingGroupedPropertyBindings;


    {
        QQmlPropertyCacheCreator<QQmlTypeCompiler> propertyCacheBuilder(
                &m_propertyCaches, &pendingGroupedPropertyBindings,
                loader, this, imports(), typeData->typeClassName());
        QQmlError cycleError = propertyCacheBuilder.verifyNoICCycle();
        if (cycleError.isValid()) {
            recordError(cycleError);
            return nullptr;
        }
        QQmlPropertyCacheCreatorBase::IncrementalResult result;
        do {
            result = propertyCacheBuilder.buildMetaObjectsIncrementally();
            const QQmlError &error = result.error;
            if (error.isValid()) {
                recordError(error);
                return nullptr;
            } else {
                // Resolve component boundaries and aliases

                QQmlComponentAndAliasResolver resolver(this, &m_propertyCaches);
                if (QQmlError error = resolver.resolve(result.processedRoot); error.isValid()) {
                    recordError(error);
                    return nullptr;
                }
                pendingGroupedPropertyBindings.resolveMissingPropertyCaches(&m_propertyCaches);
                pendingGroupedPropertyBindings.clear(); // anything that can be processed is now processed
            }
        } while (result.canResume);
    }

    {
        QQmlCustomParserScriptIndexer cpi(this);
        cpi.annotateBindingsWithScriptStrings();
    }

    if (!document->javaScriptCompilationUnit || !document->javaScriptCompilationUnit->unitData()) {
        // Compile JS binding expressions and signal handlers if necessary
        {
            // We can compile script strings ahead of time, but they must be compiled
            // without type optimizations as their scope is always entirely dynamic.
            QQmlScriptStringScanner sss(this);
            sss.scan();
        }

        Q_ASSERT(document->jsModule.fileName == typeData->urlString());
        Q_ASSERT(document->jsModule.finalUrl == typeData->finalUrlString());
        QmlIR::JSCodeGen v4CodeGenerator(document);
        for (QmlIR::Object *object : std::as_const(document->objects)) {
            if (!v4CodeGenerator.generateRuntimeFunctions(object)) {
                Q_ASSERT(v4CodeGenerator.hasError());
                recordError(v4CodeGenerator.error());
                return nullptr;
            }
        }
        document->javaScriptCompilationUnit = v4CodeGenerator.generateCompilationUnit(/*generated unit data*/false);
    }

    // Generate QML compiled type data structures

    QmlIR::QmlUnitGenerator qmlGenerator;
    qmlGenerator.generate(*document, dependencyHasher);

    if (!errors.isEmpty())
        return nullptr;

    return std::move(document->javaScriptCompilationUnit);
}

void QQmlTypeCompiler::recordError(const QV4::CompiledData::Location &location, const QString &description)
{
    QQmlError error;
    error.setLine(qmlConvertSourceCoordinate<quint32, int>(location.line()));
    error.setColumn(qmlConvertSourceCoordinate<quint32, int>(location.column()));
    error.setDescription(description);
    error.setUrl(url());
    errors << error;
}

void QQmlTypeCompiler::recordError(const QQmlJS::DiagnosticMessage &message)
{
    QQmlError error;
    error.setDescription(message.message);
    error.setLine(qmlConvertSourceCoordinate<quint32, int>(message.loc.startLine));
    error.setColumn(qmlConvertSourceCoordinate<quint32, int>(message.loc.startColumn));
    error.setUrl(url());
    errors << error;
}

void QQmlTypeCompiler::recordError(const QQmlError &e)
{
    QQmlError error = e;
    error.setUrl(url());
    errors << error;
}

QString QQmlTypeCompiler::stringAt(int idx) const
{
    return document->stringAt(idx);
}

int QQmlTypeCompiler::registerString(const QString &str)
{
    return document->jsGenerator.registerString(str);
}

int QQmlTypeCompiler::resolvedIndex(int index) const
{
    if (index < objectCount())
        return index;
    const auto *p = m_propertyCaches.at(index)->defaultProperty();
    Q_ASSERT(p && p->isComponentWrapper());
    return p->wrappedObjectIndex();
}

int QQmlTypeCompiler::implicitComponentForObject(int childIndex) const
{
    for (int i = objectCount(), end = m_propertyCaches.count(); i < end; ++i) {
        const auto *p = m_propertyCaches.at(i)->defaultProperty();
        Q_ASSERT(p && p->isComponentWrapper());
        if (p->wrappedObjectIndex() == childIndex)
            return i;
    }
    return -1;
}

const QV4::CompiledData::Unit *QQmlTypeCompiler::qmlUnit() const
{
    return document->javaScriptCompilationUnit->unitData();
}

const QQmlImports *QQmlTypeCompiler::imports() const
{
    return typeData->imports();
}

QList<QmlIR::Object *> *QQmlTypeCompiler::qmlObjects() const
{
    return &document->objects;
}

QQmlPropertyCacheVector *QQmlTypeCompiler::propertyCaches()
{
    return &m_propertyCaches;
}

const QQmlPropertyCacheVector *QQmlTypeCompiler::propertyCaches() const
{
    return &m_propertyCaches;
}

QQmlJS::MemoryPool *QQmlTypeCompiler::memoryPool()
{
    return document->jsParserEngine.pool();
}

QStringView QQmlTypeCompiler::newStringRef(const QString &string)
{
    return document->jsParserEngine.newStringRef(string);
}

const QV4::Compiler::StringTableGenerator *QQmlTypeCompiler::stringPool() const
{
    return &document->jsGenerator.stringTable;
}

QString QQmlTypeCompiler::bindingAsString(const QmlIR::Object *object, int scriptIndex) const
{
    return object->bindingAsString(document, scriptIndex);
}

void QQmlTypeCompiler::addImport(const QString &module, const QString &qualifier, QTypeRevision version)
{
    const quint32 moduleIdx = registerString(module);
    const quint32 qualifierIdx = registerString(qualifier);

    for (int i = 0, count = document->imports.size(); i < count; ++i) {
        const QV4::CompiledData::Import *existingImport = document->imports.at(i);
        if (existingImport->type == QV4::CompiledData::Import::ImportLibrary
            && existingImport->uriIndex == moduleIdx
            && existingImport->qualifierIndex == qualifierIdx)
            return;
    }
    auto pool = memoryPool();
    QV4::CompiledData::Import *import = pool->New<QV4::CompiledData::Import>();
    import->type = QV4::CompiledData::Import::ImportLibrary;
    import->version = version;
    import->uriIndex = moduleIdx;
    import->qualifierIndex = qualifierIdx;
    document->imports.append(import);
}

QQmlType QQmlTypeCompiler::qmlTypeForComponent(const QString &inlineComponentName) const
{
    return typeData->qmlType(inlineComponentName);
}

QQmlCompilePass::QQmlCompilePass(QQmlTypeCompiler *typeCompiler)
    : compiler(typeCompiler)
{
}

QQmlCustomParserScriptIndexer::QQmlCustomParserScriptIndexer(QQmlTypeCompiler *typeCompiler)
    : QQmlCompilePass(typeCompiler)
    , qmlObjects(*typeCompiler->qmlObjects())
    , customParsers(typeCompiler->customParserCache())
{
}

void QQmlCustomParserScriptIndexer::annotateBindingsWithScriptStrings()
{
    scanObjectRecursively(/*root object*/0);
    for (int i = 0; i < qmlObjects.size(); ++i)
        if (qmlObjects.at(i)->flags & QV4::CompiledData::Object::IsInlineComponentRoot)
            scanObjectRecursively(i);
}

void QQmlCustomParserScriptIndexer::scanObjectRecursively(int objectIndex, bool annotateScriptBindings)
{
    const QmlIR::Object * const obj = qmlObjects.at(objectIndex);
    if (!annotateScriptBindings)
        annotateScriptBindings = customParsers.contains(obj->inheritedTypeNameIndex);
    for (QmlIR::Binding *binding = obj->firstBinding(); binding; binding = binding->next) {
        switch (binding->type()) {
        case QV4::CompiledData::Binding::Type_Script:
            if (annotateScriptBindings) {
                binding->stringIndex = compiler->registerString(
                        compiler->bindingAsString(obj, binding->value.compiledScriptIndex));
            }
            break;
        case QV4::CompiledData::Binding::Type_Object:
        case QV4::CompiledData::Binding::Type_AttachedProperty:
        case QV4::CompiledData::Binding::Type_GroupedProperty:
            scanObjectRecursively(binding->value.objectIndex, annotateScriptBindings);
            break;
        default:
            break;
        }
    }
}

QQmlScriptStringScanner::QQmlScriptStringScanner(QQmlTypeCompiler *typeCompiler)
    : QQmlCompilePass(typeCompiler)
    , qmlObjects(*typeCompiler->qmlObjects())
    , propertyCaches(typeCompiler->propertyCaches())
{

}

void QQmlScriptStringScanner::scan()
{
    const QMetaType scriptStringMetaType = QMetaType::fromType<QQmlScriptString>();
    for (int i = 0; i < qmlObjects.size(); ++i) {
        QQmlPropertyCache::ConstPtr propertyCache = propertyCaches->at(i);
        if (!propertyCache)
            continue;

        const QmlIR::Object *obj = qmlObjects.at(i);

        QQmlPropertyResolver resolver(propertyCache);
        const QQmlPropertyData *defaultProperty = obj->indexOfDefaultPropertyOrAlias != -1 ? propertyCache->parent()->defaultProperty() : propertyCache->defaultProperty();

        for (QmlIR::Binding *binding = obj->firstBinding(); binding; binding = binding->next) {
            if (binding->type() != QV4::CompiledData::Binding::Type_Script)
                continue;
            bool notInRevision = false;
            const QQmlPropertyData *pd = binding->propertyNameIndex != quint32(0) ? resolver.property(stringAt(binding->propertyNameIndex), &notInRevision) : defaultProperty;
            if (!pd || pd->propType() != scriptStringMetaType)
                continue;

            QString script = compiler->bindingAsString(obj, binding->value.compiledScriptIndex);
            binding->stringIndex = compiler->registerString(script);
        }
    }
}

template<>
void QQmlComponentAndAliasResolver<QQmlTypeCompiler>::allocateNamedObjects(
        QmlIR::Object *object) const
{
    object->namedObjectsInComponent.allocate(m_compiler->memoryPool(), m_idToObjectIndex);
}

template<>
bool QQmlComponentAndAliasResolver<QQmlTypeCompiler>::markAsComponent(int index) const
{
    m_compiler->qmlObjects()->at(index)->flags |= QV4::CompiledData::Object::IsComponent;
    return true;
}

template<>
void QQmlComponentAndAliasResolver<QQmlTypeCompiler>::setObjectId(int index) const
{
    m_compiler->qmlObjects()->at(index)->id = m_idToObjectIndex.size();
}

template<>
void QQmlComponentAndAliasResolver<QQmlTypeCompiler>::resolveGeneralizedGroupedProperty(
        const CompiledObject &component, CompiledBinding *binding)
{
    Q_UNUSED(component);
    // We cannot make it fail here. It might be a custom-parsed property
    const int targetObjectIndex = m_idToObjectIndex.value(binding->propertyNameIndex, -1);
    if (targetObjectIndex != -1)
        m_propertyCaches->set(binding->value.objectIndex, m_propertyCaches->at(targetObjectIndex));
}

/*!
    \internal

    Attempts to resolve a "deep alias" — an alias whose sub-property path
    goes through a QObject property, an inline component binding, or another
    alias. For example: \c{alias foo: target.groupedProp.innerProp}

    First searches the target object's bindings and aliases for \a property,
    then looks up \a subProperty on the bound/aliased object's property cache.
    If no binding or alias matches, falls back to looking up \a subProperty
    on the declared type's property cache.

    On success, updates \a propIdx with the resolved value-type index and
    returns \c true.
*/
static bool resolveDeepAlias(
        QQmlTypeCompiler *compiler, const QmlIR::Object *targetObject,
        QStringView property, QStringView subProperty,
        QQmlPropertyIndex &propIdx, QMetaType targetPropertyType,
        const QQmlPropertyCacheVector *propertyCaches,
        const QMap<int, int> &idToObjectIndex)
{
    for (auto it = targetObject->bindingsBegin(), end = targetObject->bindingsEnd();
            it != end; ++it) {
        auto binding = *it;
        if (compiler->stringAt(binding.propertyNameIndex) != property)
            continue;
        const auto &cache = propertyCaches->at(binding.value.objectIndex);
        if (!cache)
            continue;
        QQmlPropertyResolver resolver(cache);
        const QQmlPropertyData *pd = resolver.property(subProperty.toString());
        if (!pd)
            continue;
        propIdx = QQmlPropertyIndex(propIdx.coreIndex(), pd->coreIndex());
        return true;
    }

    for (auto it = targetObject->aliasesBegin(), end = targetObject->aliasesEnd();
            it != end; ++it) {
        auto innerAlias = *it;
        if (compiler->stringAt(innerAlias.nameIndex()) != property)
            continue;
        const int innerObjectIndex = idToObjectIndex.value(innerAlias.idIndex(), -1);
        if (innerObjectIndex == -1)
            continue;
        const auto &cache = propertyCaches->at(innerObjectIndex);
        if (!cache)
            continue;
        QQmlPropertyResolver resolver(cache);
        const QQmlPropertyData *pd = resolver.property(subProperty.toString());
        if (!pd)
            continue;
        propIdx = QQmlPropertyIndex(propIdx.coreIndex(), pd->coreIndex());
        return true;
    }

    const QQmlPropertyCache::ConstPtr typeCache
            = QQmlMetaType::propertyCacheForType(targetPropertyType);
    if (typeCache) {
        const QQmlPropertyResolver resolver(typeCache);
        const QQmlPropertyData *pd = resolver.property(subProperty.toString());
        if (pd) {
            propIdx = QQmlPropertyIndex(propIdx.coreIndex(), pd->coreIndex());
            return true;
        }
    }

    return false;
}

template<>
typename QQmlComponentAndAliasResolver<QQmlTypeCompiler>::AliasResolutionResult
QQmlComponentAndAliasResolver<QQmlTypeCompiler>::resolveAliasesInObject(
        const CompiledObject &component, int objectIndex,
        QQmlPropertyCacheAliasCreator<QQmlTypeCompiler> *aliasCacheCreator, QQmlError *error)
{
    Q_UNUSED(component);

    const QmlIR::Object *obj = m_compiler->objectAt(objectIndex);
    if (!obj->aliasCount())
        return AllAliasesResolved;

    int aliasIndex = 0;
    int numSkippedAliases = 0;
    bool hasUnresolvedLocalAliases = false;

    for (const QmlIR::Alias *alias = obj->firstAlias(); alias; alias = alias->next, ++aliasIndex) {
        if (resolvedAliases.contains(alias)) {
            ++numSkippedAliases;
            continue;
        }


        const int idIndex = alias->idIndex();
        const int targetObjectIndex = m_idToObjectIndex.value(idIndex, -1);
        if (targetObjectIndex == -1) {
            *error = qQmlCompileError(
                    alias->referenceLocation(),
                    QQmlComponentAndAliasResolverBase::tr(
                            "Invalid alias reference. Unable to find id \"%1\"")
                            .arg(stringAt(idIndex)));
            break;
        }

        const QmlIR::Object *targetObject = m_compiler->objectAt(targetObjectIndex);
        Q_ASSERT(targetObject->id >= 0);
        const int resolvedTargetObjectId = targetObject->id;

        const QString aliasPropertyValue = stringAt(alias->propertyNameIndex());

        QStringView property;
        QStringView subProperty;

        const int propertySeparator = aliasPropertyValue.indexOf(QLatin1Char('.'));
        if (propertySeparator != -1) {
            property = QStringView{aliasPropertyValue}.left(propertySeparator);
            subProperty = QStringView{aliasPropertyValue}.mid(propertySeparator + 1);
        } else
            property = QStringView(aliasPropertyValue);

        QQmlPropertyIndex propIdx;

        if (property.isEmpty()) {
            // Alias points to object. No need to resolve properties.
        } else {
            QQmlPropertyCache::ConstPtr targetCache = m_propertyCaches->at(targetObjectIndex);
            if (!targetCache) {
                *error = qQmlCompileError(
                        alias->referenceLocation(),
                        QQmlComponentAndAliasResolverBase::tr("Invalid alias target location: %1")
                                .arg(property.toString()));
                break;
            }

            QQmlPropertyResolver resolver(targetCache);

            const QQmlPropertyData *targetProperty = resolver.property(
                    property.toString(), nullptr, QQmlPropertyResolver::IgnoreRevision);

            // If it's an alias that we haven't resolved yet, try again later.
            if (!targetProperty) {
                bool aliasPointsToOtherAlias = false;
                auto targetAlias = targetObject->aliasesBegin();
                for (const auto end = targetObject->aliasesEnd(); targetAlias != end;
                        ++targetAlias) {
                    if (stringAt(targetAlias->nameIndex()) == property) {
                        aliasPointsToOtherAlias = true;
                        break;
                    }
                }
                if (aliasPointsToOtherAlias) {
                    if (targetObjectIndex != objectIndex) {
                        // Don't continue, yet. We need to respect the order of objects.
                        return aliasIndex == numSkippedAliases
                                ? NoAliasResolved
                                : SomeAliasesResolved;
                    }

                    if (resolvedAliases.contains(targetAlias)) {
                        // Target already resolved. We can set the alias right away.
                        if (!appendAliasToPropertyCache(
                                    alias, objectIndex, aliasIndex, -1,
                                    resolvedTargetObjectId, targetObjectIndex, aliasCacheCreator,
                                    error)) {
                            break;
                        }
                        continue;
                    }

                    // Target isn't resolved yet, but it's in the same object.
                    // Continue with the other aliases.
                    // Try again later and resolve the target alias first.
                    ++numSkippedAliases;
                    hasUnresolvedLocalAliases = true;
                    continue;
                }
            }

            if (!targetProperty || targetProperty->coreIndex() > 0x0000FFFF) {
                *error = qQmlCompileError(
                        alias->referenceLocation(),
                        QQmlComponentAndAliasResolverBase::tr("Invalid alias target location: %1")
                                .arg(property.toString()));
                break;
            }

            propIdx = QQmlPropertyIndex(targetProperty->coreIndex());

            if (!subProperty.isEmpty()) {
                const QMetaObject *valueTypeMetaObject = QQmlMetaType::metaObjectForValueType(targetProperty->propType());
                if (!valueTypeMetaObject) {
                    // could be a deep alias
                    bool isDeepAlias = subProperty.at(0).isLower();
                    if (isDeepAlias) {
                        isDeepAlias = resolveDeepAlias(
                                m_compiler, targetObject, property, subProperty, propIdx,
                                targetProperty->propType(), m_propertyCaches, m_idToObjectIndex);
                    }
                    if (!isDeepAlias) {
                        *error = qQmlCompileError(
                                alias->referenceLocation(),
                                QQmlComponentAndAliasResolverBase::tr(
                                        "Invalid alias target location: %1")
                                        .arg(subProperty.toString()));
                        break;
                    }
                } else {

                    int valueTypeIndex =
                            valueTypeMetaObject->indexOfProperty(subProperty.toString().toUtf8().constData());
                    if (valueTypeIndex == -1) {
                        *error = qQmlCompileError(
                                alias->referenceLocation(),
                                QQmlComponentAndAliasResolverBase::tr(
                                        "Invalid alias target location: %1")
                                        .arg(subProperty.toString()));
                        break;
                    }
                    Q_ASSERT(valueTypeIndex <= 0x0000FFFF);

                    propIdx = QQmlPropertyIndex(propIdx.coreIndex(), valueTypeIndex);
                }
            }
        }

        if (!appendAliasToPropertyCache(
                    alias, objectIndex, aliasIndex, propIdx.toEncoded(),
                    resolvedTargetObjectId, targetObjectIndex, aliasCacheCreator, error)) {
            break;
        }
    }

    if (numSkippedAliases == aliasIndex)
        return NoAliasResolved;

    if (aliasIndex == obj->aliasCount() && !hasUnresolvedLocalAliases)
        return AllAliasesResolved;

    return SomeAliasesResolved;
}

namespace {

class RuntimeDataBuilder
{
public:
    RuntimeDataBuilder(QV4::CompiledData::CompilationUnit *unit, QQmlTypeLoader *typeLoader,
                       const QQmlImports *imports)
        : unit(unit), typeLoader(typeLoader), imports(imports)
    {
    }

    QList<QQmlError> build()
    {
        unit->allocateRuntimeData();
        for (int i = 0, count = unit->objectCount(); i < count; ++i) {
            if (!classifySignalHandlers(i))
                return errors;
        }

        for (int i = 0, count = unit->objectCount(); i < count; ++i) {
            if (unit->objectAt(i)->hasFlag(QV4::CompiledData::Object::IsInlineComponentRoot)) {
                if (!scanObject(i, ScopeDeferred::False))
                    return errors;
            }
        }
        scanObject(/*root object*/0, ScopeDeferred::False);
        if (!errors.isEmpty())
            return errors;

        for (int i = 0, count = unit->objectCount(); i < count; ++i) {
            classifyBindingsToAliases(i);
            computeBindingOrder(i);
            foldEnums(i);
        }
        return errors;
    }

private:
    enum class ScopeDeferred : bool { False, True };

    bool isValueBinding(int objectIndex, int bindingIndex) const
    {
        switch (unit->objectAt(objectIndex)->bindingTable()[bindingIndex].type()) {
        case QV4::CompiledData::Binding::Type_AttachedProperty:
        case QV4::CompiledData::Binding::Type_GroupedProperty:
            return false;
        default:
            return !unit->bindingIsSignalHandler(objectIndex, bindingIndex);
        }
    }

    bool classifySignalHandlers(int objectIndex)
    {
        const QQmlPropertyCache::ConstPtr cache = unit->propertyCaches.at(objectIndex);
        if (!cache)
            return true;

        const QV4::CompiledData::Object *obj = unit->objectAt(objectIndex);
        if (const QV4::ResolvedTypeReference *typeRef
                    = unit->resolvedType(obj->inheritedTypeNameIndex)) {
            if (QQmlCustomParser *customParser = typeRef->type().customParser()) {
                if (!(customParser->flags() & QQmlCustomParser::AcceptsSignalHandlers))
                    return true;
            }
        }

        return classifySignalHandlers(
                objectIndex, unit->stringAt(obj->inheritedTypeNameIndex), cache,
                QQmlPropertyResolver::CheckRevision);
    }

    bool classifySignalHandlers(
            int objectIndex, const QString &typeName,
            const QQmlPropertyCache::ConstPtr &propertyCache,
            QQmlPropertyResolver::RevisionCheck checkRevision)
    {
        using namespace QV4::CompiledData;

        const Object *obj = unit->objectAt(objectIndex);
        RuntimeBinding *runtimeBindings
                = unit->runtimeDataForWrite()->objectAt(objectIndex)->bindingTable();
        QSet<QString> customSignals;

        const Binding *binding = obj->bindingTable();
        for (quint32 i = 0; i < obj->nBindings; ++i, ++binding) {
            // Attached objects are visited twice: Once via the attached property and once on their
            // own.
            if (unit->bindingIsSignalHandler(objectIndex, i))
                continue;

            const QString bindingPropertyName = unit->stringAt(binding->propertyNameIndex);
            const Binding::Type bindingType = binding->type();

            if (bindingType == Binding::Type_AttachedProperty) {
                const QV4::ResolvedTypeReference *typeRef
                        = unit->resolvedType(binding->propertyNameIndex);
                QQmlType type = typeRef ? typeRef->type() : QQmlType();
                if (!type.isValid())
                    imports->resolveType(typeLoader, bindingPropertyName, &type, nullptr, nullptr);
                const QMetaObject *attachedType = type.attachedPropertiesType(typeLoader);
                if (!attachedType) {
                    return recordError(binding->location,
                                       QQmlTypeCompiler::tr("Non-existent attached object"));
                }

                // Ignore revisions of signals on attached objects. They are not unqualified.
                if (!classifySignalHandlers(binding->value.objectIndex, bindingPropertyName,
                                            QQmlMetaType::propertyCache(attachedType),
                                            QQmlPropertyResolver::IgnoreRevision)) {
                    return false;
                }
                continue;
            }

            const QString qPropertyName
                    = QQmlSignalNames::changedHandlerNameToPropertyName(bindingPropertyName)
                              .value_or(QString());
            const QString signalName
                    = QQmlSignalNames::anyHandlerNameToSignalName(bindingPropertyName)
                              .value_or(QString());
            if (signalName.isEmpty())
                continue;

            QQmlPropertyResolver resolver(propertyCache);
            bool notInRevision = false;
            const QQmlPropertyData *const signal
                    = resolver.signal(signalName, &notInRevision, checkRevision);
            const QQmlPropertyData *const signalPropertyData
                    = resolver.property(signalName, nullptr, checkRevision);
            const QQmlPropertyData *const qPropertyData = !qPropertyName.isEmpty()
                    ? resolver.property(qPropertyName, nullptr, checkRevision)
                    : nullptr;

            RuntimeBinding::Flag flag = RuntimeBinding::IsSignalHandlerExpression;
            const bool isPropertyObserver
                    = !signalPropertyData && qPropertyData && qPropertyData->notifiesViaBindable();
            if (signal && !(qPropertyData && qPropertyData->isAlias() && isPropertyObserver)) {
                int sigIndex = propertyCache->methodIndexToSignalIndex(signal->coreIndex());
                sigIndex = propertyCache->originalClone(sigIndex);
                bool unnamedParameter = false;
                const QList<QByteArray> parameterNames
                        = propertyCache->signalParameterNames(sigIndex);
                for (const QByteArray &parameterName : parameterNames) {
                    const QString param = QString::fromUtf8(parameterName);
                    if (param.isEmpty()) {
                        unnamedParameter = true;
                    } else if (unnamedParameter) {
                        return recordError(
                                binding->location,
                                QQmlTypeCompiler::tr("Signal uses unnamed parameter followed by "
                                                     "named parameter."));
                    } else if (QV4::Compiler::Codegen::isNameGlobal(param)) {
                        return recordError(
                                binding->location,
                                QQmlTypeCompiler::tr("Signal parameter \"%1\" hides global "
                                                     "variable.").arg(param));
                    }
                }
            } else if (isPropertyObserver) {
                flag = RuntimeBinding::IsPropertyObserver;
            } else {
                if (notInRevision) {
                    // Try assigning it as a property later
                    if (signalPropertyData)
                        continue;

                    if (const QV4::ResolvedTypeReference *typeRef
                                = unit->resolvedType(obj->inheritedTypeNameIndex);
                            typeRef && typeRef->type().isValid()) {
                        const QQmlType type = typeRef->type();
                        return recordError(
                                binding->location,
                                QQmlTypeCompiler::tr("\"%1.%2\" is not available in %3 %4.%5.")
                                        .arg(typeName, bindingPropertyName, type.module())
                                        .arg(type.version().majorVersion())
                                        .arg(type.version().minorVersion()));
                    }
                    return recordError(
                            binding->location,
                            QQmlTypeCompiler::tr("\"%1.%2\" is not available due to component "
                                                 "versioning.").arg(typeName, bindingPropertyName));
                }

                // Try to look up the signal parameter names in the object itself

                // build cache if necessary
                if (customSignals.isEmpty()) {
                    for (int s = 0, sc = obj->signalCount(); s < sc; ++s)
                        customSignals.insert(unit->stringAt(obj->signalAt(s)->nameIndex));
                    const Property *properties = obj->propertyTable();
                    for (int p = 0, pc = obj->propertyCount(); p < pc; ++p)
                        customSignals.insert(unit->stringAt(properties[p].nameIndex()));
                }

                // if not found, it's not a signal handler but a regular property assignment
                if (!customSignals.contains(signalName)
                        && (qPropertyName.isEmpty() || !customSignals.contains(qPropertyName))) {
                    continue;
                }
            }

            // Binding object to signal means connect the signal to the object's default method.
            if (bindingType == Binding::Type_Object) {
                runtimeBindings[i].flags |= RuntimeBinding::IsSignalHandlerObject;
                continue;
            }

            if (bindingType != Binding::Type_Script) {
                return recordError(
                        binding->location,
                        bindingType < Binding::Type_Script
                                ? QQmlTypeCompiler::tr("Cannot assign a value to a signal "
                                                       "(expecting a script to be run)")
                                : QQmlTypeCompiler::tr("Incorrectly specified signal assignment"));
            }

            runtimeBindings[i].flags |= flag;
        }
        return true;
    }

    void foldEnums(int objectIndex)
    {
        using namespace QV4::CompiledData;
        const Object *obj = unit->objectAt(objectIndex);
        RuntimeBinding *runtimeBindings
                = unit->runtimeDataForWrite()->objectAt(objectIndex)->bindingTable();
        const Binding *binding = obj->bindingTable();
        for (quint32 i = 0; i < obj->nBindings; ++i, ++binding) {
            if (binding->type() != Binding::Type_Script
                    || unit->bindingIsSignalHandler(objectIndex, i)) {
                continue;
            }

            if (const std::optional<int> value = evaluateQualifiedEnum(objectIndex, binding)) {
                runtimeBindings[i].flags |= RuntimeBinding::IsFoldedEnum;
                runtimeBindings[i].enumValue = *value;
            }
        }
    }

    std::optional<int> evaluateQualifiedEnum(int objectIndex,
                                             const QV4::CompiledData::Binding *binding)
    {
        const QQmlPropertyCache::ConstPtr cache = unit->propertyCaches.at(objectIndex);
        if (!cache)
            return std::nullopt;

        QQmlPropertyResolver resolver(cache);
        bool notInRevision = false;
        const QQmlPropertyData *prop =
                resolver.property(unit->stringAt(binding->propertyNameIndex), &notInRevision);
        if (!prop || prop->isQList())
            return std::nullopt;
        const bool isIntProp = (prop->propType().id() == QMetaType::Int) && !prop->isEnum();
        if (!prop->isEnum() && !isIntProp)
            return std::nullopt;

        if (!prop->isWritable()
            && !(binding->hasFlag(QV4::CompiledData::Binding::InitializerForReadOnlyDeclaration))) {
            recordError(binding->location,
                        QQmlTypeCompiler::tr(
                                "Invalid property assignment: \"%1\" is a read-only property")
                                .arg(unit->stringAt(binding->propertyNameIndex)));
            return std::nullopt;
        }

        const QString string = unit->stringAt(binding->stringIndex);
        if (string.isEmpty() || !string.constData()->isUpper())
            return std::nullopt;

        const int dot = string.indexOf(u'.');
        if (dot == -1 || dot == string.size() - 1)
            return std::nullopt;
        const int dot2 = string.indexOf(u'.', dot + 1);
        if (dot2 != -1 && dot2 != string.size() - 1) {
            if (!string.at(dot + 1).isUpper())
                return std::nullopt;
            if (string.indexOf(u'.', dot2 + 1) != -1)
                return std::nullopt;
        }

        const QHashedStringRef typeName(string.constData(), dot);
        const QStringView scopedEnumName =
                (dot2 != -1 ? QStringView{ string }.mid(dot + 1, dot2 - dot - 1) : QStringView());
        const QStringView enumValue = QStringView{ string }.mid(dot2 != -1 ? dot2 + 1 : dot + 1);

        if (isIntProp)
            return evaluateEnum(typeName.toString(), scopedEnumName, enumValue);

        QQmlType type;
        imports->resolveType(typeLoader, typeName, &type, nullptr, nullptr);
        if (!type.isValid())
            return std::nullopt;

        QV4::ResolvedTypeReference *tr =
                unit->resolvedType(unit->objectAt(objectIndex)->inheritedTypeNameIndex);
        bool useFastPath = type.isValid() && tr && tr->type() == type;
        QMetaProperty mprop;
        QMetaEnum menum;
        if (useFastPath) {
            mprop = cache->firstCppMetaObject()->property(prop->coreIndex());
            menum = mprop.enumerator();
            if (!menum.isScoped() && scopedEnumName.isEmpty()
                && typeName != QString::fromUtf8(menum.scope())) {
                useFastPath = false;
            }
        }

        int value = 0;
        bool ok = false;
        if (useFastPath) {
            const QByteArray enumName = enumValue.toUtf8();
            if (menum.isScoped() && !scopedEnumName.isEmpty()
                && enumName != scopedEnumName.toUtf8()) {
                return std::nullopt;
            }
            value = mprop.isFlagType() ? menum.keysToValue(enumName.constData(), &ok)
                                       : menum.keyToValue(enumName.constData(), &ok);
        } else {
            value = scopedEnumName.isEmpty()
                    ? type.enumValue(typeLoader, enumValue, &ok)
                    : type.scopedEnumValue(typeLoader, scopedEnumName, enumValue, &ok);
        }
        return ok ? std::optional<int>(value) : std::nullopt;
    }

    std::optional<int> evaluateEnum(const QString &scope, QStringView enumName,
                                    QStringView enumValue)
    {
        bool ok = false;
        QQmlType type;
        imports->resolveType(typeLoader, scope, &type, nullptr, nullptr);
        if (!type.isValid())
            return std::nullopt;

        const int value = enumName.isEmpty()
                ? type.enumValue(typeLoader,
                                 QHashedStringRef(enumValue.constData(), enumValue.size()), &ok)
                : type.scopedEnumValue(typeLoader, enumName, enumValue, &ok);
        return ok ? std::optional<int>(value) : std::nullopt;
    }

    void classifyBindingsToAliases(int objectIndex)
    {
        using namespace QV4::CompiledData;
        const QQmlPropertyCache::ConstPtr propertyCache = unit->propertyCaches.at(objectIndex);
        if (!propertyCache)
            return;

        const Object *obj = unit->objectAt(objectIndex);
        RuntimeBinding *runtimeBindings
                = unit->runtimeDataForWrite()->objectAt(objectIndex)->bindingTable();
        QQmlPropertyResolver resolver(propertyCache);
        const QQmlPropertyData *defaultProperty = obj->indexOfDefaultPropertyOrAlias != -1
                ? propertyCache->parent()->defaultProperty()
                : propertyCache->defaultProperty();

        const Binding *binding = obj->bindingTable();
        for (quint32 i = 0; i < obj->nBindings; ++i, ++binding) {
            if (!isValueBinding(objectIndex, i))
                continue;
            bool notInRevision = false;
            const QQmlPropertyData *pd = binding->propertyNameIndex != 0
                    ? resolver.property(unit->stringAt(binding->propertyNameIndex), &notInRevision)
                    : defaultProperty;
            if (pd && pd->isAlias())
                runtimeBindings[i].flags |= RuntimeBinding::IsBindingToAlias;
        }
    }

    void computeBindingOrder(int objectIndex)
    {
        using namespace QV4::CompiledData;
        const Object *obj = unit->objectAt(objectIndex);
        const Binding *bindings = obj->bindingTable();
        RuntimeObject *runtimeObject = unit->runtimeDataForWrite()->objectAt(objectIndex);
        const RuntimeBinding *runtimeBindings = runtimeObject->bindingTable();
        const auto isAlias = [&](quint32 b) {
            return runtimeBindings[b].flags & RuntimeBinding::IsBindingToAlias;
        };

        RuntimeBindingIndex *order = runtimeObject->bindingOrder();
        uint n = 0;
        const auto collect = [&](auto predicate) {
            for (quint16 b = 0; b < obj->nBindings; ++b) {
                if (predicate(b))
                    order[n++] = b;
            }
        };
        collect([&](quint16 b) { return isValueBinding(objectIndex, b) && !isAlias(b); });
        collect([&](quint16 b) { return unit->bindingIsSignalHandler(objectIndex, b); });
        collect([&](quint16 b) { return bindings[b].isAttachedProperty(); });
        collect([&](quint16 b) { return bindings[b].isGroupedProperty(); });
        collect([&](quint16 b) { return isValueBinding(objectIndex, b) && isAlias(b); });
        Q_ASSERT(n == obj->nBindings);
    }

    bool recordError(const QV4::CompiledData::Location &location, const QString &description)
    {
        QQmlError error;
        error.setUrl(unit->finalUrl());
        error.setLine(qmlConvertSourceCoordinate<quint32, int>(location.line()));
        error.setColumn(qmlConvertSourceCoordinate<quint32, int>(location.column()));
        error.setDescription(description);
        errors.append(error);
        return false;
    }

    bool deferredAndImmediatePropertyNames(
            const QV4::CompiledData::Object *obj, const QQmlPropertyCache::ConstPtr &propertyCache,
            QStringList *deferredPropertyNames, QStringList *immediatePropertyNames)
    {
        const QMetaObject *mo = propertyCache->firstCppMetaObject();
        const int deferredNamesIndex = mo->indexOfClassInfo("DeferredPropertyNames");
        const int immediateNamesIndex = mo->indexOfClassInfo("ImmediatePropertyNames");

        if (deferredNamesIndex != -1) {
            if (immediateNamesIndex != -1) {
                return recordError(obj->location,
                                   QQmlTypeCompiler::tr("You cannot define both DeferredPropertyNames "
                                                        "and ImmediatePropertyNames on the same type."));
            }
            *deferredPropertyNames
                    = QString::fromUtf8(mo->classInfo(deferredNamesIndex).value()).split(u',');
        } else if (immediateNamesIndex != -1) {
            *immediatePropertyNames
                    = QString::fromUtf8(mo->classInfo(immediateNamesIndex).value()).split(u',');
            // If the property contains an empty string, all properties shall be deferred.
            if (immediatePropertyNames->isEmpty())
                immediatePropertyNames->append(QString());
        }
        return true;
    }

    bool scanObject(int objectIndex, ScopeDeferred scopeDeferred)
    {
        using namespace QV4::CompiledData;

        if (objectIndex >= unit->objectCount())
            return scanObject(unit->resolvedIndex(objectIndex), scopeDeferred);

        const Object *obj = unit->objectAt(objectIndex);
        if (obj->idNameIndex != 0)
            seenObjectWithId = true;

        if (obj->hasFlag(Object::IsComponent)) {
            Q_ASSERT(obj->nBindings == 1);
            const Binding *componentBinding = obj->bindingTable();
            Q_ASSERT(componentBinding->type() == Binding::Type_Object);
            // A Component is separate from its surrounding scope. Its contents cannot be deferred.
            return scanObject(componentBinding->value.objectIndex, ScopeDeferred::False);
        }

        const QQmlPropertyCache::ConstPtr propertyCache = unit->propertyCaches.at(objectIndex);
        if (!propertyCache)
            return true;

        QString defaultPropertyName;
        const QQmlPropertyData *defaultProperty = nullptr;
        if (obj->indexOfDefaultPropertyOrAlias != -1) {
            const QQmlPropertyCache *parentCache = propertyCache->parent().data();
            defaultPropertyName = parentCache->defaultPropertyName();
            defaultProperty = parentCache->defaultProperty();
        } else {
            defaultPropertyName = propertyCache->defaultPropertyName();
            defaultProperty = propertyCache->defaultProperty();
        }

        QStringList deferredPropertyNames;
        QStringList immediatePropertyNames;
        if (!deferredAndImmediatePropertyNames(
                    obj, propertyCache, &deferredPropertyNames, &immediatePropertyNames)) {
            return false;
        }

        QQmlCustomParser *customParser = nullptr;
        if (const QV4::ResolvedTypeReference *typeRef
                    = unit->resolvedType(obj->inheritedTypeNameIndex)) {
            const QQmlType type = typeRef->type();
            if (type.isValid())
                customParser = type.customParser();
        }

        QQmlPropertyResolver propertyResolver(propertyCache);

        RuntimeObject *runtimeObject = unit->runtimeDataForWrite()->objectAt(objectIndex);

        const Binding *binding = obj->bindingTable();
        for (quint32 i = 0; i < obj->nBindings; ++i, ++binding) {
            QString name = unit->stringAt(binding->propertyNameIndex);

            const auto markCustomParser = [&]() {
                runtimeObject->flags |= RuntimeObject::HasCustomParserBindings;
                runtimeObject->bindingTable()[i].flags |= RuntimeBinding::IsCustomParserBinding;
            };

            if (customParser) {
                if (binding->type() == Binding::Type_AttachedProperty) {
                    if (customParser->flags() & QQmlCustomParser::AcceptsAttachedProperties) {
                        markCustomParser();
                        continue;
                    }
                } else if (QQmlSignalNames::isHandlerName(name)
                           && !(customParser->flags() & QQmlCustomParser::AcceptsSignalHandlers)) {
                    markCustomParser();
                    continue;
                }
            }

            const bool hasPropertyData = [&]() {
                if (name.isEmpty()) {
                    name = defaultPropertyName;
                    if (defaultProperty)
                        return true;
                } else if (name.constData()->isUpper()) {
                    // Upper case names cannot be custom-parsed unless they are attached properties
                    // and the custom parser explicitly accepts them. See above for that case.
                    return false;
                } else {
                    bool notInRevision = false;
                    if (propertyResolver.property(
                                name, &notInRevision, QQmlPropertyResolver::CheckRevision)) {
                        return true;
                    }
                }

                if (!customParser)
                    return false;

                // These signal handlers cannot be custom-parsed. The signal exists.
                if (unit->bindingIsSignalHandler(objectIndex, i))
                    return false;

                // If the property isn't found, we may want to custom-parse the binding.
                markCustomParser();
                return false;
            }();

            bool seenSubObjectWithId = false;
            bool isExternal = false;
            if (binding->type() >= Binding::Type_Object) {
                const bool isOwnProperty = hasPropertyData || binding->isAttachedProperty();
                isExternal = !isOwnProperty && binding->isGroupedProperty();
                if (isOwnProperty || isExternal) {
                    qSwap(seenObjectWithId, seenSubObjectWithId);

                    // Implicit component wrappers are scope boundaries.
                    const bool isImplicitComponent = unit->implicitComponentForObject(
                                binding->value.objectIndex) != -1;
                    const ScopeDeferred objectScope =
                            !isImplicitComponent
                                && (isExternal || scopeDeferred == ScopeDeferred::True)
                                ? ScopeDeferred::True
                                : ScopeDeferred::False;
                    const bool subObjectValid = scanObject(binding->value.objectIndex, objectScope);
                    qSwap(seenObjectWithId, seenSubObjectWithId);
                    seenObjectWithId |= seenSubObjectWithId;
                    if (!subObjectValid)
                        return false;
                }
            }

            bool isDeferred = false;
            if (!immediatePropertyNames.isEmpty() && !immediatePropertyNames.contains(name)) {
                if (seenSubObjectWithId) {
                    return recordError(binding->location,
                                       QQmlTypeCompiler::tr("You cannot assign an id to an object "
                                                            "assigned to a deferred property."));
                }
                if (isExternal || !disableInternalDeferredProperties())
                    isDeferred = true;
            } else if (!deferredPropertyNames.isEmpty() && deferredPropertyNames.contains(name)) {
                if (!seenSubObjectWithId && binding->type() != Binding::Type_GroupedProperty) {
                    if (isExternal || !disableInternalDeferredProperties())
                        isDeferred = true;
                }
            }

            if (binding->type() >= Binding::Type_Object && isExternal && !isDeferred
                    && !customParser) {
                return recordError(
                        binding->location,
                        QQmlTypeCompiler::tr("Cannot assign to non-existent property \"%1\"")
                                .arg(name));
            }

            if (isDeferred) {
                runtimeObject->bindingTable()[i].flags |= RuntimeBinding::IsDeferredBinding;
                runtimeObject->flags |= RuntimeObject::HasDeferredBindings;
                if (binding->type() == Binding::Type_AttachedProperty
                        || binding->type() == Binding::Type_GroupedProperty) {
                    unit->runtimeDataForWrite()->objectAt(binding->value.objectIndex)->flags
                            |= RuntimeObject::PopulatedByDeferredBinding;
                }
            }
        }

        return true;
    }

    QV4::CompiledData::CompilationUnit *unit;
    QQmlTypeLoader *typeLoader;
    const QQmlImports *imports;
    QList<QQmlError> errors;
    bool seenObjectWithId = false;
};

} // namespace

QList<QQmlError> QQmlTypeCompiler::computeRuntimeData(
        QV4::CompiledData::CompilationUnit *unit, QQmlTypeLoader *typeLoader,
        const QQmlImports *imports)
{
    return RuntimeDataBuilder(unit, typeLoader, imports).build();
}

QT_END_NAMESPACE
