/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qqmltypecompiler_p.h"

#include <private/qqmlcompiler_p.h>
#include <private/qqmlobjectcreator_p.h>
#include <private/qqmlcustomparser_p.h>
#include <private/qqmlvmemetaobject_p.h>
#include <private/qqmlcomponent_p.h>

#define COMPILE_EXCEPTION(token, desc) \
    { \
        recordError((token)->location, desc); \
        return false; \
    }

QT_BEGIN_NAMESPACE

QQmlTypeCompiler::QQmlTypeCompiler(QQmlEnginePrivate *engine, QQmlCompiledData *compiledData, QQmlTypeData *typeData, QtQml::ParsedQML *parsedQML)
    : engine(engine)
    , compiledData(compiledData)
    , typeData(typeData)
    , parsedQML(parsedQML)
{
}

bool QQmlTypeCompiler::compile()
{
    compiledData->importCache = new QQmlTypeNameCache;

    foreach (const QString &ns, typeData->namespaces())
        compiledData->importCache->add(ns);

    // Add any Composite Singletons that were used to the import cache
    foreach (const QQmlTypeData::TypeReference &singleton, typeData->compositeSingletons())
        compiledData->importCache->add(singleton.type->qmlTypeName(), singleton.type->sourceUrl(), singleton.prefix);

    typeData->imports().populateCache(compiledData->importCache);
    compiledData->importCache->addref();

    const QHash<int, QQmlTypeData::TypeReference> &resolvedTypes = typeData->resolvedTypeRefs();
    for (QHash<int, QQmlTypeData::TypeReference>::ConstIterator resolvedType = resolvedTypes.constBegin(), end = resolvedTypes.constEnd();
         resolvedType != end; ++resolvedType) {
        QQmlCompiledData::TypeReference ref;
        if (resolvedType->typeData) {
            ref.component = resolvedType->typeData->compiledData();
            ref.component->addref();
        } else {
            ref.type = resolvedType->type;
            Q_ASSERT(ref.type);
        }
        ref.majorVersion = resolvedType->majorVersion;
        ref.minorVersion = resolvedType->minorVersion;
        compiledData->resolvedTypes.insert(resolvedType.key(), ref);
    }

    // Build property caches and VME meta object data

    const int objectCount = parsedQML->objects.count();
    compiledData->datas.reserve(objectCount);
    compiledData->propertyCaches.reserve(objectCount);

    {
        QQmlPropertyCacheCreator propertyCacheBuilder(this);
        if (!propertyCacheBuilder.buildMetaObjects())
            return false;
    }

    {
        SignalHandlerConverter converter(engine, parsedQML, compiledData);
        if (!converter.convertSignalHandlerExpressionsToFunctionDeclarations()) {
            errors << converter.errors;
            return false;
        }
    }

    // Collect imported scripts
    const QList<QQmlTypeData::ScriptReference> &scripts = typeData->resolvedScripts();
    compiledData->scripts.reserve(scripts.count());
    for (int scriptIndex = 0; scriptIndex < scripts.count(); ++scriptIndex) {
        const QQmlTypeData::ScriptReference &script = scripts.at(scriptIndex);

        QString qualifier = script.qualifier;
        QString enclosingNamespace;

        const int lastDotIndex = qualifier.lastIndexOf(QLatin1Char('.'));
        if (lastDotIndex != -1) {
            enclosingNamespace = qualifier.left(lastDotIndex);
            qualifier = qualifier.mid(lastDotIndex+1);
        }

        compiledData->importCache->add(qualifier, scriptIndex, enclosingNamespace);
        QQmlScriptData *scriptData = script.script->scriptData();
        scriptData->addref();
        compiledData->scripts << scriptData;
    }

    // Resolve component boundaries and aliases

    {
        // Scan for components, determine their scopes and resolve aliases within the scope.
        QQmlComponentAndAliasResolver resolver(this);
        if (!resolver.resolve())
            return false;
    }

    // Compile JS binding expressions and signal handlers

    JSCodeGen jsCodeGen(typeData->finalUrlString(), parsedQML->code, &parsedQML->jsModule, &parsedQML->jsParserEngine, parsedQML->program, compiledData->importCache);
    const QVector<int> runtimeFunctionIndices = jsCodeGen.generateJSCodeForFunctionsAndBindings(parsedQML->functions);

    QV4::ExecutionEngine *v4 = engine->v4engine();

    QScopedPointer<QQmlJS::EvalInstructionSelection> isel(v4->iselFactory->create(engine, v4->executableAllocator, &parsedQML->jsModule, &parsedQML->jsGenerator));
    isel->setUseFastLookups(false);
    QV4::CompiledData::CompilationUnit *jsUnit = isel->compile(/*generated unit data*/false);

    // Generate QML compiled type data structures

    QmlUnitGenerator qmlGenerator;
    QV4::CompiledData::QmlUnit *qmlUnit = qmlGenerator.generate(*parsedQML, runtimeFunctionIndices);

    if (jsUnit) {
        Q_ASSERT(!jsUnit->data);
        jsUnit->ownsData = false;
        jsUnit->data = &qmlUnit->header;
    }

    compiledData->compilationUnit = jsUnit;
    if (compiledData->compilationUnit)
        compiledData->compilationUnit->ref();
    compiledData->qmlUnit = qmlUnit; // ownership transferred to m_compiledData

    // Add to type registry of composites
    if (compiledData->isCompositeType())
        engine->registerInternalCompositeType(compiledData);
    else {
        const QV4::CompiledData::Object *obj = qmlUnit->objectAt(qmlUnit->indexOfRootObject);
        QQmlCompiledData::TypeReference typeRef = compiledData->resolvedTypes.value(obj->inheritedTypeNameIndex);
        if (typeRef.component) {
            compiledData->metaTypeId = typeRef.component->metaTypeId;
            compiledData->listMetaTypeId = typeRef.component->listMetaTypeId;
        } else {
            compiledData->metaTypeId = typeRef.type->typeId();
            compiledData->listMetaTypeId = typeRef.type->qListTypeId();
        }
    }

    // Sanity check property bindings
    QQmlPropertyValidator validator(this);
    if (!validator.validate())
        return false;

    return errors.isEmpty();
}

void QQmlTypeCompiler::recordError(const QQmlError &error)
{
    QQmlError e = error;
    e.setUrl(compiledData->url);
    errors << e;
}

QString QQmlTypeCompiler::stringAt(int idx) const
{
    return parsedQML->stringAt(idx);
}

int QQmlTypeCompiler::registerString(const QString &str)
{
    return parsedQML->jsGenerator.registerString(str);
}

const QV4::CompiledData::QmlUnit *QQmlTypeCompiler::qmlUnit() const
{
    return compiledData->qmlUnit;
}

const QQmlImports *QQmlTypeCompiler::imports() const
{
    return &typeData->imports();
}

QHash<int, QQmlCompiledData::TypeReference> *QQmlTypeCompiler::resolvedTypes()
{
    return &compiledData->resolvedTypes;
}

QList<QmlObject *> *QQmlTypeCompiler::qmlObjects()
{
    return &parsedQML->objects;
}

int QQmlTypeCompiler::rootObjectIndex() const
{
    return parsedQML->indexOfRootObject;
}

void QQmlTypeCompiler::setPropertyCaches(const QVector<QQmlPropertyCache *> &caches)
{
    Q_ASSERT(compiledData->propertyCaches.isEmpty());
    compiledData->propertyCaches = caches;
    Q_ASSERT(caches.count() >= parsedQML->indexOfRootObject);
    compiledData->rootPropertyCache = caches.at(parsedQML->indexOfRootObject);
}

const QVector<QQmlPropertyCache *> &QQmlTypeCompiler::propertyCaches() const
{
    return compiledData->propertyCaches;
}

void QQmlTypeCompiler::setVMEMetaObjects(const QVector<QByteArray> &metaObjects)
{
    Q_ASSERT(compiledData->datas.isEmpty());
    compiledData->datas = metaObjects;
}

QVector<QByteArray> *QQmlTypeCompiler::vmeMetaObjects() const
{
    return &compiledData->datas;
}

QHash<int, int> *QQmlTypeCompiler::objectIndexToIdForRoot()
{
    return &compiledData->objectIndexToIdForRoot;
}

QHash<int, QHash<int, int> > *QQmlTypeCompiler::objectIndexToIdPerComponent()
{
    return &compiledData->objectIndexToIdPerComponent;
}

QHash<int, QByteArray> *QQmlTypeCompiler::customParserData()
{
    return &compiledData->customParserData;
}

MemoryPool *QQmlTypeCompiler::memoryPool()
{
    return parsedQML->jsParserEngine.pool();
}

QQmlCompilePass::QQmlCompilePass(QQmlTypeCompiler *typeCompiler)
    : compiler(typeCompiler)
{
}

void QQmlCompilePass::recordError(const QV4::CompiledData::Location &location, const QString &description)
{
    QQmlError error;
    error.setLine(location.line);
    error.setColumn(location.column);
    error.setDescription(description);
    compiler->recordError(error);
}

static QAtomicInt classIndexCounter(0);

QQmlPropertyCacheCreator::QQmlPropertyCacheCreator(QQmlTypeCompiler *typeCompiler)
    : QQmlCompilePass(typeCompiler)
    , enginePrivate(typeCompiler->enginePrivate())
    , qmlObjects(*typeCompiler->qmlObjects())
    , imports(typeCompiler->imports())
    , resolvedTypes(typeCompiler->resolvedTypes())
{
}

QQmlPropertyCacheCreator::~QQmlPropertyCacheCreator()
{
    for (int i = 0; i < propertyCaches.count(); ++i)
        if (QQmlPropertyCache *cache = propertyCaches.at(i))
            cache->release();
    propertyCaches.clear();
}

bool QQmlPropertyCacheCreator::buildMetaObjects()
{
    propertyCaches.resize(qmlObjects.count());
    vmeMetaObjects.resize(qmlObjects.count());

    if (!buildMetaObjectRecursively(compiler->rootObjectIndex(), /*referencing object*/-1, /*instantiating binding*/0))
        return false;

    compiler->setVMEMetaObjects(vmeMetaObjects);
    compiler->setPropertyCaches(propertyCaches);
    propertyCaches.clear();

    return true;
}

bool QQmlPropertyCacheCreator::buildMetaObjectRecursively(int objectIndex, int referencingObjectIndex, const QV4::CompiledData::Binding *instantiatingBinding)
{
    const QmlObject *obj = qmlObjects.at(objectIndex);

    QQmlPropertyCache *baseTypeCache = 0;

    bool needVMEMetaObject = obj->properties->count != 0 || obj->qmlSignals->count != 0 || obj->functions->count != 0;
    if (!needVMEMetaObject) {
        for (const QtQml::Binding *binding = obj->bindings->first; binding; binding = binding->next) {
            if (binding->type == QV4::CompiledData::Binding::Type_Object && (binding->flags & QV4::CompiledData::Binding::IsOnAssignment)) {

                // On assignments are implemented using value interceptors, which require a VME meta object.
                needVMEMetaObject = true;

                // If the on assignment is inside a group property, we need to distinguish between QObject based
                // group properties and value type group properties. For the former the base type is derived from
                // the property that references us, for the latter we only need a meta-object on the referencing object
                // because interceptors can't go to the shared value type instances.
                if (instantiatingBinding && instantiatingBinding->type == QV4::CompiledData::Binding::Type_GroupProperty) {
                    QQmlPropertyCache *parentCache = propertyCaches.at(referencingObjectIndex);
                    Q_ASSERT(parentCache);
                    Q_ASSERT(!stringAt(instantiatingBinding->propertyNameIndex).isEmpty());

                    bool notInRevision = false;
                    QQmlPropertyData *pd = PropertyResolver(parentCache).property(stringAt(instantiatingBinding->propertyNameIndex), &notInRevision);
                    Q_ASSERT(pd);
                    if (QQmlValueTypeFactory::isValueType(pd->propType)) {
                        needVMEMetaObject = false;
                        if (!ensureMetaObject(referencingObjectIndex))
                            return false;
                    } else if (pd->isQObject()) {
                        baseTypeCache = enginePrivate->rawPropertyCacheForType(pd->propType);
                        Q_ASSERT(baseTypeCache);
                    }
                }
                break;
            }
        }
    }

    QString typeName = stringAt(obj->inheritedTypeNameIndex);
    if (!typeName.isEmpty()) {
        QQmlCompiledData::TypeReference typeRef = resolvedTypes->value(obj->inheritedTypeNameIndex);
        baseTypeCache = typeRef.createPropertyCache(QQmlEnginePrivate::get(enginePrivate));
        Q_ASSERT(baseTypeCache);
    }

    if (needVMEMetaObject) {
        if (!createMetaObject(objectIndex, obj, baseTypeCache))
            return false;
    } else if (baseTypeCache) {
        propertyCaches[objectIndex] = baseTypeCache;
        baseTypeCache->addref();
    }

    for (const QtQml::Binding *binding = obj->bindings->first; binding; binding = binding->next)
        if (binding->type >= QV4::CompiledData::Binding::Type_Object)
            if (!buildMetaObjectRecursively(binding->value.objectIndex, objectIndex, binding))
                return false;

    return true;
}

bool QQmlPropertyCacheCreator::ensureMetaObject(int objectIndex)
{
    if (!vmeMetaObjects.at(objectIndex).isEmpty())
        return true;
    const QtQml::QmlObject *obj = qmlObjects.at(objectIndex);
    QQmlCompiledData::TypeReference typeRef = resolvedTypes->value(obj->inheritedTypeNameIndex);
    QQmlPropertyCache *baseTypeCache = typeRef.createPropertyCache(QQmlEnginePrivate::get(enginePrivate));
    return createMetaObject(objectIndex, obj, baseTypeCache);
}

bool QQmlPropertyCacheCreator::createMetaObject(int objectIndex, const QtQml::QmlObject *obj, QQmlPropertyCache *baseTypeCache)
{
    QQmlPropertyCache *cache = baseTypeCache->copyAndReserve(QQmlEnginePrivate::get(enginePrivate),
                                                             obj->properties->count,
                                                             obj->functions->count + obj->properties->count + obj->qmlSignals->count,
                                                             obj->qmlSignals->count + obj->properties->count);
    propertyCaches[objectIndex] = cache;
    cache->addref();

    struct TypeData {
        QV4::CompiledData::Property::Type dtype;
        int metaType;
    } builtinTypes[] = {
        { QV4::CompiledData::Property::Var, qMetaTypeId<QJSValue>() },
        { QV4::CompiledData::Property::Variant, QMetaType::QVariant },
        { QV4::CompiledData::Property::Int, QMetaType::Int },
        { QV4::CompiledData::Property::Bool, QMetaType::Bool },
        { QV4::CompiledData::Property::Real, QMetaType::Double },
        { QV4::CompiledData::Property::String, QMetaType::QString },
        { QV4::CompiledData::Property::Url, QMetaType::QUrl },
        { QV4::CompiledData::Property::Color, QMetaType::QColor },
        { QV4::CompiledData::Property::Font, QMetaType::QFont },
        { QV4::CompiledData::Property::Time, QMetaType::QTime },
        { QV4::CompiledData::Property::Date, QMetaType::QDate },
        { QV4::CompiledData::Property::DateTime, QMetaType::QDateTime },
        { QV4::CompiledData::Property::Rect, QMetaType::QRectF },
        { QV4::CompiledData::Property::Point, QMetaType::QPointF },
        { QV4::CompiledData::Property::Size, QMetaType::QSizeF },
        { QV4::CompiledData::Property::Vector2D, QMetaType::QVector2D },
        { QV4::CompiledData::Property::Vector3D, QMetaType::QVector3D },
        { QV4::CompiledData::Property::Vector4D, QMetaType::QVector4D },
        { QV4::CompiledData::Property::Matrix4x4, QMetaType::QMatrix4x4 },
        { QV4::CompiledData::Property::Quaternion, QMetaType::QQuaternion }
    };
    static const uint builtinTypeCount = sizeof(builtinTypes) / sizeof(TypeData);

    QByteArray newClassName;

    if (false /* ### compileState->root == obj && !compileState->nested*/) {
#if 0 // ###
        QString path = output->url.path();
        int lastSlash = path.lastIndexOf(QLatin1Char('/'));
        if (lastSlash > -1) {
            QString nameBase = path.mid(lastSlash + 1, path.length()-lastSlash-5);
            if (!nameBase.isEmpty() && nameBase.at(0).isUpper())
                newClassName = nameBase.toUtf8() + "_QMLTYPE_" +
                               QByteArray::number(classIndexCounter.fetchAndAddRelaxed(1));
        }
#endif
    }
    if (newClassName.isEmpty()) {
        newClassName = QQmlMetaObject(baseTypeCache).className();
        newClassName.append("_QML_");
        newClassName.append(QByteArray::number(classIndexCounter.fetchAndAddRelaxed(1)));
    }

    cache->_dynamicClassName = newClassName;

    int aliasCount = 0;
    int varPropCount = 0;

    for (QtQml::QmlProperty *p = obj->properties->first; p; p = p->next) {
        if (p->type == QV4::CompiledData::Property::Alias)
            aliasCount++;
        else if (p->type == QV4::CompiledData::Property::Var)
            varPropCount++;

#if 0 // ### Do this elsewhere
        // No point doing this for both the alias and non alias cases
        QQmlPropertyData *d = property(obj, p->name);
        if (d && d->isFinal())
            COMPILE_EXCEPTION(p, tr("Cannot override FINAL property"));
#endif
    }

    typedef QQmlVMEMetaData VMD;

    QByteArray &dynamicData = vmeMetaObjects[objectIndex] = QByteArray(sizeof(QQmlVMEMetaData)
                                                              + obj->properties->count * sizeof(VMD::PropertyData)
                                                              + obj->functions->count * sizeof(VMD::MethodData)
                                                              + aliasCount * sizeof(VMD::AliasData), 0);

    int effectivePropertyIndex = cache->propertyIndexCacheStart;
    int effectiveMethodIndex = cache->methodIndexCacheStart;

    // For property change signal override detection.
    // We prepopulate a set of signal names which already exist in the object,
    // and throw an error if there is a signal/method defined as an override.
    QSet<QString> seenSignals;
    seenSignals << QStringLiteral("destroyed") << QStringLiteral("parentChanged") << QStringLiteral("objectNameChanged");
    QQmlPropertyCache *parentCache = cache;
    while ((parentCache = parentCache->parent())) {
        if (int pSigCount = parentCache->signalCount()) {
            int pSigOffset = parentCache->signalOffset();
            for (int i = pSigOffset; i < pSigCount; ++i) {
                QQmlPropertyData *currPSig = parentCache->signal(i);
                // XXX TODO: find a better way to get signal name from the property data :-/
                for (QQmlPropertyCache::StringCache::ConstIterator iter = parentCache->stringCache.begin();
                        iter != parentCache->stringCache.end(); ++iter) {
                    if (currPSig == (*iter).second) {
                        seenSignals.insert(iter.key());
                        break;
                    }
                }
            }
        }
    }

    // First set up notify signals for properties - first normal, then var, then alias
    enum { NSS_Normal = 0, NSS_Var = 1, NSS_Alias = 2 };
    for (int ii = NSS_Normal; ii <= NSS_Alias; ++ii) { // 0 == normal, 1 == var, 2 == alias

        if (ii == NSS_Var && varPropCount == 0) continue;
        else if (ii == NSS_Alias && aliasCount == 0) continue;

        for (QtQml::QmlProperty *p = obj->properties->first; p; p = p->next) {
            if ((ii == NSS_Normal && (p->type == QV4::CompiledData::Property::Alias ||
                                      p->type == QV4::CompiledData::Property::Var)) ||
                ((ii == NSS_Var) && (p->type != QV4::CompiledData::Property::Var)) ||
                ((ii == NSS_Alias) && (p->type != QV4::CompiledData::Property::Alias)))
                continue;

            quint32 flags = QQmlPropertyData::IsSignal | QQmlPropertyData::IsFunction |
                            QQmlPropertyData::IsVMESignal;

            QString changedSigName = stringAt(p->nameIndex) + QLatin1String("Changed");
            seenSignals.insert(changedSigName);

            cache->appendSignal(changedSigName, flags, effectiveMethodIndex++);
        }
    }

    // Dynamic signals
    for (QtQml::Signal *s = obj->qmlSignals->first; s; s = s->next) {
        const int paramCount = s->parameters->count;

        QList<QByteArray> names;
        QVarLengthArray<int, 10> paramTypes(paramCount?(paramCount + 1):0);

        if (paramCount) {
            paramTypes[0] = paramCount;

            QtQml::SignalParameter *param = s->parameters->first;
            for (int i = 0; i < paramCount; ++i, param = param->next) {
                names.append(stringAt(param->nameIndex).toUtf8());
                if (param->type < builtinTypeCount) {
                    // built-in type
                    paramTypes[i + 1] = builtinTypes[param->type].metaType;
                } else {
                    // lazily resolved type
                    Q_ASSERT(param->type == QV4::CompiledData::Property::Custom);
                    const QString customTypeName = stringAt(param->customTypeNameIndex);
                    QQmlType *qmltype = 0;
                    if (!imports->resolveType(customTypeName, &qmltype, 0, 0, 0))
                        COMPILE_EXCEPTION(s, tr("Invalid signal parameter type: %1").arg(customTypeName));

                    if (qmltype->isComposite()) {
                        QQmlTypeData *tdata = enginePrivate->typeLoader.getType(qmltype->sourceUrl());
                        Q_ASSERT(tdata);
                        Q_ASSERT(tdata->isComplete());

                        QQmlCompiledData *data = tdata->compiledData();

                        paramTypes[i + 1] = data->metaTypeId;

                        tdata->release();
                    } else {
                        paramTypes[i + 1] = qmltype->typeId();
                    }
                }
            }
        }

        ((QQmlVMEMetaData *)dynamicData.data())->signalCount++;

        quint32 flags = QQmlPropertyData::IsSignal | QQmlPropertyData::IsFunction |
                        QQmlPropertyData::IsVMESignal;
        if (paramCount)
            flags |= QQmlPropertyData::HasArguments;

        QString signalName = stringAt(s->nameIndex);
        if (seenSignals.contains(signalName))
            COMPILE_EXCEPTION(s, tr("Duplicate signal name: invalid override of property change signal or superclass signal"));
        seenSignals.insert(signalName);

        cache->appendSignal(signalName, flags, effectiveMethodIndex++,
                            paramCount?paramTypes.constData():0, names);
    }


    // Dynamic slots
    for (QtQml::Function *s = obj->functions->first; s; s = s->next) {
        AST::FunctionDeclaration *astFunction = s->functionDeclaration;

        quint32 flags = QQmlPropertyData::IsFunction | QQmlPropertyData::IsVMEFunction;

        if (astFunction->formals)
            flags |= QQmlPropertyData::HasArguments;

        QString slotName = astFunction->name.toString();
        if (seenSignals.contains(slotName))
            COMPILE_EXCEPTION(s, tr("Duplicate method name: invalid override of property change signal or superclass signal"));
        // Note: we don't append slotName to the seenSignals list, since we don't
        // protect against overriding change signals or methods with properties.

        QList<QByteArray> parameterNames;
        AST::FormalParameterList *param = astFunction->formals;
        while (param) {
            parameterNames << param->name.toUtf8();
            param = param->next;
        }

        cache->appendMethod(slotName, flags, effectiveMethodIndex++, parameterNames);
    }


    // Dynamic properties (except var and aliases)
    int effectiveSignalIndex = cache->signalHandlerIndexCacheStart;
    int propertyIdx = 0;
    for (QtQml::QmlProperty *p = obj->properties->first; p; p = p->next, ++propertyIdx) {

        if (p->type == QV4::CompiledData::Property::Alias ||
            p->type == QV4::CompiledData::Property::Var)
            continue;

        int propertyType = 0;
        int vmePropertyType = 0;
        quint32 propertyFlags = 0;

        if (p->type < builtinTypeCount) {
            propertyType = builtinTypes[p->type].metaType;
            vmePropertyType = propertyType;

            if (p->type == QV4::CompiledData::Property::Variant)
                propertyFlags |= QQmlPropertyData::IsQVariant;
        } else {
            Q_ASSERT(p->type == QV4::CompiledData::Property::CustomList ||
                     p->type == QV4::CompiledData::Property::Custom);

            QQmlType *qmltype = 0;
            if (!imports->resolveType(stringAt(p->customTypeNameIndex), &qmltype, 0, 0, 0)) {
                COMPILE_EXCEPTION(p, tr("Invalid property type"));
            }

            Q_ASSERT(qmltype);
            if (qmltype->isComposite()) {
                QQmlTypeData *tdata = enginePrivate->typeLoader.getType(qmltype->sourceUrl());
                Q_ASSERT(tdata);
                Q_ASSERT(tdata->isComplete());

                QQmlCompiledData *data = tdata->compiledData();

                if (p->type == QV4::CompiledData::Property::Custom) {
                    propertyType = data->metaTypeId;
                    vmePropertyType = QMetaType::QObjectStar;
                } else {
                    propertyType = data->listMetaTypeId;
                    vmePropertyType = qMetaTypeId<QQmlListProperty<QObject> >();
                }

                tdata->release();
            } else {
                if (p->type == QV4::CompiledData::Property::Custom) {
                    propertyType = qmltype->typeId();
                    vmePropertyType = QMetaType::QObjectStar;
                } else {
                    propertyType = qmltype->qListTypeId();
                    vmePropertyType = qMetaTypeId<QQmlListProperty<QObject> >();
                }
            }

            if (p->type == QV4::CompiledData::Property::Custom)
                propertyFlags |= QQmlPropertyData::IsQObjectDerived;
            else
                propertyFlags |= QQmlPropertyData::IsQList;
        }

        if ((!p->flags & QV4::CompiledData::Property::IsReadOnly) && p->type != QV4::CompiledData::Property::CustomList)
            propertyFlags |= QQmlPropertyData::IsWritable;


        QString propertyName = stringAt(p->nameIndex);
        if (propertyIdx == obj->indexOfDefaultProperty) cache->_defaultPropertyName = propertyName;
        cache->appendProperty(propertyName, propertyFlags, effectivePropertyIndex++,
                              propertyType, effectiveSignalIndex);

        effectiveSignalIndex++;

        VMD *vmd = (QQmlVMEMetaData *)dynamicData.data();
        (vmd->propertyData() + vmd->propertyCount)->propertyType = vmePropertyType;
        vmd->propertyCount++;
    }

    // Now do var properties
    propertyIdx = 0;
    for (QtQml::QmlProperty *p = obj->properties->first; p; p = p->next, ++propertyIdx) {

        if (p->type != QV4::CompiledData::Property::Var)
            continue;

        quint32 propertyFlags = QQmlPropertyData::IsVarProperty;
        if (!p->flags & QV4::CompiledData::Property::IsReadOnly)
            propertyFlags |= QQmlPropertyData::IsWritable;

        VMD *vmd = (QQmlVMEMetaData *)dynamicData.data();
        (vmd->propertyData() + vmd->propertyCount)->propertyType = QMetaType::QVariant;
        vmd->propertyCount++;
        ((QQmlVMEMetaData *)dynamicData.data())->varPropertyCount++;

        QString propertyName = stringAt(p->nameIndex);
        if (propertyIdx == obj->indexOfDefaultProperty) cache->_defaultPropertyName = propertyName;
        cache->appendProperty(propertyName, propertyFlags, effectivePropertyIndex++,
                              QMetaType::QVariant, effectiveSignalIndex);

        effectiveSignalIndex++;
    }

    // Alias property count.  Actual data is setup in buildDynamicMetaAliases
    ((QQmlVMEMetaData *)dynamicData.data())->aliasCount = aliasCount;

    // Dynamic slot data - comes after the property data
    for (QtQml::Function *s = obj->functions->first; s; s = s->next) {
        AST::FunctionDeclaration *astFunction = s->functionDeclaration;
        int formalsCount = 0;
        AST::FormalParameterList *param = astFunction->formals;
        while (param) {
            formalsCount++;
            param = param->next;
        }

        VMD::MethodData methodData = { /* runtimeFunctionIndex*/ 0, // ###
                                       formalsCount,
                                       /* s->location.start.line */0 }; // ###

        VMD *vmd = (QQmlVMEMetaData *)dynamicData.data();
        VMD::MethodData &md = *(vmd->methodData() + vmd->methodCount);
        vmd->methodCount++;
        md = methodData;
    }

    return true;
}

QQmlComponentAndAliasResolver::QQmlComponentAndAliasResolver(QQmlTypeCompiler *typeCompiler)
    : QQmlCompilePass(typeCompiler)
    , enginePrivate(typeCompiler->enginePrivate())
    , pool(typeCompiler->memoryPool())
    , qmlObjects(typeCompiler->qmlObjects())
    , indexOfRootObject(typeCompiler->rootObjectIndex())
    , _componentIndex(-1)
    , _objectIndexToIdInScope(0)
    , resolvedTypes(typeCompiler->resolvedTypes())
    , propertyCaches(typeCompiler->propertyCaches())
    , vmeMetaObjectData(typeCompiler->vmeMetaObjects())
    , objectIndexToIdForRoot(typeCompiler->objectIndexToIdForRoot())
    , objectIndexToIdPerComponent(typeCompiler->objectIndexToIdPerComponent())
{
}

void QQmlComponentAndAliasResolver::findAndRegisterImplicitComponents(const QtQml::QmlObject *obj, int objectIndex)
{
    QQmlPropertyCache *propertyCache = propertyCaches.value(objectIndex);
    Q_ASSERT(propertyCache);

    PropertyResolver propertyResolver(propertyCache);

    bool defaultPropertyQueried = false;
    QQmlPropertyData *defaultProperty = 0;

    for (QtQml::Binding *binding = obj->bindings->first; binding; binding = binding->next) {
        if (binding->type != QV4::CompiledData::Binding::Type_Object)
            continue;
        if (binding->flags & QV4::CompiledData::Binding::IsSignalHandlerExpression)
            continue;

        const QtQml::QmlObject *targetObject = qmlObjects->at(binding->value.objectIndex);
        QQmlType *targetType = resolvedTypes->value(targetObject->inheritedTypeNameIndex).type;
        if (targetType && targetType->metaObject() == &QQmlComponent::staticMetaObject)
            continue;

        QQmlPropertyData *pd = 0;
        QString propertyName = stringAt(binding->propertyNameIndex);
        if (!propertyName.isEmpty()) {
            bool notInRevision = false;
            pd = propertyResolver.property(propertyName, &notInRevision);
        } else {
            if (!defaultPropertyQueried) {
                defaultProperty = propertyCache->defaultProperty();
                defaultPropertyQueried = true;
            }
            pd = defaultProperty;
        }
        if (!pd || !pd->isQObject())
            continue;

        QQmlPropertyCache *pc = enginePrivate->rawPropertyCacheForType(pd->propType);
        const QMetaObject *mo = pc->firstCppMetaObject();
        while (mo) {
            if (mo == &QQmlComponent::staticMetaObject)
                break;
            mo = mo->superClass();
        }

        if (!mo)
            continue;

        static QQmlType *componentType = QQmlMetaType::qmlType(&QQmlComponent::staticMetaObject);
        Q_ASSERT(componentType);

        QtQml::QmlObject *syntheticComponent = pool->New<QtQml::QmlObject>();
        syntheticComponent->init(pool, compiler->registerString(QString::fromUtf8(componentType->typeName())), compiler->registerString(QString()));

        if (!resolvedTypes->contains(syntheticComponent->inheritedTypeNameIndex)) {
            QQmlCompiledData::TypeReference typeRef;
            typeRef.type = componentType;
            typeRef.majorVersion = componentType->majorVersion();
            typeRef.minorVersion = componentType->minorVersion();
            resolvedTypes->insert(syntheticComponent->inheritedTypeNameIndex, typeRef);
        }

        qmlObjects->append(syntheticComponent);
        const int componentIndex = qmlObjects->count() - 1;

        QtQml::Binding *syntheticBinding = pool->New<QtQml::Binding>();
        *syntheticBinding = *binding;
        syntheticBinding->type = QV4::CompiledData::Binding::Type_Object;
        syntheticComponent->bindings->append(syntheticBinding);

        binding->value.objectIndex = componentIndex;

        componentRoots.append(componentIndex);
        componentBoundaries.append(syntheticBinding->value.objectIndex);
    }
}

bool QQmlComponentAndAliasResolver::resolve()
{
    // Detect real Component {} objects as well as implicitly defined components, such as
    //     someItemDelegate: Item {}
    // In the implicit case Item is surrounded by a synthetic Component {} because the property
    // on the left hand side is of QQmlComponent type.
    const int objCountWithoutSynthesizedComponents = qmlObjects->count();
    for (int i = 0; i < objCountWithoutSynthesizedComponents; ++i) {
        const QtQml::QmlObject *obj = qmlObjects->at(i);
        if (stringAt(obj->inheritedTypeNameIndex).isEmpty())
            continue;

        QQmlCompiledData::TypeReference tref = resolvedTypes->value(obj->inheritedTypeNameIndex);
        if (!tref.type)
            continue;
        if (tref.type->metaObject() != &QQmlComponent::staticMetaObject) {
            findAndRegisterImplicitComponents(obj, i);
            continue;
        }

        componentRoots.append(i);

        if (obj->functions->count > 0)
            COMPILE_EXCEPTION(obj, tr("Component objects cannot declare new functions."));
        if (obj->properties->count > 0)
            COMPILE_EXCEPTION(obj, tr("Component objects cannot declare new properties."));
        if (obj->qmlSignals->count > 0)
            COMPILE_EXCEPTION(obj, tr("Component objects cannot declare new signals."));

        if (obj->bindings->count == 0)
            COMPILE_EXCEPTION(obj, tr("Cannot create empty component specification"));

        const QtQml::Binding *rootBinding = obj->bindings->first;
        if (rootBinding->next || rootBinding->type != QV4::CompiledData::Binding::Type_Object)
            COMPILE_EXCEPTION(rootBinding, tr("Component elements may not contain properties other than id"));

        componentBoundaries.append(rootBinding->value.objectIndex);
    }

    std::sort(componentBoundaries.begin(), componentBoundaries.end());

    for (int i = 0; i < componentRoots.count(); ++i) {
        const QtQml::QmlObject *component  = qmlObjects->at(componentRoots.at(i));
        const QtQml::Binding *rootBinding = component->bindings->first;

        _componentIndex = i;
        _idToObjectIndex.clear();

        _objectIndexToIdInScope = &(*objectIndexToIdPerComponent)[componentRoots.at(i)];

        _objectsWithAliases.clear();

        if (!collectIdsAndAliases(rootBinding->value.objectIndex))
            return false;

        if (!resolveAliases())
            return false;
    }

    // Collect ids and aliases for root
    _componentIndex = -1;
    _idToObjectIndex.clear();
    _objectIndexToIdInScope = objectIndexToIdForRoot;
    _objectsWithAliases.clear();

    collectIdsAndAliases(indexOfRootObject);

    resolveAliases();

    return true;
}

bool QQmlComponentAndAliasResolver::collectIdsAndAliases(int objectIndex)
{
    const QtQml::QmlObject *obj = qmlObjects->at(objectIndex);

    QString id = stringAt(obj->idIndex);
    if (!id.isEmpty()) {
        if (_idToObjectIndex.contains(obj->idIndex)) {
            recordError(obj->locationOfIdProperty, tr("id is not unique"));
            return false;
        }
        _idToObjectIndex.insert(obj->idIndex, objectIndex);
        _objectIndexToIdInScope->insert(objectIndex, _objectIndexToIdInScope->count());
    }

    for (QtQml::QmlProperty *property = obj->properties->first; property; property = property->next) {
        if (property->type == QV4::CompiledData::Property::Alias) {
            _objectsWithAliases.append(objectIndex);
            break;
        }
    }

    for (QtQml::Binding *binding = obj->bindings->first; binding; binding = binding->next) {
        if (binding->type != QV4::CompiledData::Binding::Type_Object
            && binding->type != QV4::CompiledData::Binding::Type_AttachedProperty
            && binding->type != QV4::CompiledData::Binding::Type_GroupProperty)
            continue;

        // Stop at Component boundary
        if (std::binary_search(componentBoundaries.constBegin(), componentBoundaries.constEnd(), binding->value.objectIndex))
            continue;

        if (!collectIdsAndAliases(binding->value.objectIndex))
            return false;
    }

    return true;
}

bool QQmlComponentAndAliasResolver::resolveAliases()
{
    foreach (int objectIndex, _objectsWithAliases) {
        const QtQml::QmlObject *obj = qmlObjects->at(objectIndex);

        QQmlPropertyCache *propertyCache = propertyCaches.value(objectIndex);
        Q_ASSERT(propertyCache);

        int effectiveSignalIndex = propertyCache->signalHandlerIndexCacheStart + propertyCache->propertyIndexCache.count();
        int effectivePropertyIndex = propertyCache->propertyIndexCacheStart + propertyCache->propertyIndexCache.count();
        int effectiveAliasIndex = 0;

        const QtQml::QmlProperty *p = obj->properties->first;
        for (int propertyIndex = 0; propertyIndex < obj->properties->count; ++propertyIndex, p = p->next) {
            if (p->type != QV4::CompiledData::Property::Alias)
                continue;

            const int idIndex = p->aliasIdValueIndex;
            const int targetObjectIndex = _idToObjectIndex.value(idIndex, -1);
            if (targetObjectIndex == -1) {
                recordError(p->aliasLocation, tr("Invalid alias reference. Unable to find id \"%1\"").arg(stringAt(idIndex)));
                return false;
            }
            const int targetId = _objectIndexToIdInScope->value(targetObjectIndex, -1);
            Q_ASSERT(targetId != -1);

            const QString aliasPropertyValue = stringAt(p->aliasPropertyValueIndex);

            QStringRef property;
            QStringRef subProperty;

            const int propertySeparator = aliasPropertyValue.indexOf(QLatin1Char('.'));
            if (propertySeparator != -1) {
                property = aliasPropertyValue.leftRef(propertySeparator);
                subProperty = aliasPropertyValue.midRef(propertySeparator + 1);
            } else
                property = QStringRef(&aliasPropertyValue, 0, aliasPropertyValue.length());

            int propIdx = -1;
            int propType = 0;
            int notifySignal = -1;
            int flags = 0;
            int type = 0;
            bool writable = false;
            bool resettable = false;

            quint32 propertyFlags = QQmlPropertyData::IsAlias;

            if (property.isEmpty()) {
                const QtQml::QmlObject *targetObject = qmlObjects->at(targetObjectIndex);
                QQmlCompiledData::TypeReference typeRef = resolvedTypes->value(targetObject->inheritedTypeNameIndex);

                if (typeRef.type)
                    type = typeRef.type->typeId();
                else
                    type = typeRef.component->metaTypeId;

                flags |= QML_ALIAS_FLAG_PTR;
                propertyFlags |= QQmlPropertyData::IsQObjectDerived;
            } else {
                QQmlPropertyCache *targetCache = propertyCaches.value(targetObjectIndex);
                Q_ASSERT(targetCache);
                QtQml::PropertyResolver resolver(targetCache);

                QQmlPropertyData *targetProperty = resolver.property(property.toString());
                if (!targetProperty || targetProperty->coreIndex > 0x0000FFFF) {
                    recordError(p->aliasLocation, tr("Invalid alias location"));
                    return false;
                }

                propIdx = targetProperty->coreIndex;
                type = targetProperty->propType;

                writable = targetProperty->isWritable();
                resettable = targetProperty->isResettable();
                notifySignal = targetProperty->notifyIndex;

                if (!subProperty.isEmpty()) {
                    QQmlValueType *valueType = QQmlValueTypeFactory::valueType(type);
                    if (!valueType) {
                        recordError(p->aliasLocation, tr("Invalid alias location"));
                        return false;
                    }

                    propType = type;

                    int valueTypeIndex =
                        valueType->metaObject()->indexOfProperty(subProperty.toString().toUtf8().constData());
                    if (valueTypeIndex == -1) {
                        recordError(p->aliasLocation, tr("Invalid alias location"));
                        return false;
                    }
                    Q_ASSERT(valueTypeIndex <= 0x0000FFFF);

                    propIdx |= (valueTypeIndex << 16);
                    if (valueType->metaObject()->property(valueTypeIndex).isEnumType())
                        type = QVariant::Int;
                    else
                        type = valueType->metaObject()->property(valueTypeIndex).userType();

                } else {
                    if (targetProperty->isEnum()) {
                        type = QVariant::Int;
                    } else {
                        // Copy type flags
                        propertyFlags |= targetProperty->getFlags() & QQmlPropertyData::PropTypeFlagMask;

                        if (targetProperty->isVarProperty())
                            propertyFlags |= QQmlPropertyData::IsQVariant;

                        if (targetProperty->isQObject())
                            flags |= QML_ALIAS_FLAG_PTR;
                    }
                }
            }

            QQmlVMEMetaData::AliasData aliasData = { targetId, propIdx, propType, flags, notifySignal };

            typedef QQmlVMEMetaData VMD;
            QByteArray &dynamicData = (*vmeMetaObjectData)[objectIndex];
            Q_ASSERT(!dynamicData.isEmpty());
            VMD *vmd = (QQmlVMEMetaData *)dynamicData.data();
            *(vmd->aliasData() + effectiveAliasIndex++) = aliasData;

            Q_ASSERT(dynamicData.isDetached());

            if (!(p->flags & QV4::CompiledData::Property::IsReadOnly) && writable)
                propertyFlags |= QQmlPropertyData::IsWritable;
            else
                propertyFlags &= ~QQmlPropertyData::IsWritable;

            if (resettable)
                propertyFlags |= QQmlPropertyData::IsResettable;
            else
                propertyFlags &= ~QQmlPropertyData::IsResettable;

            QString propertyName = stringAt(p->nameIndex);
            if (propertyIndex == obj->indexOfDefaultProperty) propertyCache->_defaultPropertyName = propertyName;
            propertyCache->appendProperty(propertyName, propertyFlags, effectivePropertyIndex++,
                                          type, effectiveSignalIndex++);

        }
    }
    return true;
}


QQmlPropertyValidator::QQmlPropertyValidator(QQmlTypeCompiler *typeCompiler)
    : QQmlCompilePass(typeCompiler)
    , qmlUnit(typeCompiler->qmlUnit())
    , resolvedTypes(*typeCompiler->resolvedTypes())
    , propertyCaches(typeCompiler->propertyCaches())
    , objectIndexToIdPerComponent(*typeCompiler->objectIndexToIdPerComponent())
    , customParserData(typeCompiler->customParserData())
{
}

bool QQmlPropertyValidator::validate()
{
    return validateObject(qmlUnit->indexOfRootObject);
}

const QQmlImports &QQmlPropertyValidator::imports() const
{
    return *compiler->imports();
}

bool QQmlPropertyValidator::validateObject(int objectIndex)
{
    const QV4::CompiledData::Object *obj = qmlUnit->objectAt(objectIndex);
    if (stringAt(obj->inheritedTypeNameIndex).isEmpty())
        return true;

    if (isComponent(objectIndex))
        return true;

    QQmlPropertyCache *propertyCache = propertyCaches.at(objectIndex);
    Q_ASSERT(propertyCache);

    QQmlCustomParser *customParser = 0;
    QQmlCompiledData::TypeReference objectType = resolvedTypes.value(obj->inheritedTypeNameIndex);
    if (objectType.type)
        customParser = objectType.type->customParser();
    QList<const QV4::CompiledData::Binding*> customBindings;

    PropertyResolver propertyResolver(propertyCache);

    QQmlPropertyData *defaultProperty = propertyCache->defaultProperty();

    const QV4::CompiledData::Binding *binding = obj->bindingTable();
    for (quint32 i = 0; i < obj->nBindings; ++i, ++binding) {
        if (binding->type >= QV4::CompiledData::Binding::Type_Object && !customParser) {
            if (!validateObject(binding->value.objectIndex))
                return false;
        }

        if (binding->type == QV4::CompiledData::Binding::Type_AttachedProperty
            || binding->type == QV4::CompiledData::Binding::Type_GroupProperty) {
            if (customParser)
                customBindings << binding;
            continue;
        }

        const QString name = stringAt(binding->propertyNameIndex);

        bool bindingToDefaultProperty = false;

        bool notInRevision = false;
        QQmlPropertyData *pd = 0;
        if (!name.isEmpty()) {
            if (binding->flags & QV4::CompiledData::Binding::IsSignalHandlerExpression)
                pd = propertyResolver.signal(name, &notInRevision);
            else
                pd = propertyResolver.property(name, &notInRevision);

            if (notInRevision) {
                QString typeName = stringAt(obj->inheritedTypeNameIndex);
                if (objectType.type) {
                    COMPILE_EXCEPTION(binding, tr("\"%1.%2\" is not available in %3 %4.%5.").arg(typeName).arg(name).arg(objectType.type->module()).arg(objectType.majorVersion).arg(objectType.minorVersion));
                } else {
                    COMPILE_EXCEPTION(binding, tr("\"%1.%2\" is not available due to component versioning.").arg(typeName).arg(name));
                }
            }
        } else {
           pd = defaultProperty;
           bindingToDefaultProperty = true;
        }

        if (!pd) {
            if (customParser) {
                customBindings << binding;
                continue;
            }
            if (bindingToDefaultProperty) {
                COMPILE_EXCEPTION(binding, tr("Cannot assign to non-existent default property"));
            } else {
                COMPILE_EXCEPTION(binding, tr("Cannot assign to non-existent property \"%1\"").arg(name));
            }
        }
    }

    if (customParser && !customBindings.isEmpty()) {
        customParser->clearErrors();
        customParser->compiler = this;
        QByteArray data = customParser->compile(qmlUnit, customBindings);
        customParser->compiler = 0;
        customParserData->insert(objectIndex, data);
        const QList<QQmlError> parserErrors = customParser->errors();
        if (!parserErrors.isEmpty()) {
            foreach (QQmlError error, parserErrors)
                compiler->recordError(error);
            return false;
        }
    }

    return true;
}

QT_END_NAMESPACE
