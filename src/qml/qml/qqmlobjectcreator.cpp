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

#include "qqmlobjectcreator_p.h"

#include <private/qqmlengine_p.h>
#include <private/qqmlabstractbinding_p.h>
#include <private/qqmlvmemetaobject_p.h>
#include <private/qv4function_p.h>
#include <private/qv4functionobject_p.h>
#include <private/qqmlcontextwrapper_p.h>
#include <private/qqmlbinding_p.h>
#include <private/qqmlstringconverters_p.h>

QT_USE_NAMESPACE

#define COMPILE_EXCEPTION(token, desc) \
    { \
        recordError((token)->location, desc); \
        return false; \
    }

static QAtomicInt classIndexCounter(0);

QQmlPropertyCacheCreator::QQmlPropertyCacheCreator(QQmlEnginePrivate *enginePrivate, const QV4::CompiledData::QmlUnit *unit, const QUrl &url, QQmlTypeNameCache *typeNameCache, const QQmlImports *imports)
    : enginePrivate(enginePrivate)
    , unit(unit)
    , url(url)
    , typeNameCache(typeNameCache)
    , imports(imports)
{
}

bool QQmlPropertyCacheCreator::create(const QV4::CompiledData::Object *obj, QQmlPropertyCache **resultCache, QByteArray *vmeMetaObjectData)
{
    QQmlTypeNameCache::Result res = typeNameCache->query(stringAt(obj->inheritedTypeNameIndex));
    Q_ASSERT(res.isValid()); // types resolved earlier in resolveTypes()

    QQmlPropertyCache *baseTypeCache = enginePrivate->cache(res.type->metaObject());
    if (obj->nProperties == 0 && obj->nSignals == 0 && obj->nFunctions == 0) {
        *resultCache = baseTypeCache;
        vmeMetaObjectData->clear();
        return true;
    }

    QQmlPropertyCache *cache = baseTypeCache->copyAndReserve(QQmlEnginePrivate::get(enginePrivate), obj->nProperties, obj->nFunctions, obj->nSignals);
    *resultCache = cache;

    vmeMetaObjectData->clear();

    struct TypeData {
        QV4::CompiledData::Property::Type dtype;
        int metaType;
    } builtinTypes[] = {
        { QV4::CompiledData::Property::Var, QMetaType::QVariant },
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

    const QV4::CompiledData::Property *p = obj->propertyTable();
    for (quint32 i = 0; i < obj->nProperties; ++i, ++p) {

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

    QByteArray &dynamicData = *vmeMetaObjectData = QByteArray(sizeof(QQmlVMEMetaData)
                                                              + obj->nProperties * sizeof(VMD::PropertyData)
                                                              + obj->nFunctions * sizeof(VMD::MethodData)
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

        const QV4::CompiledData::Property *p = obj->propertyTable();
        for (quint32 i = 0; i < obj->nProperties; ++i, ++p) {
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
    for (uint i = 0; i < obj->nSignals; ++i) {
        const QV4::CompiledData::Signal *s = obj->signalAt(i);
        const int paramCount = s->nParameters;

        QList<QByteArray> names;
        QVarLengthArray<int, 10> paramTypes(paramCount?(paramCount + 1):0);

        if (paramCount) {
            paramTypes[0] = paramCount;

            for (int i = 0; i < paramCount; ++i) {
                const QV4::CompiledData::Parameter *param = s->parameterAt(i);
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
        if (seenSignals.contains(signalName)) {
#if 0 // ###
            const QQmlScript::Object::DynamicSignal &currSig = *s;
            COMPILE_EXCEPTION(&currSig, tr("Duplicate signal name: invalid override of property change signal or superclass signal"));
#endif
        }
        seenSignals.insert(signalName);

        cache->appendSignal(signalName, flags, effectiveMethodIndex++,
                            paramCount?paramTypes.constData():0, names);
    }


    // Dynamic slots
    const quint32 *functionIndex = obj->functionOffsetTable();
    for (quint32 i = 0; i < obj->nFunctions; ++i, ++functionIndex) {
        const QV4::CompiledData::Function *s = unit->header.functionAt(*functionIndex);
        int paramCount = s->nFormals;

        quint32 flags = QQmlPropertyData::IsFunction | QQmlPropertyData::IsVMEFunction;

        if (paramCount)
            flags |= QQmlPropertyData::HasArguments;

        QString slotName = stringAt(s->nameIndex);
        if (seenSignals.contains(slotName)) {
#if 0 // ###
            const QQmlScript::Object::DynamicSlot &currSlot = *s;
            COMPILE_EXCEPTION(&currSlot, tr("Duplicate method name: invalid override of property change signal or superclass signal"));
#endif
        }
        // Note: we don't append slotName to the seenSignals list, since we don't
        // protect against overriding change signals or methods with properties.

        const quint32 *formalsIndices = s->formalsTable();
        QList<QByteArray> parameterNames;
        parameterNames.reserve(paramCount);
        for (int i = 0; i < paramCount; ++i)
            parameterNames << stringAt(formalsIndices[i]).toUtf8();

        cache->appendMethod(slotName, flags, effectiveMethodIndex++, parameterNames);
    }


    // Dynamic properties (except var and aliases)
    int effectiveSignalIndex = cache->signalHandlerIndexCacheStart;
    /* const QV4::CompiledData::Property* */ p = obj->propertyTable();
    for (quint32 i = 0; i < obj->nProperties; ++i, ++p) {

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
             //   COMPILE_EXCEPTION(p, tr("Invalid property type"));
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
        if (i == obj->indexOfDefaultProperty) cache->_defaultPropertyName = propertyName;
        cache->appendProperty(propertyName, propertyFlags, effectivePropertyIndex++,
                              propertyType, effectiveSignalIndex);

        effectiveSignalIndex++;

        VMD *vmd = (QQmlVMEMetaData *)dynamicData.data();
        (vmd->propertyData() + vmd->propertyCount)->propertyType = vmePropertyType;
        vmd->propertyCount++;
    }

    // Now do var properties
    /* const QV4::CompiledData::Property* */ p = obj->propertyTable();
    for (quint32 i = 0; i < obj->nProperties; ++i, ++p) {

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
        if (i == obj->indexOfDefaultProperty) cache->_defaultPropertyName = propertyName;
        cache->appendProperty(propertyName, propertyFlags, effectivePropertyIndex++,
                              QMetaType::QVariant, effectiveSignalIndex);

        effectiveSignalIndex++;
    }

    // Alias property count.  Actual data is setup in buildDynamicMetaAliases
    ((QQmlVMEMetaData *)dynamicData.data())->aliasCount = aliasCount;

    // Dynamic slot data - comes after the property data
    /*const quint32* */functionIndex = obj->functionOffsetTable();
    for (quint32 i = 0; i < obj->nFunctions; ++i, ++functionIndex) {
        const QV4::CompiledData::Function *s = unit->header.functionAt(*functionIndex);

        VMD::MethodData methodData = { int(s->nFormals),
                                       /* body offset*/0,
                                       /* body length*/0,
                                       /* s->location.start.line */0 }; // ###

        VMD *vmd = (QQmlVMEMetaData *)dynamicData.data();
        VMD::MethodData &md = *(vmd->methodData() + vmd->methodCount);
        vmd->methodCount++;
        md = methodData;
    }

    return true;
}

void QQmlPropertyCacheCreator::recordError(const QV4::CompiledData::Location &location, const QString &description)
{
    QQmlError error;
    error.setUrl(url);
    error.setLine(location.line);
    error.setColumn(location.column);
    error.setDescription(description);
    errors << error;
}

static void removeBindingOnProperty(QObject *o, int index)
{
    int coreIndex = index & 0x0000FFFF;
    int valueTypeIndex = (index & 0xFFFF0000 ? index >> 16 : -1);

    QQmlAbstractBinding *binding = QQmlPropertyPrivate::setBinding(o, coreIndex, valueTypeIndex, 0);
    if (binding) binding->destroy();
}

QmlObjectCreator::QmlObjectCreator(QQmlContextData *contextData, const QV4::CompiledData::QmlUnit *qmlUnit,
                                   const QV4::CompiledData::CompilationUnit *jsUnit, QQmlTypeNameCache *typeNameCache,
                                   const QList<QQmlPropertyCache*> &propertyCaches,
                                   const QList<QByteArray> &vmeMetaObjectData)
    : engine(contextData->engine)
    , unit(qmlUnit)
    , jsUnit(jsUnit)
    , context(contextData)
    , typeNameCache(typeNameCache)
    , propertyCaches(propertyCaches)
    , vmeMetaObjectData(vmeMetaObjectData)
    , _qobject(0)
    , _compiledObject(0)
    , _ddata(0)
    , _propertyCache(0)
{
}

QVector<QQmlAbstractBinding*> QmlObjectCreator::setupBindings(QV4::Object *qmlGlobal)
{
    QV4::ExecutionEngine *v4 = QV8Engine::getV4(engine);

    QQmlPropertyPrivate::WriteFlags propertyWriteFlags = QQmlPropertyPrivate::BypassInterceptor |
                                                               QQmlPropertyPrivate::RemoveBindingOnAliasWrite;
    int propertyWriteStatus = -1;
    QVariant fallbackVariantValue;

    QVector<QQmlAbstractBinding*> createdDynamicBindings(_compiledObject->nBindings, 0);

    const QV4::CompiledData::Binding *binding = _compiledObject->bindingTable();
    for (quint32 i = 0; i < _compiledObject->nBindings; ++i, ++binding) {
        QString name = stringAt(binding->propertyNameIndex);

        if (name.isEmpty() && binding->value.type == QV4::CompiledData::Value::Type_Object) {
            create(binding->value.objectIndex, _qobject);
            continue;
        }

        QQmlPropertyData *property = _propertyCache->property(name, _qobject, context);

        if (_ddata->hasBindingBit(property->coreIndex))
            removeBindingOnProperty(_qobject, property->coreIndex);

        if (binding->value.type == QV4::CompiledData::Value::Type_Script) {
            QV4::Function *runtimeFunction = jsUnit->runtimeFunctions[binding->value.compiledScriptIndex];
            QV4::FunctionObject *function = new (v4->memoryManager) QV4::QmlBindingWrapper(v4->rootContext, runtimeFunction, qmlGlobal);
            QQmlBinding *binding = new QQmlBinding(QV4::Value::fromObject(function), _qobject, context,
                                                   QString(), 0, 0); // ###

            binding->setTarget(_qobject, *property, context);
            binding->addToObject();

            createdDynamicBindings[i] = binding;
            binding->m_mePtr = &createdDynamicBindings[i];
            continue;
        }

        void *argv[] = { 0, 0, &propertyWriteStatus, &propertyWriteFlags };

        // shortcuts
#if 0
        if (property->propType == QMetaType::Double && binding->value.type == QV4::CompiledData::Value::Type_Number) {
            argv[0] = const_cast<double*>(&binding->value.d);
        } else if (property->propType == QMetaType::Bool && binding->value.type == QV4::CompiledData::Value::Type_Boolean) {
            argv[0] = const_cast<bool*>(&binding->value.b);
        } else
#endif
        {
            // fallback
            fallbackVariantValue = variantForBinding(property->propType, binding);

            if (property->propType == QMetaType::QVariant)
                argv[0] = &fallbackVariantValue;
            else
                argv[0] = fallbackVariantValue.data();
        }

        QMetaObject::metacall(_qobject, QMetaObject::WriteProperty, property->coreIndex, argv);
    }

    return createdDynamicBindings;
}

void QmlObjectCreator::setupFunctions(QV4::Object *qmlGlobal)
{
    QQmlVMEMetaObject *vme = QQmlVMEMetaObject::get(_qobject);
    QV4::ExecutionEngine *v4 = QV8Engine::getV4(engine);

    const quint32 *functionIdx = _compiledObject->functionOffsetTable();
    for (quint32 i = 0; i < _compiledObject->nFunctions; ++i, ++functionIdx) {
        QV4::Function *function = jsUnit->runtimeFunctions[*functionIdx];
        const QString name = function->name->toQString();

        QQmlPropertyData *property = _propertyCache->property(name, _qobject, context);
        if (!property->isVMEFunction())
            continue;

        QV4::FunctionObject *v4Function = new (v4->memoryManager) QV4::QmlBindingWrapper(v4->rootContext, function, qmlGlobal);
        vme->setVmeMethod(property->coreIndex, QV4::Value::fromObject(v4Function));
    }
}

QObject *QmlObjectCreator::create(int index, QObject *parent)
{
    const QV4::CompiledData::Object *obj = unit->objectAt(index);

    QQmlTypeNameCache::Result res = typeNameCache->query(stringAt(obj->inheritedTypeNameIndex));
    if (!res.isValid())
        return 0;

    QObject *result = res.type->create();
    // ### use no-event variant
    if (parent)
        result->setParent(parent);

    QQmlData *declarativeData = QQmlData::get(result, /*create*/true);

    QQmlRefPointer<QQmlPropertyCache> cache = propertyCaches.value(index);
    Q_ASSERT(!cache.isNull());

    qSwap(_propertyCache, cache);
    qSwap(_qobject, result);
    qSwap(_compiledObject, obj);
    qSwap(_ddata, declarativeData);

    context->addObject(_qobject);

    const QByteArray data = vmeMetaObjectData.value(index);
    if (!data.isEmpty()) {
        // install on _object
        (void)new QQmlVMEMetaObject(_qobject, _propertyCache, reinterpret_cast<const QQmlVMEMetaData*>(data.constData()));
        if (_ddata->propertyCache)
            _ddata->propertyCache->release();
        _ddata->propertyCache = _propertyCache;
        _ddata->propertyCache->addref();
    }

    QV4::Value scopeObject = QV4::QmlContextWrapper::qmlScope(QV8Engine::get(engine), context, _qobject);

    QVector<QQmlAbstractBinding*> dynamicBindings = setupBindings(scopeObject.asObject());
    setupFunctions(scopeObject.asObject());

    // ### do this later when requested
    for (int i = 0; i < dynamicBindings.count(); ++i) {
        QQmlAbstractBinding *b = dynamicBindings.at(i);
        if (!b)
            continue;
        b->m_mePtr = 0;
        QQmlData *data = QQmlData::get(b->object());
        Q_ASSERT(data);
        data->clearPendingBindingBit(b->propertyIndex());
        b->setEnabled(true, QQmlPropertyPrivate::BypassInterceptor |
                            QQmlPropertyPrivate::DontRemoveBinding);
    }

    qSwap(_propertyCache, cache);
    qSwap(_ddata, declarativeData);
    qSwap(_compiledObject, obj);
    qSwap(_qobject, result);

    return result;
}

QVariant QmlObjectCreator::variantForBinding(int expectedMetaType, const QV4::CompiledData::Binding *binding) const
{
    QVariant result;

    switch (expectedMetaType) {
    case QMetaType::QString:
        result = valueAsString(&binding->value);
        break;
    case QMetaType::Bool:
        result = valueAsBoolean(&binding->value);
        break;
    case QMetaType::Double:
        result = valueAsNumber(&binding->value);
        break;
    case QMetaType::Int:
        result = (int)valueAsNumber(&binding->value);
        break;
    case QVariant::Color: {
        bool ok = false;
        result = QQmlStringConverters::colorFromString(valueAsString(&binding->value), &ok);
        if (!ok) {
            // ### compile error
        }
        break;
    }
    default:
        QQmlMetaType::StringConverter converter = QQmlMetaType::customStringConverter(expectedMetaType);
        if (converter) {
            result = converter(valueAsString(&binding->value));
        } else {
            if (expectedMetaType == QMetaType::QVariant)
                result = QVariant();
            else
                result = QVariant(expectedMetaType, (void*)0);
        }
        break;
    }
    return result;
}

QString QmlObjectCreator::valueAsString(const QV4::CompiledData::Value *value) const
{
    switch (value->type) {
    case QV4::CompiledData::Value::Type_Script:
    case QV4::CompiledData::Value::Type_String:
        return stringAt(value->stringIndex);
    case QV4::CompiledData::Value::Type_Boolean:
        return value->b ? QStringLiteral("true") : QStringLiteral("false");
    case QV4::CompiledData::Value::Type_Number:
        return QString::number(value->d);
    case QV4::CompiledData::Value::Type_Invalid:
        return QString();
    default:
        break;
    }
    return QString();
}

double QmlObjectCreator::valueAsNumber(const QV4::CompiledData::Value *value)
{
    if (value->type == QV4::CompiledData::Value::Type_Number)
        return value->d;
    return 0.0;
}

bool QmlObjectCreator::valueAsBoolean(const QV4::CompiledData::Value *value)
{
    if (value->type == QV4::CompiledData::Value::Type_Boolean)
        return value->b;
    return false;
}

void QmlObjectCreator::recordError(const QV4::CompiledData::Location &location, const QString &description)
{
    QQmlError error;
    error.setUrl(url);
    error.setLine(location.line);
    error.setColumn(location.column);
    error.setDescription(description);
    errors << error;
}
