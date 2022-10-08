// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmltypecompiler_p.h"

#include <private/qqmlobjectcreator_p.h>
#include <private/qqmlcustomparser_p.h>
#include <private/qqmlvmemetaobject_p.h>
#include <private/qqmlcomponent_p.h>
#include <private/qqmlpropertyresolver_p.h>

#define COMPILE_EXCEPTION(token, desc) \
    { \
        recordError((token)->location, desc); \
        return false; \
    }

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcQmlTypeCompiler, "qt.qml.typecompiler");

QQmlTypeCompiler::QQmlTypeCompiler(QQmlEnginePrivate *engine, QQmlTypeData *typeData,
                                   QmlIR::Document *parsedQML, const QQmlRefPointer<QQmlTypeNameCache> &typeNameCache,
                                   QV4::ResolvedTypeReferenceMap *resolvedTypeCache, const QV4::CompiledData::DependentTypesHasher &dependencyHasher)
    : resolvedTypes(resolvedTypeCache)
    , engine(engine)
    , dependencyHasher(dependencyHasher)
    , document(parsedQML)
    , typeNameCache(typeNameCache)
    , typeData(typeData)
{
}

QQmlRefPointer<QV4::ExecutableCompilationUnit> QQmlTypeCompiler::compile()
{
    // Build property caches and VME meta object data

    for (auto it = resolvedTypes->constBegin(), end = resolvedTypes->constEnd();
         it != end; ++it) {
        QQmlCustomParser *customParser = (*it)->type().customParser();
        if (customParser)
            customParsers.insert(it.key(), customParser);
    }

    QQmlPendingGroupPropertyBindings pendingGroupPropertyBindings;


    {
        QQmlPropertyCacheCreator<QQmlTypeCompiler> propertyCacheBuilder(&m_propertyCaches, &pendingGroupPropertyBindings,
                                                                        engine, this, imports(), typeData->typeClassName());
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

                QQmlComponentAndAliasResolver resolver(this);
                if (!resolver.resolve(result.processedRoot))
                    return nullptr;
                pendingGroupPropertyBindings.resolveMissingPropertyCaches(&m_propertyCaches);
                pendingGroupPropertyBindings.clear(); // anything that can be processed is now processed
            }
        } while (result.canResume);
    }

    {
        QQmlDefaultPropertyMerger merger(this);
        merger.mergeDefaultProperties();
    }

    {
        SignalHandlerResolver converter(this);
        if (!converter.resolveSignalHandlerExpressions())
            return nullptr;
    }

    {
        QQmlEnumTypeResolver enumResolver(this);
        if (!enumResolver.resolveEnumBindings())
            return nullptr;
    }

    {
        QQmlCustomParserScriptIndexer cpi(this);
        cpi.annotateBindingsWithScriptStrings();
    }

    {
        QQmlAliasAnnotator annotator(this);
        annotator.annotateBindingsToAliases();
    }

    {
        QQmlDeferredAndCustomParserBindingScanner deferredAndCustomParserBindingScanner(this);
        if (!deferredAndCustomParserBindingScanner.scanObject())
            return nullptr;
    }

    if (!document->javaScriptCompilationUnit.unitData()) {
        // Compile JS binding expressions and signal handlers if necessary
        {
            // We can compile script strings ahead of time, but they must be compiled
            // without type optimizations as their scope is always entirely dynamic.
            QQmlScriptStringScanner sss(this);
            sss.scan();
        }

        document->jsModule.fileName = typeData->urlString();
        document->jsModule.finalUrl = typeData->finalUrlString();
        QmlIR::JSCodeGen v4CodeGenerator(document, engine->v4engine()->illegalNames());
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

    QQmlRefPointer<QV4::ExecutableCompilationUnit> compilationUnit
            = QV4::ExecutableCompilationUnit::create(std::move(
                    document->javaScriptCompilationUnit));
    compilationUnit->typeNameCache = typeNameCache;
    compilationUnit->resolvedTypes = *resolvedTypes;
    compilationUnit->propertyCaches = std::move(m_propertyCaches);
    Q_ASSERT(compilationUnit->propertyCaches.count() == static_cast<int>(compilationUnit->objectCount()));
    return compilationUnit;
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

int QQmlTypeCompiler::registerConstant(QV4::ReturnedValue v)
{
    return document->jsGenerator.registerConstant(v);
}

const QV4::CompiledData::Unit *QQmlTypeCompiler::qmlUnit() const
{
    return document->javaScriptCompilationUnit.unitData();
}

const QQmlImports *QQmlTypeCompiler::imports() const
{
    return typeData->imports();
}

QVector<QmlIR::Object *> *QQmlTypeCompiler::qmlObjects() const
{
    return &document->objects;
}

void QQmlTypeCompiler::setPropertyCaches(QQmlPropertyCacheVector &&caches)
{
    m_propertyCaches = std::move(caches);
    Q_ASSERT(m_propertyCaches.count() > 0);
}

const QQmlPropertyCacheVector *QQmlTypeCompiler::propertyCaches() const
{
    return &m_propertyCaches;
}

QQmlPropertyCacheVector &&QQmlTypeCompiler::takePropertyCaches()
{
    return std::move(m_propertyCaches);
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

CompositeMetaTypeIds QQmlTypeCompiler::typeIdsForComponent(int objectId) const
{
    return typeData->typeIds(objectId);
}

QQmlCompilePass::QQmlCompilePass(QQmlTypeCompiler *typeCompiler)
    : compiler(typeCompiler)
{
}

SignalHandlerResolver::SignalHandlerResolver(QQmlTypeCompiler *typeCompiler)
    : QQmlCompilePass(typeCompiler)
    , enginePrivate(typeCompiler->enginePrivate())
    , qmlObjects(*typeCompiler->qmlObjects())
    , imports(typeCompiler->imports())
    , customParsers(typeCompiler->customParserCache())
    , illegalNames(typeCompiler->enginePrivate()->v4engine()->illegalNames())
    , propertyCaches(typeCompiler->propertyCaches())
{
}

bool SignalHandlerResolver::resolveSignalHandlerExpressions()
{
    for (int objectIndex = 0; objectIndex < qmlObjects.size(); ++objectIndex) {
        const QmlIR::Object * const obj = qmlObjects.at(objectIndex);
        QQmlPropertyCache::ConstPtr cache = propertyCaches->at(objectIndex);
        if (!cache)
            continue;
        if (QQmlCustomParser *customParser = customParsers.value(obj->inheritedTypeNameIndex)) {
            if (!(customParser->flags() & QQmlCustomParser::AcceptsSignalHandlers))
                continue;
        }
        const QString elementName = stringAt(obj->inheritedTypeNameIndex);
        if (!resolveSignalHandlerExpressions(obj, elementName, cache))
            return false;
    }
    return true;
}

bool SignalHandlerResolver::resolveSignalHandlerExpressions(
        const QmlIR::Object *obj, const QString &typeName,
        const QQmlPropertyCache::ConstPtr &propertyCache)
{
    // map from signal name defined in qml itself to list of parameters
    QHash<QString, QStringList> customSignals;

    for (QmlIR::Binding *binding = obj->firstBinding(); binding; binding = binding->next) {
        const QString bindingPropertyName = stringAt(binding->propertyNameIndex);
        // Attached property?
        const QV4::CompiledData::Binding::Type bindingType = binding->type();
        if (bindingType == QV4::CompiledData::Binding::Type_AttachedProperty) {
            const QmlIR::Object *attachedObj = qmlObjects.at(binding->value.objectIndex);
            auto *typeRef = resolvedType(binding->propertyNameIndex);
            QQmlType type = typeRef ? typeRef->type() : QQmlType();
            if (!type.isValid())
                imports->resolveType(bindingPropertyName, &type, nullptr, nullptr, nullptr);

            const QMetaObject *attachedType = type.attachedPropertiesType(enginePrivate);
            if (!attachedType)
                COMPILE_EXCEPTION(binding, tr("Non-existent attached object"));
            QQmlPropertyCache::ConstPtr cache = QQmlMetaType::propertyCache(attachedType);
            if (!resolveSignalHandlerExpressions(attachedObj, bindingPropertyName, cache))
                return false;
            continue;
        }

        if (!QmlIR::IRBuilder::isSignalPropertyName(bindingPropertyName))
            continue;

        QQmlPropertyResolver resolver(propertyCache);

        const QString signalName = QmlIR::IRBuilder::signalNameFromSignalPropertyName(
                    bindingPropertyName);

        QString qPropertyName;
        if (signalName.endsWith(QLatin1String("Changed")))
            qPropertyName = signalName.mid(0, signalName.size() - static_cast<int>(strlen("Changed")));

        bool notInRevision = false;
        const QQmlPropertyData * const signal = resolver.signal(signalName, &notInRevision);
        const QQmlPropertyData * const signalPropertyData = resolver.property(signalName, /*notInRevision ptr*/nullptr);
        const QQmlPropertyData * const qPropertyData = !qPropertyName.isEmpty() ? resolver.property(qPropertyName) : nullptr;
        QString finalSignalHandlerPropertyName = signalName;
        QV4::CompiledData::Binding::Flag flag
                = QV4::CompiledData::Binding::IsSignalHandlerExpression;

        const bool isPropertyObserver = !signalPropertyData && qPropertyData && qPropertyData->isBindable();
        if (signal && !(qPropertyData && qPropertyData->isAlias() && isPropertyObserver)) {
            int sigIndex = propertyCache->methodIndexToSignalIndex(signal->coreIndex());
            sigIndex = propertyCache->originalClone(sigIndex);

            bool unnamedParameter = false;

            QList<QByteArray> parameterNames = propertyCache->signalParameterNames(sigIndex);
            for (int i = 0; i < parameterNames.size(); ++i) {
                const QString param = QString::fromUtf8(parameterNames.at(i));
                if (param.isEmpty())
                    unnamedParameter = true;
                else if (unnamedParameter) {
                    COMPILE_EXCEPTION(binding, tr("Signal uses unnamed parameter followed by named parameter."));
                } else if (illegalNames.contains(param)) {
                    COMPILE_EXCEPTION(binding, tr("Signal parameter \"%1\" hides global variable.").arg(param));
                }
            }
        } else if (isPropertyObserver) {
            finalSignalHandlerPropertyName = qPropertyName;
            flag = QV4::CompiledData::Binding::IsPropertyObserver;
        } else {
            if (notInRevision) {
                // Try assinging it as a property later
                if (signalPropertyData)
                    continue;

                const QString &originalPropertyName = stringAt(binding->propertyNameIndex);

                auto *typeRef = resolvedType(obj->inheritedTypeNameIndex);
                const QQmlType type = typeRef ? typeRef->type() : QQmlType();
                if (type.isValid()) {
                    COMPILE_EXCEPTION(binding, tr("\"%1.%2\" is not available in %3 %4.%5.")
                                      .arg(typeName).arg(originalPropertyName).arg(type.module())
                                      .arg(type.version().majorVersion())
                                      .arg(type.version().minorVersion()));
                } else {
                    COMPILE_EXCEPTION(binding, tr("\"%1.%2\" is not available due to component versioning.").arg(typeName).arg(originalPropertyName));
                }
            }

            // Try to look up the signal parameter names in the object itself

            // build cache if necessary
            if (customSignals.isEmpty()) {
                for (const QmlIR::Signal *signal = obj->firstSignal(); signal; signal = signal->next) {
                    const QString &signalName = stringAt(signal->nameIndex);
                    customSignals.insert(signalName, signal->parameterStringList(compiler->stringPool()));
                }

                for (const QmlIR::Property *property = obj->firstProperty(); property; property = property->next) {
                    const QString propName = stringAt(property->nameIndex);
                    customSignals.insert(propName, QStringList());
                }
            }

            QHash<QString, QStringList>::ConstIterator entry = customSignals.constFind(signalName);
            if (entry == customSignals.constEnd() && !qPropertyName.isEmpty())
                entry = customSignals.constFind(qPropertyName);

            if (entry == customSignals.constEnd()) {
                // Can't find even a custom signal, then just don't do anything and try
                // keeping the binding as a regular property assignment.
                continue;
            }
        }

        // Binding object to signal means connect the signal to the object's default method.
        if (bindingType == QV4::CompiledData::Binding::Type_Object) {
            binding->setFlag(QV4::CompiledData::Binding::IsSignalHandlerObject);
            continue;
        }

        if (bindingType != QV4::CompiledData::Binding::Type_Script) {
            if (bindingType < QV4::CompiledData::Binding::Type_Script) {
                COMPILE_EXCEPTION(binding, tr("Cannot assign a value to a signal (expecting a script to be run)"));
            } else {
                COMPILE_EXCEPTION(binding, tr("Incorrectly specified signal assignment"));
            }
        }

        binding->propertyNameIndex = compiler->registerString(finalSignalHandlerPropertyName);
        binding->setFlag(flag);
    }
    return true;
}

QQmlEnumTypeResolver::QQmlEnumTypeResolver(QQmlTypeCompiler *typeCompiler)
    : QQmlCompilePass(typeCompiler)
    , qmlObjects(*typeCompiler->qmlObjects())
    , propertyCaches(typeCompiler->propertyCaches())
    , imports(typeCompiler->imports())
{
}

bool QQmlEnumTypeResolver::resolveEnumBindings()
{
    for (int i = 0; i < qmlObjects.size(); ++i) {
        QQmlPropertyCache::ConstPtr propertyCache = propertyCaches->at(i);
        if (!propertyCache)
            continue;
        const QmlIR::Object *obj = qmlObjects.at(i);

        QQmlPropertyResolver resolver(propertyCache);

        for (QmlIR::Binding *binding = obj->firstBinding(); binding; binding = binding->next) {
            const QV4::CompiledData::Binding::Flags bindingFlags = binding->flags();
            if (bindingFlags & QV4::CompiledData::Binding::IsSignalHandlerExpression
                    || bindingFlags & QV4::CompiledData::Binding::IsSignalHandlerObject
                    || bindingFlags & QV4::CompiledData::Binding::IsPropertyObserver)
                continue;

            if (binding->type() != QV4::CompiledData::Binding::Type_Script)
                continue;

            const QString propertyName = stringAt(binding->propertyNameIndex);
            bool notInRevision = false;
            const QQmlPropertyData *pd = resolver.property(propertyName, &notInRevision);
            if (!pd || pd->isQList())
                continue;

            if (!pd->isEnum() && pd->propType().id() != QMetaType::Int)
                continue;

            if (!tryQualifiedEnumAssignment(obj, propertyCache, pd, binding))
                return false;
        }
    }

    return true;
}

bool QQmlEnumTypeResolver::assignEnumToBinding(QmlIR::Binding *binding, QStringView, int enumValue, bool)
{
    binding->setType(QV4::CompiledData::Binding::Type_Number);
    binding->value.constantValueIndex = compiler->registerConstant(QV4::Encode((double)enumValue));
//    binding->setNumberValueInternal((double)enumValue);
    binding->setFlag(QV4::CompiledData::Binding::IsResolvedEnum);
    return true;
}

bool QQmlEnumTypeResolver::tryQualifiedEnumAssignment(
        const QmlIR::Object *obj, const QQmlPropertyCache::ConstPtr &propertyCache,
        const QQmlPropertyData *prop, QmlIR::Binding *binding)
{
    bool isIntProp = (prop->propType().id() == QMetaType::Int) && !prop->isEnum();
    if (!prop->isEnum() && !isIntProp)
        return true;

    if (!prop->isWritable()
            && !(binding->hasFlag(QV4::CompiledData::Binding::InitializerForReadOnlyDeclaration))) {
        COMPILE_EXCEPTION(binding, tr("Invalid property assignment: \"%1\" is a read-only property")
                                           .arg(stringAt(binding->propertyNameIndex)));
    }

    Q_ASSERT(binding->type() == QV4::CompiledData::Binding::Type_Script);
    const QString string = compiler->bindingAsString(obj, binding->value.compiledScriptIndex);
    if (!string.constData()->isUpper())
        return true;

    // reject any "complex" expression (even simple arithmetic)
    // we do this by excluding everything that is not part of a
    // valid identifier or a dot
    for (const QChar &c : string)
        if (!(c.isLetterOrNumber() || c == u'.' || c == u'_' || c.isSpace()))
            return true;

    // we support one or two '.' in the enum phrase:
    // * <TypeName>.<EnumValue>
    // * <TypeName>.<ScopedEnumName>.<EnumValue>

    int dot = string.indexOf(QLatin1Char('.'));
    if (dot == -1 || dot == string.size()-1)
        return true;

    int dot2 = string.indexOf(QLatin1Char('.'), dot+1);
    if (dot2 != -1 && dot2 != string.size()-1) {
        if (!string.at(dot+1).isUpper())
            return true;
        if (string.indexOf(QLatin1Char('.'), dot2+1) != -1)
            return true;
    }

    QHashedStringRef typeName(string.constData(), dot);
    const bool isQtObject = (typeName == QLatin1String("Qt"));
    const QStringView scopedEnumName = (dot2 != -1 ? QStringView{string}.mid(dot + 1, dot2 - dot - 1) : QStringView());
    // ### consider supporting scoped enums in Qt namespace
    const QStringView enumValue = QStringView{string}.mid(!isQtObject && dot2 != -1 ? dot2 + 1 : dot + 1);

    if (isIntProp) { // ### C++11 allows enums to be other integral types. Should we support other integral types here?
        // Allow enum assignment to ints.
        bool ok;
        int enumval = evaluateEnum(typeName.toString(), scopedEnumName, enumValue, &ok);
        if (ok) {
            if (!assignEnumToBinding(binding, enumValue, enumval, isQtObject))
                return false;
        }
        return true;
    }
    QQmlType type;
    imports->resolveType(typeName, &type, nullptr, nullptr, nullptr);

    if (!type.isValid() && !isQtObject)
        return true;

    int value = 0;
    bool ok = false;

    auto *tr = resolvedType(obj->inheritedTypeNameIndex);

    // When these two match, we can short cut the search, unless...
    bool useFastPath = type.isValid() && tr && tr->type() == type;
    QMetaProperty mprop;
    QMetaEnum menum;
    if (useFastPath) {
        mprop = propertyCache->firstCppMetaObject()->property(prop->coreIndex());
        menum = mprop.enumerator();
        // ...the enumerator merely comes from a related metaobject, but the enum scope does not match
        // the typename we resolved
        if (!menum.isScoped() && scopedEnumName.isEmpty() && typeName != QString::fromUtf8(menum.scope()))
            useFastPath = false;;
    }
    if (useFastPath) {
        QByteArray enumName = enumValue.toUtf8();
        if (menum.isScoped() && !scopedEnumName.isEmpty() && enumName != scopedEnumName.toUtf8())
            return true;

        if (mprop.isFlagType()) {
            value = menum.keysToValue(enumName.constData(), &ok);
        } else {
            value = menum.keyToValue(enumName.constData(), &ok);
        }
    } else {
        // Otherwise we have to search the whole type
        if (type.isValid()) {
            if (!scopedEnumName.isEmpty())
                value = type.scopedEnumValue(compiler->enginePrivate(), scopedEnumName, enumValue, &ok);
            else
                value = type.enumValue(compiler->enginePrivate(), QHashedStringRef(enumValue), &ok);
        } else {
            QByteArray enumName = enumValue.toUtf8();
            const QMetaObject *metaObject = &Qt::staticMetaObject;
            for (int ii = metaObject->enumeratorCount() - 1; !ok && ii >= 0; --ii) {
                QMetaEnum e = metaObject->enumerator(ii);
                value = e.keyToValue(enumName.constData(), &ok);
            }
        }
    }

    if (!ok)
        return true;

    return assignEnumToBinding(binding, enumValue, value, isQtObject);
}

int QQmlEnumTypeResolver::evaluateEnum(const QString &scope, QStringView enumName, QStringView enumValue, bool *ok) const
{
    Q_ASSERT_X(ok, "QQmlEnumTypeResolver::evaluateEnum", "ok must not be a null pointer");
    *ok = false;

    if (scope != QLatin1String("Qt")) {
        QQmlType type;
        imports->resolveType(scope, &type, nullptr, nullptr, nullptr);
        if (!type.isValid())
            return -1;
        if (!enumName.isEmpty())
            return type.scopedEnumValue(compiler->enginePrivate(), enumName, enumValue, ok);
        return type.enumValue(compiler->enginePrivate(), QHashedStringRef(enumValue.constData(), enumValue.size()), ok);
    }

    const QMetaObject *mo = &Qt::staticMetaObject;
    int i = mo->enumeratorCount();
    const QByteArray ba = enumValue.toUtf8();
    while (i--) {
        int v = mo->enumerator(i).keyToValue(ba.constData(), ok);
        if (*ok)
            return v;
    }
    return -1;
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
        case QV4::CompiledData::Binding::Type_GroupProperty:
            scanObjectRecursively(binding->value.objectIndex, annotateScriptBindings);
            break;
        default:
            break;
        }
    }
}

QQmlAliasAnnotator::QQmlAliasAnnotator(QQmlTypeCompiler *typeCompiler)
    : QQmlCompilePass(typeCompiler)
    , qmlObjects(*typeCompiler->qmlObjects())
    , propertyCaches(typeCompiler->propertyCaches())
{
}

void QQmlAliasAnnotator::annotateBindingsToAliases()
{
    for (int i = 0; i < qmlObjects.size(); ++i) {
        QQmlPropertyCache::ConstPtr propertyCache = propertyCaches->at(i);
        if (!propertyCache)
            continue;

        const QmlIR::Object *obj = qmlObjects.at(i);

        QQmlPropertyResolver resolver(propertyCache);
        const QQmlPropertyData *defaultProperty = obj->indexOfDefaultPropertyOrAlias != -1 ? propertyCache->parent()->defaultProperty() : propertyCache->defaultProperty();

        for (QmlIR::Binding *binding = obj->firstBinding(); binding; binding = binding->next) {
            if (!binding->isValueBinding())
                continue;
            bool notInRevision = false;
            const QQmlPropertyData *pd = binding->propertyNameIndex != quint32(0) ? resolver.property(stringAt(binding->propertyNameIndex), &notInRevision) : defaultProperty;
            if (pd && pd->isAlias())
                binding->setFlag(QV4::CompiledData::Binding::IsBindingToAlias);
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

QQmlComponentAndAliasResolver::QQmlComponentAndAliasResolver(QQmlTypeCompiler *typeCompiler)
    : QQmlCompilePass(typeCompiler)
    , enginePrivate(typeCompiler->enginePrivate())
    , pool(typeCompiler->memoryPool())
    , qmlObjects(typeCompiler->qmlObjects())
    , propertyCaches(std::move(typeCompiler->takePropertyCaches()))
{
}

static bool isUsableComponent(const QMetaObject *metaObject)
{
    // The metaObject is a component we're interested in if it either is a QQmlComponent itself
    // or if any of its parents is a QQmlAbstractDelegateComponent. We don't want to include
    // qqmldelegatecomponent_p.h because it belongs to QtQmlModels.

    if (metaObject == &QQmlComponent::staticMetaObject)
        return true;

    for (; metaObject; metaObject = metaObject->superClass()) {
        if (qstrcmp(metaObject->className(), "QQmlAbstractDelegateComponent") == 0)
            return true;
    }

    return false;
}

void QQmlComponentAndAliasResolver::findAndRegisterImplicitComponents(
        const QmlIR::Object *obj, const QQmlPropertyCache::ConstPtr &propertyCache)
{
    QQmlPropertyResolver propertyResolver(propertyCache);

    const QQmlPropertyData *defaultProperty = obj->indexOfDefaultPropertyOrAlias != -1 ? propertyCache->parent()->defaultProperty() : propertyCache->defaultProperty();

    for (QmlIR::Binding *binding = obj->firstBinding(); binding; binding = binding->next) {
        if (binding->type() != QV4::CompiledData::Binding::Type_Object)
            continue;
        if (binding->hasFlag(QV4::CompiledData::Binding::IsSignalHandlerObject))
            continue;

        const QmlIR::Object *targetObject = qmlObjects->at(binding->value.objectIndex);
        auto *tr = resolvedType(targetObject->inheritedTypeNameIndex);
        Q_ASSERT(tr);

        const QMetaObject *firstMetaObject = nullptr;
        const auto type = tr->type();
        if (type.isValid())
            firstMetaObject = type.metaObject();
        else if (const auto compilationUnit = tr->compilationUnit())
            firstMetaObject = compilationUnit->rootPropertyCache()->firstCppMetaObject();
        if (isUsableComponent(firstMetaObject))
            continue;
        // if here, not a QQmlComponent, so needs wrapping

        const QQmlPropertyData *pd = nullptr;
        if (binding->propertyNameIndex != quint32(0)) {
            bool notInRevision = false;
            pd = propertyResolver.property(stringAt(binding->propertyNameIndex), &notInRevision);
        } else {
            pd = defaultProperty;
        }
        if (!pd || !pd->isQObject())
            continue;

        // If the version is given, use it and look up by QQmlType.
        // Otherwise, make sure we look up by metaobject.
        // TODO: Is this correct?
        QQmlPropertyCache::ConstPtr pc = pd->typeVersion().hasMinorVersion()
                ? QQmlMetaType::rawPropertyCacheForType(pd->propType(), pd->typeVersion())
                : QQmlMetaType::rawPropertyCacheForType(pd->propType());
        const QMetaObject *mo = pc ? pc->firstCppMetaObject() : nullptr;
        while (mo) {
            if (mo == &QQmlComponent::staticMetaObject)
                break;
            mo = mo->superClass();
        }

        if (!mo)
            continue;

        // emulate "import QML 1.0" and then wrap the component in "QML.Component {}"
        QQmlType componentType = QQmlMetaType::qmlType(
                    &QQmlComponent::staticMetaObject, QStringLiteral("QML"),
                    QTypeRevision::fromVersion(1, 0));
        Q_ASSERT(componentType.isValid());
        const QString qualifier = QStringLiteral("QML");

        compiler->addImport(componentType.module(), qualifier, componentType.version());

        QmlIR::Object *syntheticComponent = pool->New<QmlIR::Object>();
        syntheticComponent->init(
                    pool,
                    compiler->registerString(qualifier + QLatin1Char('.') + componentType.elementName()),
                    compiler->registerString(QString()), binding->valueLocation);
        syntheticComponent->flags |= QV4::CompiledData::Object::IsComponent;

        if (!containsResolvedType(syntheticComponent->inheritedTypeNameIndex)) {
            auto typeRef = new QV4::ResolvedTypeReference;
            typeRef->setType(componentType);
            typeRef->setVersion(componentType.version());
            insertResolvedType(syntheticComponent->inheritedTypeNameIndex, typeRef);
        }

        qmlObjects->append(syntheticComponent);
        const int componentIndex = qmlObjects->size() - 1;
        // Keep property caches symmetric
        QQmlPropertyCache::ConstPtr componentCache
                = QQmlMetaType::propertyCache(&QQmlComponent::staticMetaObject);
        propertyCaches.append(componentCache);

        QmlIR::Binding *syntheticBinding = pool->New<QmlIR::Binding>();
        *syntheticBinding = *binding;

        // The synthetic binding inside Component has no name. It's just "Component { Foo {} }".
        syntheticBinding->propertyNameIndex = 0;

        syntheticBinding->setType(QV4::CompiledData::Binding::Type_Object);
        QString error = syntheticComponent->appendBinding(syntheticBinding, /*isListBinding*/false);
        Q_ASSERT(error.isEmpty());
        Q_UNUSED(error);

        binding->value.objectIndex = componentIndex;

        componentRoots.append(componentIndex);
    }
}

// Resolve ignores everything relating to inline components, except for implicit components.
bool QQmlComponentAndAliasResolver::resolve(int root)
{
    // Detect real Component {} objects as well as implicitly defined components, such as
    //     someItemDelegate: Item {}
    // In the implicit case Item is surrounded by a synthetic Component {} because the property
    // on the left hand side is of QQmlComponent type.
    const int objCountWithoutSynthesizedComponents = qmlObjects->size();
    const int startObjectIndex = root == 0 ? root : root+1; // root+1, as ic root is handled at the end
    for (int i = startObjectIndex; i < objCountWithoutSynthesizedComponents; ++i) {
        QmlIR::Object *obj = qmlObjects->at(i);
        const bool isInlineComponentRoot
                = obj->flags & QV4::CompiledData::Object::IsInlineComponentRoot;
        const bool isPartOfInlineComponent
                = obj->flags & QV4::CompiledData::Object::IsPartOfInlineComponent;
        QQmlPropertyCache::ConstPtr cache = propertyCaches.at(i);

        bool isExplicitComponent = false;
        if (obj->inheritedTypeNameIndex) {
            auto *tref = resolvedType(obj->inheritedTypeNameIndex);
            Q_ASSERT(tref);
            if (tref->type().metaObject() == &QQmlComponent::staticMetaObject)
                isExplicitComponent = true;
        }

        if (isInlineComponentRoot && isExplicitComponent) {
            qCWarning(lcQmlTypeCompiler).nospace().noquote()
                    << compiler->url().toString() << ":" << obj->location.line() << ":"
                    << obj->location.column()
                    << ": Using a Component as the root of an inline component is deprecated: "
                       "inline components are "
                       "automatically wrapped into Components when needed.";
        }

        if (root == 0) {
            // normal component root, skip over anything inline component related
            if (isInlineComponentRoot || isPartOfInlineComponent)
                continue;
        } else if (!isPartOfInlineComponent || isInlineComponentRoot) {
            // We've left the current inline component (potentially entered a new one),
            // but we still need to resolve implicit components which are part of inline components.
            if (cache && !isExplicitComponent)
                findAndRegisterImplicitComponents(obj, cache);
            break;
        }

        if (obj->inheritedTypeNameIndex == 0 && !cache)
            continue;

        if (!isExplicitComponent) {
            if (cache)
                findAndRegisterImplicitComponents(obj, cache);
            continue;
        }

        obj->flags |= QV4::CompiledData::Object::IsComponent;

        // check if this object is the root
        if (i == 0) {
            if (isExplicitComponent)
                qCWarning(lcQmlTypeCompiler).nospace().noquote()
                        << compiler->url().toString() << ":" << obj->location.line() << ":"
                        << obj->location.column()
                        << ": Using a Component as the root of a qmldocument is deprecated: types "
                           "defined in qml documents are "
                           "automatically wrapped into Components when needed.";
        }

        if (obj->functionCount() > 0)
            COMPILE_EXCEPTION(obj, tr("Component objects cannot declare new functions."));
        if (obj->propertyCount() > 0 || obj->aliasCount() > 0)
            COMPILE_EXCEPTION(obj, tr("Component objects cannot declare new properties."));
        if (obj->signalCount() > 0)
            COMPILE_EXCEPTION(obj, tr("Component objects cannot declare new signals."));

        if (obj->bindingCount() == 0)
            COMPILE_EXCEPTION(obj, tr("Cannot create empty component specification"));

        const QmlIR::Binding *rootBinding = obj->firstBinding();

        for (const QmlIR::Binding *b = rootBinding; b; b = b->next) {
            if (b->propertyNameIndex != 0)
                COMPILE_EXCEPTION(rootBinding, tr("Component elements may not contain properties other than id"));
        }

        if (rootBinding->next || rootBinding->type() != QV4::CompiledData::Binding::Type_Object)
            COMPILE_EXCEPTION(obj, tr("Invalid component body specification"));

        // For the root object, we are going to collect ids/aliases and resolve them for as a separate
        // last pass.
        if (i != 0)
            componentRoots.append(i);

    }

    for (int i = 0; i < componentRoots.size(); ++i) {
        QmlIR::Object *component  = qmlObjects->at(componentRoots.at(i));
        const QmlIR::Binding *rootBinding = component->firstBinding();

        _idToObjectIndex.clear();

        _objectsWithAliases.clear();

        if (!collectIdsAndAliases(rootBinding->value.objectIndex))
            return false;

        component->namedObjectsInComponent.allocate(pool, _idToObjectIndex);

        if (!resolveAliases(componentRoots.at(i)))
            return false;
    }

    // Collect ids and aliases for root
    _idToObjectIndex.clear();
    _objectsWithAliases.clear();

    collectIdsAndAliases(root);

    QmlIR::Object *rootComponent = qmlObjects->at(root);
    rootComponent->namedObjectsInComponent.allocate(pool, _idToObjectIndex);

    if (!resolveAliases(root))
        return false;

    // Implicit component insertion may have added objects and thus we also need
    // to extend the symmetric propertyCaches.
    compiler->setPropertyCaches(std::move(propertyCaches));
    compiler->setComponentRoots(componentRoots);

    return true;
}

bool QQmlComponentAndAliasResolver::collectIdsAndAliases(int objectIndex)
{
    QmlIR::Object *obj = qmlObjects->at(objectIndex);

    if (obj->idNameIndex != 0) {
        if (_idToObjectIndex.contains(obj->idNameIndex)) {
            recordError(obj->locationOfIdProperty, tr("id is not unique"));
            return false;
        }
        obj->id = _idToObjectIndex.size();
        _idToObjectIndex.insert(obj->idNameIndex, objectIndex);
    }

    if (obj->aliasCount() > 0)
        _objectsWithAliases.append(objectIndex);

    // Stop at Component boundary
    if (obj->flags & QV4::CompiledData::Object::IsComponent && objectIndex != /*root object*/0)
        return true;

    for (const QmlIR::Binding *binding = obj->firstBinding(); binding; binding = binding->next) {
        switch (binding->type()) {
        case QV4::CompiledData::Binding::Type_Object:
        case QV4::CompiledData::Binding::Type_AttachedProperty:
        case QV4::CompiledData::Binding::Type_GroupProperty:
            if (!collectIdsAndAliases(binding->value.objectIndex))
                return false;
            break;
        default:
            break;
        }
    }

    return true;
}

bool QQmlComponentAndAliasResolver::resolveAliases(int componentIndex)
{
    if (_objectsWithAliases.isEmpty())
        return true;

    QQmlPropertyCacheAliasCreator<QQmlTypeCompiler> aliasCacheCreator(&propertyCaches, compiler);

    bool atLeastOneAliasResolved;
    do {
        atLeastOneAliasResolved = false;
        QVector<int> pendingObjects;

        for (int objectIndex: std::as_const(_objectsWithAliases)) {

            QQmlError error;
            const auto result = resolveAliasesInObject(objectIndex, &error);

            if (error.isValid()) {
                recordError(error);
                return false;
            }

            if (result == AllAliasesResolved) {
                QQmlError error = aliasCacheCreator.appendAliasesToPropertyCache(*qmlObjects->at(componentIndex), objectIndex, enginePrivate);
                if (error.isValid()) {
                    recordError(error);
                    return false;
                }
                atLeastOneAliasResolved = true;
            } else if (result == SomeAliasesResolved) {
                atLeastOneAliasResolved = true;
                pendingObjects.append(objectIndex);
            } else {
                pendingObjects.append(objectIndex);
            }
        }
        qSwap(_objectsWithAliases, pendingObjects);
    } while (!_objectsWithAliases.isEmpty() && atLeastOneAliasResolved);

    if (!atLeastOneAliasResolved && !_objectsWithAliases.isEmpty()) {
        const QmlIR::Object *obj = qmlObjects->at(_objectsWithAliases.first());
        for (auto alias = obj->aliasesBegin(), end = obj->aliasesEnd(); alias != end; ++alias) {
            if (!alias->hasFlag(QV4::CompiledData::Alias::Resolved)) {
                recordError(alias->location, tr("Circular alias reference detected"));
                return false;
            }
        }
    }

    return true;
}

QQmlComponentAndAliasResolver::AliasResolutionResult
QQmlComponentAndAliasResolver::resolveAliasesInObject(int objectIndex,
                                                      QQmlError *error)
{
    const QmlIR::Object * const obj = qmlObjects->at(objectIndex);
    if (!obj->aliasCount())
        return AllAliasesResolved;

    int numResolvedAliases = 0;
    bool seenUnresolvedAlias = false;

    for (QmlIR::Alias *alias = obj->firstAlias(); alias; alias = alias->next) {
        if (alias->hasFlag(QV4::CompiledData::Alias::Resolved))
            continue;

        seenUnresolvedAlias = true;

        const int idIndex = alias->idIndex();
        const int targetObjectIndex = _idToObjectIndex.value(idIndex, -1);
        if (targetObjectIndex == -1) {
            *error = qQmlCompileError(
                    alias->referenceLocation,
                    tr("Invalid alias reference. Unable to find id \"%1\"").arg(stringAt(idIndex)));
            break;
        }

        const QmlIR::Object *targetObject = qmlObjects->at(targetObjectIndex);
        Q_ASSERT(targetObject->id >= 0);
        alias->setTargetObjectId(targetObject->id);
        alias->setIsAliasToLocalAlias(false);

        const QString aliasPropertyValue = stringAt(alias->propertyNameIndex);

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
            alias->setFlag(QV4::CompiledData::Alias::AliasPointsToPointerObject);
        } else {
            QQmlPropertyCache::ConstPtr targetCache = propertyCaches.at(targetObjectIndex);
            if (!targetCache) {
                *error = qQmlCompileError(
                        alias->referenceLocation,
                        tr("Invalid alias target location: %1").arg(property.toString()));
                break;
            }

            QQmlPropertyResolver resolver(targetCache);

            const QQmlPropertyData *targetProperty = resolver.property(property.toString());

            // If it's an alias that we haven't resolved yet, try again later.
            if (!targetProperty) {
                bool aliasPointsToOtherAlias = false;
                int localAliasIndex = 0;
                for (auto targetAlias = targetObject->aliasesBegin(), end = targetObject->aliasesEnd(); targetAlias != end; ++targetAlias, ++localAliasIndex) {
                    if (stringAt(targetAlias->nameIndex()) == property) {
                        aliasPointsToOtherAlias = true;
                        break;
                    }
                }
                if (aliasPointsToOtherAlias) {
                    if (targetObjectIndex == objectIndex) {
                        alias->localAliasIndex = localAliasIndex;
                        alias->setIsAliasToLocalAlias(true);
                        alias->setFlag(QV4::CompiledData::Alias::Resolved);
                        ++numResolvedAliases;
                        continue;
                    }

                    // restore
                    alias->setIdIndex(idIndex);
                    // Try again later and resolve the target alias first.
                    break;
                }
            }

            if (!targetProperty || targetProperty->coreIndex() > 0x0000FFFF) {
                *error = qQmlCompileError(
                        alias->referenceLocation,
                        tr("Invalid alias target location: %1").arg(property.toString()));
                break;
            }

            propIdx = QQmlPropertyIndex(targetProperty->coreIndex());

            if (!subProperty.isEmpty()) {
                const QMetaObject *valueTypeMetaObject = QQmlMetaType::metaObjectForValueType(targetProperty->propType());
                if (!valueTypeMetaObject) {
                    // could be a deep alias
                    bool isDeepAlias = subProperty.at(0).isLower();
                    if (isDeepAlias) {
                        isDeepAlias = false;
                        for (auto it = targetObject->bindingsBegin(); it != targetObject->bindingsEnd(); ++it) {
                            auto binding = *it;
                            if (compiler->stringAt(binding.propertyNameIndex) == property) {
                                resolver = QQmlPropertyResolver(propertyCaches.at(binding.value.objectIndex));
                                const QQmlPropertyData *actualProperty = resolver.property(subProperty.toString());
                                if (actualProperty) {
                                    propIdx = QQmlPropertyIndex(propIdx.coreIndex(), actualProperty->coreIndex());
                                    isDeepAlias = true;
                                }
                            }
                        }
                    }
                    if (!isDeepAlias) {
                        *error = qQmlCompileError(
                                alias->referenceLocation,
                                tr("Invalid alias target location: %1").arg(subProperty.toString()));
                        break;
                    }
                } else {

                    int valueTypeIndex =
                            valueTypeMetaObject->indexOfProperty(subProperty.toString().toUtf8().constData());
                    if (valueTypeIndex == -1) {
                        *error = qQmlCompileError(
                                alias->referenceLocation,
                                tr("Invalid alias target location: %1").arg(subProperty.toString()));
                        break;
                    }
                    Q_ASSERT(valueTypeIndex <= 0x0000FFFF);

                    propIdx = QQmlPropertyIndex(propIdx.coreIndex(), valueTypeIndex);
                }
            } else {
                if (targetProperty->isQObject())
                    alias->setFlag(QV4::CompiledData::Alias::AliasPointsToPointerObject);
            }
        }

        alias->encodedMetaPropertyIndex = propIdx.toEncoded();
        alias->setFlag(QV4::CompiledData::Alias::Resolved);
        numResolvedAliases++;
    }

    if (numResolvedAliases == 0)
        return seenUnresolvedAlias ? NoAliasResolved : AllAliasesResolved;

    return SomeAliasesResolved;
}

QQmlDeferredAndCustomParserBindingScanner::QQmlDeferredAndCustomParserBindingScanner(QQmlTypeCompiler *typeCompiler)
    : QQmlCompilePass(typeCompiler)
    , qmlObjects(typeCompiler->qmlObjects())
    , propertyCaches(typeCompiler->propertyCaches())
    , customParsers(typeCompiler->customParserCache())
    , _seenObjectWithId(false)
{
}

bool QQmlDeferredAndCustomParserBindingScanner::scanObject()
{
    for (int i = 0; i < qmlObjects->size(); ++i) {
        if ((qmlObjects->at(i)->flags & QV4::CompiledData::Object::IsInlineComponentRoot)
                && !scanObject(i, ScopeDeferred::False)) {
            return false;
        }
    }
    return scanObject(/*root object*/0, ScopeDeferred::False);
}

bool QQmlDeferredAndCustomParserBindingScanner::scanObject(
        int objectIndex, ScopeDeferred scopeDeferred)
{
    using namespace QV4::CompiledData;

    QmlIR::Object *obj = qmlObjects->at(objectIndex);
    if (obj->idNameIndex != 0)
        _seenObjectWithId = true;

    if (obj->flags & Object::IsComponent) {
        Q_ASSERT(obj->bindingCount() == 1);
        const Binding *componentBinding = obj->firstBinding();
        Q_ASSERT(componentBinding->type() == Binding::Type_Object);
        // Components are separate from their surrounding scope. They cannot be deferred.
        return scanObject(componentBinding->value.objectIndex, ScopeDeferred::False);
    }

    QQmlPropertyCache::ConstPtr propertyCache = propertyCaches->at(objectIndex);
    if (!propertyCache)
        return true;

    QString defaultPropertyName;
    const QQmlPropertyData *defaultProperty = nullptr;
    if (obj->indexOfDefaultPropertyOrAlias != -1) {
        const QQmlPropertyCache *cache = propertyCache->parent().data();
        defaultPropertyName = cache->defaultPropertyName();
        defaultProperty = cache->defaultProperty();
    } else {
        defaultPropertyName = propertyCache->defaultPropertyName();
        defaultProperty = propertyCache->defaultProperty();
    }

    QQmlCustomParser *customParser = customParsers.value(obj->inheritedTypeNameIndex);

    QQmlPropertyResolver propertyResolver(propertyCache);

    QStringList deferredPropertyNames;
    QStringList immediatePropertyNames;
    {
        const QMetaObject *mo = propertyCache->firstCppMetaObject();
        const int deferredNamesIndex = mo->indexOfClassInfo("DeferredPropertyNames");
        const int immediateNamesIndex = mo->indexOfClassInfo("ImmediatePropertyNames");
        if (deferredNamesIndex != -1) {
            if (immediateNamesIndex != -1) {
                COMPILE_EXCEPTION(obj, tr("You cannot define both DeferredPropertyNames and "
                                          "ImmediatePropertyNames on the same type."));
            }
            const QMetaClassInfo classInfo = mo->classInfo(deferredNamesIndex);
            deferredPropertyNames = QString::fromUtf8(classInfo.value()).split(u',');
        } else if (immediateNamesIndex != -1) {
            const QMetaClassInfo classInfo = mo->classInfo(immediateNamesIndex);
            immediatePropertyNames = QString::fromUtf8(classInfo.value()).split(u',');

            // If the property contains an empty string, all properties shall be deferred.
            if (immediatePropertyNames.isEmpty())
                immediatePropertyNames.append(QString());
        }
    }

    for (QmlIR::Binding *binding = obj->firstBinding(); binding; binding = binding->next) {
        QString name = stringAt(binding->propertyNameIndex);

        if (customParser) {
            if (binding->type() == Binding::Type_AttachedProperty) {
                if (customParser->flags() & QQmlCustomParser::AcceptsAttachedProperties) {
                    binding->setFlag(Binding::IsCustomParserBinding);
                    obj->flags |= Object::HasCustomParserBindings;
                    continue;
                }
            } else if (QmlIR::IRBuilder::isSignalPropertyName(name)
                       && !(customParser->flags() & QQmlCustomParser::AcceptsSignalHandlers)) {
                obj->flags |= Object::HasCustomParserBindings;
                binding->setFlag(Binding::IsCustomParserBinding);
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

            const Binding::Flags bindingFlags = binding->flags();
            if (bindingFlags & Binding::IsSignalHandlerExpression
                        || bindingFlags & Binding::IsSignalHandlerObject
                        || bindingFlags & Binding::IsPropertyObserver) {
                // These signal handlers cannot be custom-parsed. We have already established
                // that the signal exists.
                return false;
            }

            // If the property isn't found, we may want to custom-parse the binding.
            obj->flags |= Object::HasCustomParserBindings;
            binding->setFlag(Binding::IsCustomParserBinding);
            return false;
        }();

        bool seenSubObjectWithId = false;
        bool isExternal = false;
        if (binding->type() >= Binding::Type_Object) {
            const bool isOwnProperty = hasPropertyData || binding->isAttachedProperty();
            isExternal = !isOwnProperty && binding->isGroupProperty();
            if (isOwnProperty || isExternal) {
                qSwap(_seenObjectWithId, seenSubObjectWithId);
                const bool subObjectValid = scanObject(
                            binding->value.objectIndex,
                            (isExternal || scopeDeferred == ScopeDeferred::True)
                                ? ScopeDeferred::True
                                : ScopeDeferred::False);
                qSwap(_seenObjectWithId, seenSubObjectWithId);
                if (!subObjectValid)
                    return false;
                _seenObjectWithId |= seenSubObjectWithId;
            }
        }

        bool isDeferred = false;
        if (!immediatePropertyNames.isEmpty() && !immediatePropertyNames.contains(name)) {
            if (seenSubObjectWithId) {
                COMPILE_EXCEPTION(binding, tr("You cannot assign an id to an object assigned "
                                              "to a deferred property."));
            }
            isDeferred = true;
        } else if (!deferredPropertyNames.isEmpty() && deferredPropertyNames.contains(name)) {
            if (!seenSubObjectWithId && binding->type() != Binding::Type_GroupProperty)
                isDeferred = true;
        }

        if (binding->type() >= Binding::Type_Object) {
            if (isExternal && !isDeferred && !customParser) {
                COMPILE_EXCEPTION(
                            binding, tr("Cannot assign to non-existent property \"%1\"").arg(name));
            }
        }

        if (isDeferred) {
            binding->setFlag(Binding::IsDeferredBinding);
            obj->flags |= Object::HasDeferredBindings;
        }
    }

    return true;
}

QQmlDefaultPropertyMerger::QQmlDefaultPropertyMerger(QQmlTypeCompiler *typeCompiler)
    : QQmlCompilePass(typeCompiler)
    , qmlObjects(*typeCompiler->qmlObjects())
    , propertyCaches(typeCompiler->propertyCaches())
{

}

void QQmlDefaultPropertyMerger::mergeDefaultProperties()
{
    for (int i = 0; i < qmlObjects.size(); ++i)
        mergeDefaultProperties(i);
}

void QQmlDefaultPropertyMerger::mergeDefaultProperties(int objectIndex)
{
    QQmlPropertyCache::ConstPtr propertyCache = propertyCaches->at(objectIndex);
    if (!propertyCache)
        return;

    QmlIR::Object *object = qmlObjects.at(objectIndex);

    QString defaultProperty = object->indexOfDefaultPropertyOrAlias != -1 ? propertyCache->parent()->defaultPropertyName() : propertyCache->defaultPropertyName();
    QmlIR::Binding *bindingsToReinsert = nullptr;
    QmlIR::Binding *tail = nullptr;

    QmlIR::Binding *previousBinding = nullptr;
    QmlIR::Binding *binding = object->firstBinding();
    while (binding) {
        if (binding->propertyNameIndex == quint32(0) || stringAt(binding->propertyNameIndex) != defaultProperty) {
            previousBinding = binding;
            binding = binding->next;
            continue;
        }

        QmlIR::Binding *toReinsert = binding;
        binding = object->unlinkBinding(previousBinding, binding);

        if (!tail) {
            bindingsToReinsert = toReinsert;
            tail = toReinsert;
        } else {
            tail->next = toReinsert;
            tail = tail->next;
        }
        tail->next = nullptr;
    }

    binding = bindingsToReinsert;
    while (binding) {
        QmlIR::Binding *toReinsert = binding;
        binding = binding->next;
        object->insertSorted(toReinsert);
    }
}

QT_END_NAMESPACE
