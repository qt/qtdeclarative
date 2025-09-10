// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmltypecompiler_p.h"

#include <private/qqmlobjectcreator_p.h>
#include <private/qqmlcustomparser_p.h>
#include <private/qqmlvmemetaobject_p.h>
#include <private/qqmlcomponent_p.h>
#include <private/qqmlpropertyresolver_p.h>
#include <private/qqmlcomponentandaliasresolver_p.h>
#include <private/qqmlsignalnames_p.h>

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
        QQmlEnginePrivate *engine, QQmlTypeData *typeData, QmlIR::Document *parsedQML,
        QV4::CompiledData::ResolvedTypeReferenceMap *resolvedTypeCache,
        const QV4::CompiledData::DependentTypesHasher &dependencyHasher)
    : resolvedTypes(resolvedTypeCache)
    , engine(engine)
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

                QQmlComponentAndAliasResolver resolver(this, enginePrivate(), &m_propertyCaches);
                if (QQmlError error = resolver.resolve(result.processedRoot); error.isValid()) {
                    recordError(error);
                    return nullptr;
                }
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

int QQmlTypeCompiler::registerConstant(QV4::ReturnedValue v)
{
    return document->jsGenerator.registerConstant(v);
}

const QV4::CompiledData::Unit *QQmlTypeCompiler::qmlUnit() const
{
    return document->javaScriptCompilationUnit->unitData();
}

const QQmlImports *QQmlTypeCompiler::imports() const
{
    return typeData->imports();
}

QVector<QmlIR::Object *> *QQmlTypeCompiler::qmlObjects() const
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
            if (!type.isValid()) {
                imports->resolveType(
                        QQmlTypeLoader::get(enginePrivate), bindingPropertyName, &type, nullptr,
                        nullptr);
            }

            const QMetaObject *attachedType = type.attachedPropertiesType(enginePrivate);
            if (!attachedType)
                COMPILE_EXCEPTION(binding, tr("Non-existent attached object"));
            QQmlPropertyCache::ConstPtr cache = QQmlMetaType::propertyCache(attachedType);
            if (!resolveSignalHandlerExpressions(attachedObj, bindingPropertyName, cache))
                return false;
            continue;
        }

        QString qPropertyName;
        QString signalName;
        if (auto propertyName =
                    QQmlSignalNames::changedHandlerNameToPropertyName(bindingPropertyName)) {
            qPropertyName = *propertyName;
            signalName = *QQmlSignalNames::changedHandlerNameToSignalName(bindingPropertyName);
        } else {
            signalName = QQmlSignalNames::handlerNameToSignalName(bindingPropertyName)
                                 .value_or(QString());
        }
        if (signalName.isEmpty())
            continue;

        QQmlPropertyResolver resolver(propertyCache);

        bool notInRevision = false;
        const QQmlPropertyData * const signal = resolver.signal(signalName, &notInRevision);
        const QQmlPropertyData * const signalPropertyData = resolver.property(signalName, /*notInRevision ptr*/nullptr);
        const QQmlPropertyData * const qPropertyData = !qPropertyName.isEmpty() ? resolver.property(qPropertyName) : nullptr;
        QString finalSignalHandlerPropertyName = signalName;
        QV4::CompiledData::Binding::Flag flag
                = QV4::CompiledData::Binding::IsSignalHandlerExpression;

        const bool isPropertyObserver
                = !signalPropertyData && qPropertyData && qPropertyData->notifiesViaBindable();
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
    imports->resolveType(
            QQmlTypeLoader::get(compiler->enginePrivate()), typeName, &type, nullptr, nullptr);

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
            if (!scopedEnumName.isEmpty()) {
                value = type.scopedEnumValue(
                        compiler->typeLoader(), scopedEnumName, enumValue, &ok);
            } else {
                value = type.enumValue(compiler->typeLoader(), QHashedStringRef(enumValue), &ok);
            }
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
        imports->resolveType(compiler->typeLoader(), scope, &type, nullptr, nullptr);
        if (!type.isValid())
            return -1;
        if (!enumName.isEmpty())
            return type.scopedEnumValue(compiler->typeLoader(), enumName, enumValue, ok);
        return type.enumValue(
                compiler->typeLoader(),
                QHashedStringRef(enumValue.constData(), enumValue.size()), ok);
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
bool QQmlComponentAndAliasResolver<QQmlTypeCompiler>::wrapImplicitComponent(QmlIR::Binding *binding)
{
    QQmlJS::MemoryPool *pool = m_compiler->memoryPool();
    QVector<QmlIR::Object *> *qmlObjects = m_compiler->qmlObjects();

    // emulate "import QML 1.0" and then wrap the component in "QML.Component {}"
    QQmlType componentType = QQmlMetaType::qmlType(
                &QQmlComponent::staticMetaObject, QStringLiteral("QML"),
                QTypeRevision::fromVersion(1, 0));
    Q_ASSERT(componentType.isValid());
    const QString qualifier = QStringLiteral("QML");

    m_compiler->addImport(componentType.module(), qualifier, componentType.version());

    QmlIR::Object *syntheticComponent = pool->New<QmlIR::Object>();
    syntheticComponent->init(
                pool,
                m_compiler->registerString(
                    qualifier + QLatin1Char('.') + componentType.elementName()),
                m_compiler->registerString(QString()), binding->valueLocation);
    syntheticComponent->flags |= QV4::CompiledData::Object::IsComponent;

    if (!m_compiler->resolvedTypes->contains(syntheticComponent->inheritedTypeNameIndex)) {
        auto typeRef = new QV4::ResolvedTypeReference;
        typeRef->setType(componentType);
        typeRef->setVersion(componentType.version());
        m_compiler->resolvedTypes->insert(syntheticComponent->inheritedTypeNameIndex, typeRef);
    }

    qmlObjects->append(syntheticComponent);
    const int componentIndex = qmlObjects->size() - 1;
    // Keep property caches symmetric
    QQmlPropertyCache::ConstPtr componentCache
            = QQmlMetaType::propertyCache(&QQmlComponent::staticMetaObject);
    m_propertyCaches->append(componentCache);

    QmlIR::Binding *syntheticBinding = pool->New<QmlIR::Binding>();
    *syntheticBinding = *binding;

    // The synthetic binding inside Component has no name. It's just "Component { Foo {} }".
    syntheticBinding->propertyNameIndex = 0;

    syntheticBinding->setType(QV4::CompiledData::Binding::Type_Object);
    QString error = syntheticComponent->appendBinding(syntheticBinding, /*isListBinding*/false);
    Q_ASSERT(error.isEmpty());
    Q_UNUSED(error);

    binding->value.objectIndex = componentIndex;

    m_componentRoots.append(componentIndex);
    return true;
}

template<>
void QQmlComponentAndAliasResolver<QQmlTypeCompiler>::resolveGeneralizedGroupProperty(
        const CompiledObject &component, CompiledBinding *binding)
{
    Q_UNUSED(component);
    // We cannot make it fail here. It might be a custom-parsed property
    const int targetObjectIndex = m_idToObjectIndex.value(binding->propertyNameIndex, -1);
    if (targetObjectIndex != -1)
        m_propertyCaches->set(binding->value.objectIndex, m_propertyCaches->at(targetObjectIndex));
}

template<>
typename QQmlComponentAndAliasResolver<QQmlTypeCompiler>::AliasResolutionResult
QQmlComponentAndAliasResolver<QQmlTypeCompiler>::resolveAliasesInObject(
        const CompiledObject &component, int objectIndex, QQmlError *error)
{
    Q_UNUSED(component);

    const QmlIR::Object * const obj = m_compiler->objectAt(objectIndex);
    if (!obj->aliasCount())
        return AllAliasesResolved;

    int numResolvedAliases = 0;
    bool seenUnresolvedAlias = false;

    for (QmlIR::Alias *alias = obj->firstAlias(); alias; alias = alias->next) {
        if (alias->hasFlag(QV4::CompiledData::Alias::Resolved))
            continue;

        seenUnresolvedAlias = true;

        const int idIndex = alias->idIndex();
        const int targetObjectIndex = m_idToObjectIndex.value(idIndex, -1);
        if (targetObjectIndex == -1) {
            *error = qQmlCompileError(
                    alias->referenceLocation,
                    QQmlComponentAndAliasResolverBase::tr("Invalid alias reference. Unable to find id \"%1\"").arg(stringAt(idIndex)));
            break;
        }

        const QmlIR::Object *targetObject = m_compiler->objectAt(targetObjectIndex);
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
            QQmlPropertyCache::ConstPtr targetCache = m_propertyCaches->at(targetObjectIndex);
            if (!targetCache) {
                *error = qQmlCompileError(
                        alias->referenceLocation,
                        QQmlComponentAndAliasResolverBase::tr("Invalid alias target location: %1").arg(property.toString()));
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
                        QQmlComponentAndAliasResolverBase::tr("Invalid alias target location: %1").arg(property.toString()));
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
                            if (m_compiler->stringAt(binding.propertyNameIndex) == property) {
                                resolver = QQmlPropertyResolver(m_propertyCaches->at(binding.value.objectIndex));
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
                                QQmlComponentAndAliasResolverBase::tr("Invalid alias target location: %1").arg(subProperty.toString()));
                        break;
                    }
                } else {

                    int valueTypeIndex =
                            valueTypeMetaObject->indexOfProperty(subProperty.toString().toUtf8().constData());
                    if (valueTypeIndex == -1) {
                        *error = qQmlCompileError(
                                alias->referenceLocation,
                                QQmlComponentAndAliasResolverBase::tr("Invalid alias target location: %1").arg(subProperty.toString()));
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
            } else if (QQmlSignalNames::isHandlerName(name)
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
            if (isExternal || !disableInternalDeferredProperties())
                isDeferred = true;
        } else if (!deferredPropertyNames.isEmpty() && deferredPropertyNames.contains(name)) {
            if (!seenSubObjectWithId && binding->type() != Binding::Type_GroupProperty) {
                if (isExternal || !disableInternalDeferredProperties())
                    isDeferred = true;
            }
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
