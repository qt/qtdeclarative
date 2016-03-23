/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qqmlobjectcreator_p.h"

#include <private/qqmlengine_p.h>
#include <private/qqmlvmemetaobject_p.h>
#include <private/qv4function_p.h>
#include <private/qv4functionobject_p.h>
#include <private/qv4qobjectwrapper_p.h>
#include <private/qqmlcontextwrapper_p.h>
#include <private/qqmlbinding_p.h>
#include <private/qqmlstringconverters_p.h>
#include <private/qqmlboundsignal_p.h>
#include <private/qqmlcomponentattached_p.h>
#include <private/qqmlcomponent_p.h>
#include <private/qqmlcustomparser_p.h>
#include <private/qqmlscriptstring_p.h>
#include <private/qqmlpropertyvalueinterceptor_p.h>
#include <private/qqmlvaluetypeproxybinding_p.h>

QT_USE_NAMESPACE

namespace {
struct ActiveOCRestorer
{
    ActiveOCRestorer(QQmlObjectCreator *creator, QQmlEnginePrivate *ep)
    : ep(ep), oldCreator(ep->activeObjectCreator) { ep->activeObjectCreator = creator; }
    ~ActiveOCRestorer() { ep->activeObjectCreator = oldCreator; }

    QQmlEnginePrivate *ep;
    QQmlObjectCreator *oldCreator;
};
}

QQmlObjectCreator::QQmlObjectCreator(QQmlContextData *parentContext, QQmlCompiledData *compiledData, QQmlContextData *creationContext, void *activeVMEDataForRootContext)
    : phase(Startup)
    , compiledData(compiledData)
    , resolvedTypes(compiledData->resolvedTypes)
    , propertyCaches(compiledData->propertyCaches)
    , vmeMetaObjectData(compiledData->metaObjects)
    , activeVMEDataForRootContext(activeVMEDataForRootContext)
{
    init(parentContext);

    sharedState = new QQmlObjectCreatorSharedState;
    topLevelCreator = true;
    sharedState->componentAttached = 0;
    sharedState->allCreatedBindings.allocate(compiledData->totalBindingsCount);
    sharedState->allParserStatusCallbacks.allocate(compiledData->totalParserStatusCount);
    sharedState->allCreatedObjects.allocate(compiledData->totalObjectCount);
    sharedState->allJavaScriptObjects = 0;
    sharedState->creationContext = creationContext;
    sharedState->rootContext = 0;

    QQmlProfiler *profiler = QQmlEnginePrivate::get(engine)->profiler;
    Q_QML_PROFILE_IF_ENABLED(QQmlProfilerDefinitions::ProfileCreating, profiler,
            sharedState->profiler.init(profiler, compiledData->totalParserStatusCount));
}

QQmlObjectCreator::QQmlObjectCreator(QQmlContextData *parentContext, QQmlCompiledData *compiledData, QQmlObjectCreatorSharedState *inheritedSharedState)
    : phase(Startup)
    , compiledData(compiledData)
    , resolvedTypes(compiledData->resolvedTypes)
    , propertyCaches(compiledData->propertyCaches)
    , vmeMetaObjectData(compiledData->metaObjects)
    , activeVMEDataForRootContext(0)
{
    init(parentContext);

    sharedState = inheritedSharedState;
    topLevelCreator = false;
}

void QQmlObjectCreator::init(QQmlContextData *providedParentContext)
{
    parentContext = providedParentContext;
    engine = parentContext->engine;
    v4 = QV8Engine::getV4(engine);

    if (!compiledData->isInitialized())
        compiledData->initialize(engine);

    qmlUnit = compiledData->compilationUnit->data;
    context = 0;
    _qobject = 0;
    _scopeObject = 0;
    _valueTypeProperty = 0;
    _compiledObject = 0;
    _compiledObjectIndex = -1;
    _ddata = 0;
    _propertyCache = 0;
    _vmeMetaObject = 0;
    _qmlContext = 0;
}

QQmlObjectCreator::~QQmlObjectCreator()
{
    if (topLevelCreator) {
        {
            QQmlObjectCreatorRecursionWatcher watcher(this);
        }
        for (int i = 0; i < sharedState->allParserStatusCallbacks.count(); ++i) {
            QQmlParserStatus *ps = sharedState->allParserStatusCallbacks.at(i);
            if (ps)
                ps->d = 0;
        }
        while (sharedState->componentAttached) {
            QQmlComponentAttached *a = sharedState->componentAttached;
            a->rem();
        }
    }
}

QObject *QQmlObjectCreator::create(int subComponentIndex, QObject *parent, QQmlInstantiationInterrupt *interrupt)
{
    if (phase == CreatingObjectsPhase2) {
        phase = ObjectsCreated;
        return context->contextObject;
    }
    Q_ASSERT(phase == Startup);
    phase = CreatingObjects;

    int objectToCreate;

    if (subComponentIndex == -1) {
        objectIndexToId = compiledData->objectIndexToIdForRoot;
        objectToCreate = qmlUnit->indexOfRootObject;
    } else {
        objectIndexToId = compiledData->objectIndexToIdPerComponent[subComponentIndex];
        const QV4::CompiledData::Object *compObj = qmlUnit->objectAt(subComponentIndex);
        objectToCreate = compObj->bindingTable()->value.objectIndex;
    }

    context = new QQmlContextData;
    context->isInternal = true;
    context->imports = compiledData->importCache;
    context->imports->addref();
    context->typeCompilationUnit = compiledData->compilationUnit;
    context->setParent(parentContext);

    if (!sharedState->rootContext) {
        sharedState->rootContext = context;
        sharedState->rootContext->activeVMEData = activeVMEDataForRootContext;
        sharedState->rootContext->isRootObjectInCreation = true;
    }

    QV4::Scope scope(v4);

    Q_ASSERT(sharedState->allJavaScriptObjects || topLevelCreator);
    if (topLevelCreator)
        sharedState->allJavaScriptObjects = scope.alloc(compiledData->totalObjectCount);

    context->setIdPropertyData(objectIndexToId);

    if (subComponentIndex == -1 && compiledData->scripts.count()) {
        QV4::ScopedObject scripts(scope, v4->newArrayObject(compiledData->scripts.count()));
        context->importedScripts.set(v4, scripts);
        QV4::ScopedValue v(scope);
        for (int i = 0; i < compiledData->scripts.count(); ++i) {
            QQmlScriptData *s = compiledData->scripts.at(i);
            scripts->putIndexed(i, (v = s->scriptValueForContext(context)));
        }
    } else if (sharedState->creationContext) {
        context->importedScripts = sharedState->creationContext->importedScripts;
    }

    QObject *instance = createInstance(objectToCreate, parent, /*isContextObject*/true);
    if (instance) {
        QQmlData *ddata = QQmlData::get(instance);
        Q_ASSERT(ddata);
        if (ddata->compiledData)
            ddata->compiledData->release();
        ddata->compiledData = compiledData;
        ddata->compiledData->addref();
    }

    if (topLevelCreator)
        sharedState->allJavaScriptObjects = 0;

    phase = CreatingObjectsPhase2;

    if (interrupt && interrupt->shouldInterrupt())
        return 0;

    phase = ObjectsCreated;

    return instance;
}

bool QQmlObjectCreator::populateDeferredProperties(QObject *instance)
{
    QQmlData *declarativeData = QQmlData::get(instance);
    context = declarativeData->deferredData->context;
    sharedState->rootContext = context;

    QObject *bindingTarget = instance;

    QQmlRefPointer<QQmlPropertyCache> cache = declarativeData->propertyCache;
    QQmlVMEMetaObject *vmeMetaObject = QQmlVMEMetaObject::get(instance);

    QObject *scopeObject = instance;
    qSwap(_scopeObject, scopeObject);

    QV4::Scope valueScope(v4);

    Q_ASSERT(topLevelCreator);
    Q_ASSERT(!sharedState->allJavaScriptObjects);
    sharedState->allJavaScriptObjects = valueScope.alloc(compiledData->totalObjectCount);

    QV4::QmlContext *qmlContext = static_cast<QV4::QmlContext *>(valueScope.alloc(1));

    qSwap(_qmlContext, qmlContext);

    qSwap(_propertyCache, cache);
    qSwap(_qobject, instance);

    int objectIndex = declarativeData->deferredData->deferredIdx;
    qSwap(_compiledObjectIndex, objectIndex);

    const QV4::CompiledData::Object *obj = qmlUnit->objectAt(_compiledObjectIndex);
    qSwap(_compiledObject, obj);

    qSwap(_ddata, declarativeData);
    qSwap(_bindingTarget, bindingTarget);
    qSwap(_vmeMetaObject, vmeMetaObject);

    QBitArray bindingSkipList = compiledData->deferredBindingsPerObject.value(_compiledObjectIndex);
    for (int i = 0; i < bindingSkipList.count(); ++i)
        bindingSkipList.setBit(i, !bindingSkipList.testBit(i));

    setupBindings(bindingSkipList);

    qSwap(_vmeMetaObject, vmeMetaObject);
    qSwap(_bindingTarget, bindingTarget);
    qSwap(_ddata, declarativeData);
    qSwap(_compiledObject, obj);
    qSwap(_compiledObjectIndex, objectIndex);
    qSwap(_qobject, instance);
    qSwap(_propertyCache, cache);

    qSwap(_qmlContext, qmlContext);
    qSwap(_scopeObject, scopeObject);

    phase = ObjectsCreated;

    return errors.isEmpty();
}

void QQmlObjectCreator::setPropertyValue(const QQmlPropertyData *property, const QV4::CompiledData::Binding *binding)
{
    QQmlPropertyPrivate::WriteFlags propertyWriteFlags = QQmlPropertyPrivate::BypassInterceptor |
                                                               QQmlPropertyPrivate::RemoveBindingOnAliasWrite;
    int propertyWriteStatus = -1;
    void *argv[] = { 0, 0, &propertyWriteStatus, &propertyWriteFlags };

    QV4::Scope scope(v4);

    int propertyType = property->propType;

    if (property->isEnum()) {
        if (binding->flags & QV4::CompiledData::Binding::IsResolvedEnum) {
            propertyType = QMetaType::Int;
        } else {
            // ### This should be resolved earlier at compile time and the binding value should be changed accordingly.
            QVariant value = binding->valueAsString(qmlUnit);
            bool ok = QQmlPropertyPrivate::write(_qobject, *property, value, context);
            Q_ASSERT(ok);
            Q_UNUSED(ok);
            return;
        }
    }

    switch (propertyType) {
    case QMetaType::QVariant: {
        if (binding->type == QV4::CompiledData::Binding::Type_Number) {
            double n = binding->valueAsNumber();
            if (double(int(n)) == n) {
                if (property->isVarProperty()) {
                    _vmeMetaObject->setVMEProperty(property->coreIndex, QV4::Primitive::fromInt32(int(n)));
                } else {
                    int i = int(n);
                    QVariant value(i);
                    argv[0] = &value;
                    QMetaObject::metacall(_qobject, QMetaObject::WriteProperty, property->coreIndex, argv);
                }
            } else {
                if (property->isVarProperty()) {
                    _vmeMetaObject->setVMEProperty(property->coreIndex, QV4::Primitive::fromDouble(n));
                } else {
                    QVariant value(n);
                    argv[0] = &value;
                    QMetaObject::metacall(_qobject, QMetaObject::WriteProperty, property->coreIndex, argv);
                }
            }
        } else if (binding->type == QV4::CompiledData::Binding::Type_Boolean) {
            if (property->isVarProperty()) {
                _vmeMetaObject->setVMEProperty(property->coreIndex, QV4::Primitive::fromBoolean(binding->valueAsBoolean()));
            } else {
                QVariant value(binding->valueAsBoolean());
                argv[0] = &value;
                QMetaObject::metacall(_qobject, QMetaObject::WriteProperty, property->coreIndex, argv);
            }
        } else {
            QString stringValue = binding->valueAsString(qmlUnit);
            if (property->isVarProperty()) {
                QV4::ScopedString s(scope, v4->newString(stringValue));
                _vmeMetaObject->setVMEProperty(property->coreIndex, s);
            } else {
                QVariant value = QQmlStringConverters::variantFromString(stringValue);
                argv[0] = &value;
                QMetaObject::metacall(_qobject, QMetaObject::WriteProperty, property->coreIndex, argv);
            }
        }
    }
    break;
    case QVariant::String: {
        Q_ASSERT(binding->evaluatesToString());
        QString value = binding->valueAsString(qmlUnit);
        argv[0] = &value;
        QMetaObject::metacall(_qobject, QMetaObject::WriteProperty, property->coreIndex, argv);
    }
    break;
    case QVariant::StringList: {
        Q_ASSERT(binding->evaluatesToString());
        QStringList value(binding->valueAsString(qmlUnit));
        argv[0] = &value;
        QMetaObject::metacall(_qobject, QMetaObject::WriteProperty, property->coreIndex, argv);
    }
    break;
    case QVariant::ByteArray: {
        Q_ASSERT(binding->type == QV4::CompiledData::Binding::Type_String);
        QByteArray value(binding->valueAsString(qmlUnit).toUtf8());
        argv[0] = &value;
        QMetaObject::metacall(_qobject, QMetaObject::WriteProperty, property->coreIndex, argv);
    }
    break;
    case QVariant::Url: {
        Q_ASSERT(binding->type == QV4::CompiledData::Binding::Type_String);
        QString string = binding->valueAsString(qmlUnit);
        // Encoded dir-separators defeat QUrl processing - decode them first
        string.replace(QLatin1String("%2f"), QLatin1String("/"), Qt::CaseInsensitive);
        QUrl value = string.isEmpty() ? QUrl() : compiledData->url().resolved(QUrl(string));
        // Apply URL interceptor
        if (engine->urlInterceptor())
            value = engine->urlInterceptor()->intercept(value, QQmlAbstractUrlInterceptor::UrlString);
        argv[0] = &value;
        QMetaObject::metacall(_qobject, QMetaObject::WriteProperty, property->coreIndex, argv);
    }
    break;
    case QVariant::UInt: {
        Q_ASSERT(binding->type == QV4::CompiledData::Binding::Type_Number);
        double d = binding->valueAsNumber();
        uint value = uint(d);
        argv[0] = &value;
        QMetaObject::metacall(_qobject, QMetaObject::WriteProperty, property->coreIndex, argv);
        break;
    }
    break;
    case QVariant::Int: {
        Q_ASSERT(binding->type == QV4::CompiledData::Binding::Type_Number);
        double d = binding->valueAsNumber();
        int value = int(d);
        argv[0] = &value;
        QMetaObject::metacall(_qobject, QMetaObject::WriteProperty, property->coreIndex, argv);
        break;
    }
    break;
    case QMetaType::Float: {
        Q_ASSERT(binding->type == QV4::CompiledData::Binding::Type_Number);
        float value = float(binding->valueAsNumber());
        argv[0] = &value;
        QMetaObject::metacall(_qobject, QMetaObject::WriteProperty, property->coreIndex, argv);
    }
    break;
    case QVariant::Double: {
        Q_ASSERT(binding->type == QV4::CompiledData::Binding::Type_Number);
        double value = binding->valueAsNumber();
        argv[0] = &value;
        QMetaObject::metacall(_qobject, QMetaObject::WriteProperty, property->coreIndex, argv);
    }
    break;
    case QVariant::Color: {
        bool ok = false;
        uint colorValue = QQmlStringConverters::rgbaFromString(binding->valueAsString(qmlUnit), &ok);
        Q_ASSERT(ok);
        struct { void *data[4]; } buffer;
        if (QQml_valueTypeProvider()->storeValueType(property->propType, &colorValue, &buffer, sizeof(buffer))) {
            argv[0] = reinterpret_cast<void *>(&buffer);
            QMetaObject::metacall(_qobject, QMetaObject::WriteProperty, property->coreIndex, argv);
        }
    }
    break;
#ifndef QT_NO_DATESTRING
    case QVariant::Date: {
        bool ok = false;
        QDate value = QQmlStringConverters::dateFromString(binding->valueAsString(qmlUnit), &ok);
        Q_ASSERT(ok);
        argv[0] = &value;
        QMetaObject::metacall(_qobject, QMetaObject::WriteProperty, property->coreIndex, argv);
    }
    break;
    case QVariant::Time: {
        bool ok = false;
        QTime value = QQmlStringConverters::timeFromString(binding->valueAsString(qmlUnit), &ok);
        Q_ASSERT(ok);
        argv[0] = &value;
        QMetaObject::metacall(_qobject, QMetaObject::WriteProperty, property->coreIndex, argv);
    }
    break;
    case QVariant::DateTime: {
        bool ok = false;
        QDateTime value = QQmlStringConverters::dateTimeFromString(binding->valueAsString(qmlUnit), &ok);
        // ### VME compatibility :(
        {
            const qint64 date = value.date().toJulianDay();
            const int msecsSinceStartOfDay = value.time().msecsSinceStartOfDay();
            value = QDateTime(QDate::fromJulianDay(date), QTime::fromMSecsSinceStartOfDay(msecsSinceStartOfDay));
        }
        Q_ASSERT(ok);
        argv[0] = &value;
        QMetaObject::metacall(_qobject, QMetaObject::WriteProperty, property->coreIndex, argv);
    }
    break;
#endif // QT_NO_DATESTRING
    case QVariant::Point: {
        bool ok = false;
        QPoint value = QQmlStringConverters::pointFFromString(binding->valueAsString(qmlUnit), &ok).toPoint();
        Q_ASSERT(ok);
        argv[0] = &value;
        QMetaObject::metacall(_qobject, QMetaObject::WriteProperty, property->coreIndex, argv);
    }
    break;
    case QVariant::PointF: {
        bool ok = false;
        QPointF value = QQmlStringConverters::pointFFromString(binding->valueAsString(qmlUnit), &ok);
        Q_ASSERT(ok);
        argv[0] = &value;
        QMetaObject::metacall(_qobject, QMetaObject::WriteProperty, property->coreIndex, argv);
    }
    break;
    case QVariant::Size: {
        bool ok = false;
        QSize value = QQmlStringConverters::sizeFFromString(binding->valueAsString(qmlUnit), &ok).toSize();
        Q_ASSERT(ok);
        argv[0] = &value;
        QMetaObject::metacall(_qobject, QMetaObject::WriteProperty, property->coreIndex, argv);
    }
    break;
    case QVariant::SizeF: {
        bool ok = false;
        QSizeF value = QQmlStringConverters::sizeFFromString(binding->valueAsString(qmlUnit), &ok);
        Q_ASSERT(ok);
        argv[0] = &value;
        QMetaObject::metacall(_qobject, QMetaObject::WriteProperty, property->coreIndex, argv);
    }
    break;
    case QVariant::Rect: {
        bool ok = false;
        QRect value = QQmlStringConverters::rectFFromString(binding->valueAsString(qmlUnit), &ok).toRect();
        Q_ASSERT(ok);
        argv[0] = &value;
        QMetaObject::metacall(_qobject, QMetaObject::WriteProperty, property->coreIndex, argv);
    }
    break;
    case QVariant::RectF: {
        bool ok = false;
        QRectF value = QQmlStringConverters::rectFFromString(binding->valueAsString(qmlUnit), &ok);
        Q_ASSERT(ok);
        argv[0] = &value;
        QMetaObject::metacall(_qobject, QMetaObject::WriteProperty, property->coreIndex, argv);
    }
    break;
    case QVariant::Bool: {
        Q_ASSERT(binding->type == QV4::CompiledData::Binding::Type_Boolean);
        bool value = binding->valueAsBoolean();
        argv[0] = &value;
        QMetaObject::metacall(_qobject, QMetaObject::WriteProperty, property->coreIndex, argv);
    }
    break;
    case QVariant::Vector3D: {
        struct {
            float xp;
            float yp;
            float zy;
        } vec;
        bool ok = QQmlStringConverters::createFromString(QMetaType::QVector3D, binding->valueAsString(qmlUnit), &vec, sizeof(vec));
        Q_ASSERT(ok);
        Q_UNUSED(ok);
        argv[0] = reinterpret_cast<void *>(&vec);
        QMetaObject::metacall(_qobject, QMetaObject::WriteProperty, property->coreIndex, argv);
    }
    break;
    case QVariant::Vector4D: {
        struct {
            float xp;
            float yp;
            float zy;
            float wp;
        } vec;
        bool ok = QQmlStringConverters::createFromString(QMetaType::QVector4D, binding->valueAsString(qmlUnit), &vec, sizeof(vec));
        Q_ASSERT(ok);
        Q_UNUSED(ok);
        argv[0] = reinterpret_cast<void *>(&vec);
        QMetaObject::metacall(_qobject, QMetaObject::WriteProperty, property->coreIndex, argv);
    }
    break;
    case QVariant::RegExp:
        Q_ASSERT(!"not possible");
        break;
    default: {
        // generate single literal value assignment to a list property if required
        if (property->propType == qMetaTypeId<QList<qreal> >()) {
            Q_ASSERT(binding->type == QV4::CompiledData::Binding::Type_Number);
            QList<qreal> value;
            value.append(binding->valueAsNumber());
            argv[0] = reinterpret_cast<void *>(&value);
            QMetaObject::metacall(_qobject, QMetaObject::WriteProperty, property->coreIndex, argv);
            break;
        } else if (property->propType == qMetaTypeId<QList<int> >()) {
            Q_ASSERT(binding->type == QV4::CompiledData::Binding::Type_Number);
            double n = binding->valueAsNumber();
            QList<int> value;
            value.append(int(n));
            argv[0] = reinterpret_cast<void *>(&value);
            QMetaObject::metacall(_qobject, QMetaObject::WriteProperty, property->coreIndex, argv);
            break;
        } else if (property->propType == qMetaTypeId<QList<bool> >()) {
            Q_ASSERT(binding->type == QV4::CompiledData::Binding::Type_Boolean);
            QList<bool> value;
            value.append(binding->valueAsBoolean());
            argv[0] = reinterpret_cast<void *>(&value);
            QMetaObject::metacall(_qobject, QMetaObject::WriteProperty, property->coreIndex, argv);
            break;
        } else if (property->propType == qMetaTypeId<QList<QUrl> >()) {
            Q_ASSERT(binding->type == QV4::CompiledData::Binding::Type_String);
            QString urlString = binding->valueAsString(qmlUnit);
            QUrl u = urlString.isEmpty() ? QUrl() : compiledData->url().resolved(QUrl(urlString));
            QList<QUrl> value;
            value.append(u);
            argv[0] = reinterpret_cast<void *>(&value);
            QMetaObject::metacall(_qobject, QMetaObject::WriteProperty, property->coreIndex, argv);
            break;
        } else if (property->propType == qMetaTypeId<QList<QString> >()) {
            Q_ASSERT(binding->evaluatesToString());
            QList<QString> value;
            value.append(binding->valueAsString(qmlUnit));
            argv[0] = reinterpret_cast<void *>(&value);
            QMetaObject::metacall(_qobject, QMetaObject::WriteProperty, property->coreIndex, argv);
            break;
        } else if (property->propType == qMetaTypeId<QJSValue>()) {
            QJSValue value;
            if (binding->type == QV4::CompiledData::Binding::Type_Boolean) {
                value = QJSValue(binding->valueAsBoolean());
            } else if (binding->type == QV4::CompiledData::Binding::Type_Number) {
                double n = binding->valueAsNumber();
                if (double(int(n)) == n) {
                    value = QJSValue(int(n));
                } else
                    value = QJSValue(n);
            } else {
                value = QJSValue(binding->valueAsString(qmlUnit));
            }
            argv[0] = reinterpret_cast<void *>(&value);
            QMetaObject::metacall(_qobject, QMetaObject::WriteProperty, property->coreIndex, argv);
            break;
        }

        // otherwise, try a custom type assignment
        QString stringValue = binding->valueAsString(qmlUnit);
        QQmlMetaType::StringConverter converter = QQmlMetaType::customStringConverter(property->propType);
        Q_ASSERT(converter);
        QVariant value = (*converter)(stringValue);

        QMetaProperty metaProperty = _qobject->metaObject()->property(property->coreIndex);
        if (value.isNull() || ((int)metaProperty.type() != property->propType && metaProperty.userType() != property->propType)) {
            recordError(binding->location, tr("Cannot assign value %1 to property %2").arg(stringValue).arg(QString::fromUtf8(metaProperty.name())));
            break;
        }

        argv[0] = value.data();
        QMetaObject::metacall(_qobject, QMetaObject::WriteProperty, property->coreIndex, argv);
    }
    break;
    }
}

static QQmlType *qmlTypeForObject(QObject *object)
{
    QQmlType *type = 0;
    const QMetaObject *mo = object->metaObject();
    while (mo && !type) {
        type = QQmlMetaType::qmlType(mo);
        mo = mo->superClass();
    }
    return type;
}

void QQmlObjectCreator::setupBindings(const QBitArray &bindingsToSkip)
{
    QQmlListProperty<void> savedList;
    qSwap(_currentList, savedList);

    const QV4::CompiledData::BindingPropertyData &propertyData = compiledData->compilationUnit->bindingPropertyDataPerObject.at(_compiledObjectIndex);

    if (_compiledObject->idIndex) {
        const QQmlPropertyData *idProperty = propertyData.last();
        Q_ASSERT(!idProperty || !idProperty->isValid() || idProperty->name(_qobject) == QLatin1String("id"));
        if (idProperty && idProperty->isValid() && idProperty->isWritable() && idProperty->propType == QMetaType::QString) {
            QV4::CompiledData::Binding idBinding;
            idBinding.propertyNameIndex = 0; // Not used
            idBinding.flags = 0;
            idBinding.type = QV4::CompiledData::Binding::Type_String;
            idBinding.stringIndex = _compiledObject->idIndex;
            idBinding.location = _compiledObject->location; // ###
            setPropertyValue(idProperty, &idBinding);
        }
    }

    // ### this is best done through type-compile-time binding skip lists.
    if (_valueTypeProperty) {
        QQmlAbstractBinding *binding = QQmlPropertyPrivate::binding(_bindingTarget, _valueTypeProperty->coreIndex);

        if (binding && !binding->isValueTypeProxy()) {
            QQmlPropertyPrivate::removeBinding(_bindingTarget, _valueTypeProperty->coreIndex);
        } else if (binding) {
            QQmlValueTypeProxyBinding *proxy = static_cast<QQmlValueTypeProxyBinding *>(binding);

            if (qmlTypeForObject(_bindingTarget)) {
                quint32 bindingSkipList = 0;

                QQmlPropertyData *defaultProperty = _compiledObject->indexOfDefaultProperty != -1 ? _propertyCache->parent()->defaultProperty() : _propertyCache->defaultProperty();

                const QV4::CompiledData::Binding *binding = _compiledObject->bindingTable();
                for (quint32 i = 0; i < _compiledObject->nBindings; ++i, ++binding) {
                    QQmlPropertyData *property = binding->propertyNameIndex != 0 ? _propertyCache->property(stringAt(binding->propertyNameIndex), _qobject, context) : defaultProperty;
                    if (property)
                        bindingSkipList |= (1 << property->coreIndex);
                }

                proxy->removeBindings(bindingSkipList);
            }
        }
    }

    int currentListPropertyIndex = -1;

    const QV4::CompiledData::Binding *binding = _compiledObject->bindingTable();
    for (quint32 i = 0; i < _compiledObject->nBindings; ++i, ++binding) {
        if (static_cast<int>(i) < bindingsToSkip.size() && bindingsToSkip.testBit(i))
            continue;

        const QQmlPropertyData *property = propertyData.at(i);

        if (property && property->isQList()) {
            if (property->coreIndex != currentListPropertyIndex) {
                void *argv[1] = { (void*)&_currentList };
                QMetaObject::metacall(_qobject, QMetaObject::ReadProperty, property->coreIndex, argv);
                currentListPropertyIndex = property->coreIndex;
            }
        } else if (_currentList.object) {
            _currentList = QQmlListProperty<void>();
            currentListPropertyIndex = -1;
        }

        if (!setPropertyBinding(property, binding))
            return;
    }

    qSwap(_currentList, savedList);
}

bool QQmlObjectCreator::setPropertyBinding(const QQmlPropertyData *property, const QV4::CompiledData::Binding *binding)
{
    if (binding->type == QV4::CompiledData::Binding::Type_AttachedProperty) {
        Q_ASSERT(stringAt(qmlUnit->objectAt(binding->value.objectIndex)->inheritedTypeNameIndex).isEmpty());
        QQmlCompiledData::TypeReference *tr = resolvedTypes.value(binding->propertyNameIndex);
        Q_ASSERT(tr);
        QQmlType *attachedType = tr->type;
        if (!attachedType) {
            QQmlTypeNameCache::Result res = context->imports->query(stringAt(binding->propertyNameIndex));
            if (res.isValid())
                attachedType = res.type;
            else
                return false;
        }
        const int id = attachedType->attachedPropertiesId(QQmlEnginePrivate::get(engine));
        QObject *qmlObject = qmlAttachedPropertiesObjectById(id, _qobject);
        if (!populateInstance(binding->value.objectIndex, qmlObject, qmlObject, /*value type property*/0))
            return false;
        return true;
    }

    // ### resolve this at compile time
    if (property && property->propType == qMetaTypeId<QQmlScriptString>()) {
        QQmlScriptString ss(binding->valueAsScriptString(qmlUnit), context->asQQmlContext(), _scopeObject);
        ss.d.data()->bindingId = binding->type == QV4::CompiledData::Binding::Type_Script ? binding->value.compiledScriptIndex : (quint32)QQmlBinding::Invalid;
        ss.d.data()->lineNumber = binding->location.line;
        ss.d.data()->columnNumber = binding->location.column;
        ss.d.data()->isStringLiteral = binding->type == QV4::CompiledData::Binding::Type_String;
        ss.d.data()->isNumberLiteral = binding->type == QV4::CompiledData::Binding::Type_Number;
        ss.d.data()->numberValue = binding->valueAsNumber();

        QQmlPropertyPrivate::WriteFlags propertyWriteFlags = QQmlPropertyPrivate::BypassInterceptor |
                                                                   QQmlPropertyPrivate::RemoveBindingOnAliasWrite;
        int propertyWriteStatus = -1;
        void *argv[] = { &ss, 0, &propertyWriteStatus, &propertyWriteFlags };
        QMetaObject::metacall(_qobject, QMetaObject::WriteProperty, property->coreIndex, argv);
        return true;
    }

    QObject *createdSubObject = 0;
    if (binding->type == QV4::CompiledData::Binding::Type_Object) {
        createdSubObject = createInstance(binding->value.objectIndex, _bindingTarget);
        if (!createdSubObject)
            return false;
    }

    if (!property) // ### error
        return true;

    if (binding->type == QV4::CompiledData::Binding::Type_GroupProperty) {
        const QV4::CompiledData::Object *obj = qmlUnit->objectAt(binding->value.objectIndex);
        if (stringAt(obj->inheritedTypeNameIndex).isEmpty()) {

            QObject *groupObject = 0;
            QQmlValueType *valueType = 0;
            const QQmlPropertyData *valueTypeProperty = 0;
            QObject *bindingTarget = _bindingTarget;

            if (QQmlValueTypeFactory::isValueType(property->propType)) {
                valueType = QQmlValueTypeFactory::valueType(property->propType);
                if (!valueType) {
                    recordError(binding->location, tr("Cannot set properties on %1 as it is null").arg(stringAt(binding->propertyNameIndex)));
                    return false;
                }

                valueType->read(_qobject, property->coreIndex);

                groupObject = valueType;
                valueTypeProperty = property;
            } else {
                void *argv[1] = { &groupObject };
                QMetaObject::metacall(_qobject, QMetaObject::ReadProperty, property->coreIndex, argv);
                if (!groupObject) {
                    recordError(binding->location, tr("Cannot set properties on %1 as it is null").arg(stringAt(binding->propertyNameIndex)));
                    return false;
                }

                bindingTarget = groupObject;
            }

            if (!populateInstance(binding->value.objectIndex, groupObject, bindingTarget, valueTypeProperty))
                return false;

            if (valueType)
                valueType->write(_qobject, property->coreIndex, QQmlPropertyPrivate::BypassInterceptor);

            return true;
        }
    }

    if (_ddata->hasBindingBit(property->coreIndex) && !(binding->flags & QV4::CompiledData::Binding::IsSignalHandlerExpression)
        && !(binding->flags & QV4::CompiledData::Binding::IsOnAssignment)
        && !_valueTypeProperty)
        QQmlPropertyPrivate::removeBinding(_bindingTarget, property->coreIndex);

    if (binding->type == QV4::CompiledData::Binding::Type_Script) {
        QV4::Function *runtimeFunction = compiledData->compilationUnit->runtimeFunctions[binding->value.compiledScriptIndex];

        QV4::Scope scope(v4);
        QV4::ScopedContext qmlContext(scope, currentQmlContext());
        QV4::ScopedFunctionObject function(scope, QV4::FunctionObject::createScriptFunction(qmlContext, runtimeFunction, /*createProto*/ false));

        if (binding->flags & QV4::CompiledData::Binding::IsSignalHandlerExpression) {
            int signalIndex = _propertyCache->methodIndexToSignalIndex(property->coreIndex);
            QQmlBoundSignal *bs = new QQmlBoundSignal(_bindingTarget, signalIndex, _scopeObject, engine);
            QQmlBoundSignalExpression *expr = new QQmlBoundSignalExpression(_bindingTarget, signalIndex,
                                                                            context, _scopeObject, function);

            bs->takeExpression(expr);
        } else {
            QQmlBinding *qmlBinding = new QQmlBinding(function, _scopeObject, context);

            // When writing bindings to grouped properties implemented as value types,
            // such as point.x: { someExpression; }, then the binding is installed on
            // the point property (_qobjectForBindings) and after evaluating the expression,
            // the result is written to a value type virtual property, that contains the sub-index
            // of the "x" property.
            QQmlPropertyData targetCorePropertyData = *property;
            if (_valueTypeProperty)
                targetCorePropertyData = QQmlPropertyPrivate::saveValueType(*_valueTypeProperty, _qobject->metaObject(), property->coreIndex, engine);

            sharedState->allCreatedBindings.push(QQmlAbstractBinding::Ptr(qmlBinding));

            qmlBinding->setTarget(_bindingTarget, targetCorePropertyData);

            if (targetCorePropertyData.isAlias()) {
                QQmlPropertyPrivate::setBinding(qmlBinding, QQmlPropertyPrivate::DontEnable);
            } else {
                qmlBinding->addToObject();

                if (!_valueTypeProperty) {
                    QQmlData *targetDeclarativeData = QQmlData::get(_bindingTarget);
                    Q_ASSERT(targetDeclarativeData);
                    targetDeclarativeData->setPendingBindingBit(_bindingTarget, property->coreIndex);
                }
            }
        }
        return true;
    }

    if (binding->type == QV4::CompiledData::Binding::Type_Object) {
        if (binding->flags & QV4::CompiledData::Binding::IsOnAssignment) {
            // ### determine value source and interceptor casts ahead of time.
            QQmlType *type = qmlTypeForObject(createdSubObject);
            Q_ASSERT(type);

            QQmlPropertyData targetCorePropertyData = *property;
            if (_valueTypeProperty)
                targetCorePropertyData = QQmlPropertyPrivate::saveValueType(*_valueTypeProperty, _qobject->metaObject(), property->coreIndex, engine);

            int valueSourceCast = type->propertyValueSourceCast();
            if (valueSourceCast != -1) {
                QQmlPropertyValueSource *vs = reinterpret_cast<QQmlPropertyValueSource *>(reinterpret_cast<char *>(createdSubObject) + valueSourceCast);
                QObject *target = createdSubObject->parent();
                vs->setTarget(QQmlPropertyPrivate::restore(target, targetCorePropertyData, context));
                return true;
            }
            int valueInterceptorCast = type->propertyValueInterceptorCast();
            if (valueInterceptorCast != -1) {
                QQmlPropertyValueInterceptor *vi = reinterpret_cast<QQmlPropertyValueInterceptor *>(reinterpret_cast<char *>(createdSubObject) + valueInterceptorCast);
                QObject *target = createdSubObject->parent();

                if (targetCorePropertyData.isAlias()) {
                    int propIndex;
                    QQmlPropertyPrivate::findAliasTarget(target, targetCorePropertyData.coreIndex, &target, &propIndex);
                    QQmlData *data = QQmlData::get(target);
                    if (!data || !data->propertyCache) {
                        qWarning() << "can't resolve property alias for 'on' assignment";
                        return false;
                    }
                    targetCorePropertyData = *data->propertyCache->property(propIndex);
                }

                QQmlProperty prop =
                    QQmlPropertyPrivate::restore(target, targetCorePropertyData, context);

                vi->setTarget(prop);
                QQmlInterceptorMetaObject *mo = QQmlInterceptorMetaObject::get(target);
                if (!mo)
                    mo = new QQmlInterceptorMetaObject(target, QQmlData::get(target)->propertyCache);
                mo->registerInterceptor(prop.index(), QQmlPropertyPrivate::valueTypeCoreIndex(prop), vi);
                return true;
            }
            return false;
        }

        // Assigning object to signal property?
        if (binding->flags & QV4::CompiledData::Binding::IsSignalHandlerObject) {
            if (!property->isFunction()) {
                recordError(binding->valueLocation, tr("Cannot assign an object to signal property %1").arg(property->name(_qobject)));
                return false;
            }
            QMetaMethod method = QQmlMetaType::defaultMethod(createdSubObject);
            if (!method.isValid()) {
                recordError(binding->valueLocation, tr("Cannot assign object type %1 with no default method").arg(QString::fromLatin1(createdSubObject->metaObject()->className())));
                return false;
            }

            QMetaMethod signalMethod = _qobject->metaObject()->method(property->coreIndex);
            if (!QMetaObject::checkConnectArgs(signalMethod, method)) {
                recordError(binding->valueLocation,
                            tr("Cannot connect mismatched signal/slot %1 %vs. %2")
                            .arg(QString::fromUtf8(method.methodSignature()))
                            .arg(QString::fromUtf8(signalMethod.methodSignature())));
                return false;
            }

            QQmlPropertyPrivate::connect(_qobject, property->coreIndex, createdSubObject, method.methodIndex());
            return true;
        }

        QQmlPropertyPrivate::WriteFlags propertyWriteFlags = QQmlPropertyPrivate::BypassInterceptor |
                                                                   QQmlPropertyPrivate::RemoveBindingOnAliasWrite;
        int propertyWriteStatus = -1;
        void *argv[] = { 0, 0, &propertyWriteStatus, &propertyWriteFlags };

        if (const char *iid = QQmlMetaType::interfaceIId(property->propType)) {
            void *ptr = createdSubObject->qt_metacast(iid);
            if (ptr) {
                argv[0] = &ptr;
                QMetaObject::metacall(_qobject, QMetaObject::WriteProperty, property->coreIndex, argv);
            } else {
                recordError(binding->location, tr("Cannot assign object to interface property"));
                return false;
            }
        } else if (property->propType == QMetaType::QVariant) {
            if (property->isVarProperty()) {
                QV4::Scope scope(v4);
                QV4::ScopedValue wrappedObject(scope, QV4::QObjectWrapper::wrap(QV8Engine::getV4(engine), createdSubObject));
                _vmeMetaObject->setVMEProperty(property->coreIndex, wrappedObject);
            } else {
                QVariant value = QVariant::fromValue(createdSubObject);
                argv[0] = &value;
                QMetaObject::metacall(_qobject, QMetaObject::WriteProperty, property->coreIndex, argv);
            }
        } else if (property->isQList()) {
            Q_ASSERT(_currentList.object);

            void *itemToAdd = createdSubObject;

            const char *iid = 0;
            int listItemType = QQmlEnginePrivate::get(engine)->listType(property->propType);
            if (listItemType != -1)
                iid = QQmlMetaType::interfaceIId(listItemType);
            if (iid)
                itemToAdd = createdSubObject->qt_metacast(iid);

            if (_currentList.append)
                _currentList.append(&_currentList, itemToAdd);
            else {
                recordError(binding->location, tr("Cannot assign object to read only list"));
                return false;
            }

        } else {
            // pointer compatibility was tested in QQmlPropertyValidator at type compile time
            argv[0] = &createdSubObject;
            QMetaObject::metacall(_qobject, QMetaObject::WriteProperty, property->coreIndex, argv);
        }
        return true;
    }

    if (property->isQList()) {
        recordError(binding->location, tr("Cannot assign primitives to lists"));
        return false;
    }

    setPropertyValue(property, binding);
    return true;
}

void QQmlObjectCreator::setupFunctions()
{
    QV4::Scope scope(v4);
    QV4::ScopedValue function(scope);
    QV4::ScopedContext qmlContext(scope, currentQmlContext());

    const quint32 *functionIdx = _compiledObject->functionOffsetTable();
    for (quint32 i = 0; i < _compiledObject->nFunctions; ++i, ++functionIdx) {
        QV4::Function *runtimeFunction = compiledData->compilationUnit->runtimeFunctions[*functionIdx];
        const QString name = runtimeFunction->name()->toQString();

        QQmlPropertyData *property = _propertyCache->property(name, _qobject, context);
        if (!property->isVMEFunction())
            continue;

        function = QV4::FunctionObject::createScriptFunction(qmlContext, runtimeFunction);
        _vmeMetaObject->setVmeMethod(property->coreIndex, function);
    }
}

void QQmlObjectCreator::recordError(const QV4::CompiledData::Location &location, const QString &description)
{
    QQmlError error;
    error.setUrl(compiledData->url());
    error.setLine(location.line);
    error.setColumn(location.column);
    error.setDescription(description);
    errors << error;
}

void QQmlObjectCreator::registerObjectWithContextById(int objectIndex, QObject *instance) const
{
    QHash<int, int>::ConstIterator idEntry = objectIndexToId.find(objectIndex);
    if (idEntry != objectIndexToId.constEnd())
        context->setIdProperty(idEntry.value(), instance);
}

QV4::Heap::QmlContext *QQmlObjectCreator::currentQmlContext()
{
    if (!_qmlContext->objectValue())
        _qmlContext->setM(v4->rootContext()->newQmlContext(context, _scopeObject));

    return _qmlContext->d();
}

QObject *QQmlObjectCreator::createInstance(int index, QObject *parent, bool isContextObject)
{
    QQmlObjectCreationProfiler profiler(sharedState->profiler.profiler);
    ActiveOCRestorer ocRestorer(this, QQmlEnginePrivate::get(engine));

    bool isComponent = false;
    QObject *instance = 0;
    QQmlData *ddata = 0;
    QQmlCustomParser *customParser = 0;
    QQmlParserStatus *parserStatus = 0;
    bool installPropertyCache = true;

    const QV4::CompiledData::Object *obj = qmlUnit->objectAt(index);
    if (compiledData->isComponent(index)) {
        isComponent = true;
        QQmlComponent *component = new QQmlComponent(engine, compiledData, index, parent);
        Q_QML_OC_PROFILE(sharedState->profiler, profiler.update(
                             compiledData, obj, QStringLiteral("<component>"), context->url()));
        QQmlComponentPrivate::get(component)->creationContext = context;
        instance = component;
        ddata = QQmlData::get(instance, /*create*/true);
    } else {
        QQmlCompiledData::TypeReference *typeRef = resolvedTypes.value(obj->inheritedTypeNameIndex);
        Q_ASSERT(typeRef);
        installPropertyCache = !typeRef->isFullyDynamicType;
        QQmlType *type = typeRef->type;
        if (type) {
            Q_QML_OC_PROFILE(sharedState->profiler, profiler.update(
                                 compiledData, obj, type->qmlTypeName(), context->url()));
            instance = type->create();
            if (!instance) {
                recordError(obj->location, tr("Unable to create object of type %1").arg(stringAt(obj->inheritedTypeNameIndex)));
                return 0;
            }

            const int parserStatusCast = type->parserStatusCast();
            if (parserStatusCast != -1)
                parserStatus = reinterpret_cast<QQmlParserStatus*>(reinterpret_cast<char *>(instance) + parserStatusCast);

            customParser = type->customParser();

            if (sharedState->rootContext && sharedState->rootContext->isRootObjectInCreation) {
                QQmlData *ddata = QQmlData::get(instance, /*create*/true);
                ddata->rootObjectInCreation = true;
                sharedState->rootContext->isRootObjectInCreation = false;
            }

            sharedState->allCreatedObjects.push(instance);
        } else {
            Q_ASSERT(typeRef->component);
            Q_QML_OC_PROFILE(sharedState->profiler, profiler.update(
                                 compiledData, obj, typeRef->component->fileName(),
                                 context->url()));
            if (typeRef->component->compilationUnit->data->isSingleton())
            {
                recordError(obj->location, tr("Composite Singleton Type %1 is not creatable").arg(stringAt(obj->inheritedTypeNameIndex)));
                return 0;
            }

            QQmlObjectCreator subCreator(context, typeRef->component, sharedState.data());
            instance = subCreator.create();
            if (!instance) {
                errors += subCreator.errors;
                return 0;
            }
        }
        if (parent)
            QQml_setParent_noEvent(instance, parent);

        ddata = QQmlData::get(instance, /*create*/true);
        ddata->lineNumber = obj->location.line;
        ddata->columnNumber = obj->location.column;
    }

    ddata->setImplicitDestructible();
    if (static_cast<quint32>(index) == qmlUnit->indexOfRootObject || ddata->rootObjectInCreation) {
        if (ddata->context) {
            Q_ASSERT(ddata->context != context);
            Q_ASSERT(ddata->outerContext);
            Q_ASSERT(ddata->outerContext != context);
            QQmlContextData *c = ddata->context;
            while (c->linkedContext) c = c->linkedContext;
            c->linkedContext = context;
        } else
            context->addObject(instance);
        ddata->ownContext = true;
    } else if (!ddata->context)
        context->addObject(instance);

    ddata->outerContext = context;

    if (parserStatus) {
        parserStatus->classBegin();
        // push() the profiler state here, together with the parserStatus, as we'll pop() them
        // together, too.
        Q_QML_OC_PROFILE(sharedState->profiler, sharedState->profiler.push(obj));
        sharedState->allParserStatusCallbacks.push(parserStatus);
        parserStatus->d = &sharedState->allParserStatusCallbacks.top();
    }

    // Register the context object in the context early on in order for pending binding
    // initialization to find it available.
    if (isContextObject)
        context->contextObject = instance;

    QBitArray bindingsToSkip;
    if (customParser) {
        QHash<int, QBitArray>::ConstIterator customParserBindings = compiledData->customParserBindings.constFind(index);
        if (customParserBindings != compiledData->customParserBindings.constEnd()) {
            customParser->engine = QQmlEnginePrivate::get(engine);
            customParser->imports = compiledData->importCache;

            QList<const QV4::CompiledData::Binding *> bindings;
            const QV4::CompiledData::Object *obj = qmlUnit->objectAt(index);
            for (int i = 0; i < customParserBindings->count(); ++i)
                if (customParserBindings->testBit(i))
                    bindings << obj->bindingTable() + i;
            customParser->applyBindings(instance, compiledData, bindings);

            customParser->engine = 0;
            customParser->imports = (QQmlTypeNameCache*)0;
            bindingsToSkip = *customParserBindings;
        }
    }

    if (isComponent) {
        registerObjectWithContextById(index, instance);
        return instance;
    }

    QQmlRefPointer<QQmlPropertyCache> cache = propertyCaches.at(index);
    Q_ASSERT(!cache.isNull());
    if (installPropertyCache) {
        if (ddata->propertyCache)
            ddata->propertyCache->release();;
        ddata->propertyCache = cache;
        ddata->propertyCache->addref();
    }

    QObject *scopeObject = instance;
    qSwap(_scopeObject, scopeObject);

    Q_ASSERT(sharedState->allJavaScriptObjects);
    *sharedState->allJavaScriptObjects = QV4::QObjectWrapper::wrap(v4, instance);
    ++sharedState->allJavaScriptObjects;

    QV4::Scope valueScope(v4);
    QV4::QmlContext *qmlContext = static_cast<QV4::QmlContext *>(valueScope.alloc(1));

    qSwap(_qmlContext, qmlContext);

    bool result = populateInstance(index, instance, /*binding target*/instance, /*value type property*/0, bindingsToSkip);

    qSwap(_qmlContext, qmlContext);
    qSwap(_scopeObject, scopeObject);

    return result ? instance : 0;
}

QQmlContextData *QQmlObjectCreator::finalize(QQmlInstantiationInterrupt &interrupt)
{
    Q_ASSERT(phase == ObjectsCreated || phase == Finalizing);
    phase = Finalizing;

    QQmlObjectCreatorRecursionWatcher watcher(this);
    ActiveOCRestorer ocRestorer(this, QQmlEnginePrivate::get(engine));

    while (!sharedState->allCreatedBindings.isEmpty()) {
        QQmlAbstractBinding::Ptr b = sharedState->allCreatedBindings.pop();
        Q_ASSERT(b);
        // skip, if b is not added to an object
        if (!b->isAddedToObject())
            continue;
        QQmlData *data = QQmlData::get(b->targetObject());
        Q_ASSERT(data);
        data->clearPendingBindingBit(b->targetPropertyIndex());
        b->setEnabled(true, QQmlPropertyPrivate::BypassInterceptor |
                      QQmlPropertyPrivate::DontRemoveBinding);

        if (watcher.hasRecursed() || interrupt.shouldInterrupt())
            return 0;
    }

    if (QQmlVME::componentCompleteEnabled()) { // the qml designer does the component complete later
        while (!sharedState->allParserStatusCallbacks.isEmpty()) {
            QQmlObjectCompletionProfiler profiler(&sharedState->profiler);
            QQmlParserStatus *status = sharedState->allParserStatusCallbacks.pop();

            if (status && status->d) {
                status->d = 0;
                status->componentComplete();
            }

            if (watcher.hasRecursed() || interrupt.shouldInterrupt())
                return 0;
        }
    }

    for (int ii = 0; ii < sharedState->finalizeCallbacks.count(); ++ii) {
        QQmlEnginePrivate::FinalizeCallback callback = sharedState->finalizeCallbacks.at(ii);
        QObject *obj = callback.first;
        if (obj) {
            void *args[] = { 0 };
            QMetaObject::metacall(obj, QMetaObject::InvokeMetaMethod, callback.second, args);
        }
        if (watcher.hasRecursed())
            return 0;
    }
    sharedState->finalizeCallbacks.clear();

    while (sharedState->componentAttached) {
        QQmlComponentAttached *a = sharedState->componentAttached;
        a->rem();
        QQmlData *d = QQmlData::get(a->parent());
        Q_ASSERT(d);
        Q_ASSERT(d->context);
        a->add(&d->context->componentAttached);
        if (QQmlVME::componentCompleteEnabled())
            emit a->completed();

        if (watcher.hasRecursed() || interrupt.shouldInterrupt())
            return 0;
    }

    phase = Done;

    return sharedState->rootContext;
}

void QQmlObjectCreator::clear()
{
    if (phase == Done || phase == Finalizing || phase == Startup)
        return;
    Q_ASSERT(phase != Startup);

    while (!sharedState->allCreatedObjects.isEmpty())
        delete sharedState->allCreatedObjects.pop();

    while (sharedState->componentAttached) {
        QQmlComponentAttached *a = sharedState->componentAttached;
        a->rem();
    }

    phase = Done;
}

bool QQmlObjectCreator::populateInstance(int index, QObject *instance, QObject *bindingTarget, const QQmlPropertyData *valueTypeProperty, const QBitArray &bindingsToSkip)
{
    QQmlData *declarativeData = QQmlData::get(instance, /*create*/true);

    qSwap(_qobject, instance);
    qSwap(_valueTypeProperty, valueTypeProperty);
    qSwap(_compiledObjectIndex, index);
    const QV4::CompiledData::Object *obj = qmlUnit->objectAt(_compiledObjectIndex);
    qSwap(_compiledObject, obj);
    qSwap(_ddata, declarativeData);
    qSwap(_bindingTarget, bindingTarget);

    QV4::Scope valueScope(v4);
    QV4::ScopedValue scopeObjectProtector(valueScope);

    QQmlRefPointer<QQmlPropertyCache> cache = propertyCaches.at(_compiledObjectIndex);

    QQmlVMEMetaObject *vmeMetaObject = 0;
    const QByteArray data = vmeMetaObjectData.value(_compiledObjectIndex);
    if (!data.isEmpty()) {
        Q_ASSERT(!cache.isNull());
        // install on _object
        vmeMetaObject = new QQmlVMEMetaObject(_qobject, cache, reinterpret_cast<const QQmlVMEMetaData*>(data.constData()));
        if (_ddata->propertyCache)
            _ddata->propertyCache->release();
        _ddata->propertyCache = cache;
        _ddata->propertyCache->addref();
        scopeObjectProtector = _ddata->jsWrapper.value();
    } else {
        vmeMetaObject = QQmlVMEMetaObject::get(_qobject);
    }

    registerObjectWithContextById(_compiledObjectIndex, _qobject);

    qSwap(_propertyCache, cache);
    qSwap(_vmeMetaObject, vmeMetaObject);

    QBitArray bindingSkipList = bindingsToSkip;
    {
        QHash<int, QBitArray>::ConstIterator deferredBindings = compiledData->deferredBindingsPerObject.constFind(_compiledObjectIndex);
        if (deferredBindings != compiledData->deferredBindingsPerObject.constEnd()) {
            if (bindingSkipList.isEmpty())
                bindingSkipList.resize(deferredBindings->count());

            for (int i = 0; i < deferredBindings->count(); ++i)
                if (deferredBindings->testBit(i))
                    bindingSkipList.setBit(i);
            QQmlData::DeferredData *deferData = new QQmlData::DeferredData;
            deferData->deferredIdx = _compiledObjectIndex;
            deferData->compiledData = compiledData;
            deferData->compiledData->addref();
            deferData->context = context;
            _ddata->deferredData = deferData;
        }
    }

    if (_compiledObject->nFunctions > 0)
        setupFunctions();
    setupBindings(bindingSkipList);

    qSwap(_vmeMetaObject, vmeMetaObject);
    qSwap(_bindingTarget, bindingTarget);
    qSwap(_ddata, declarativeData);
    qSwap(_compiledObject, obj);
    qSwap(_compiledObjectIndex, index);
    qSwap(_valueTypeProperty, valueTypeProperty);
    qSwap(_qobject, instance);
    qSwap(_propertyCache, cache);

    return errors.isEmpty();
}




QQmlObjectCreatorRecursionWatcher::QQmlObjectCreatorRecursionWatcher(QQmlObjectCreator *creator)
    : sharedState(creator->sharedState)
    , watcher(creator->sharedState.data())
{
}
