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
#include <private/qqmlbinding_p.h>
#include <private/qqmlstringconverters_p.h>
#include <private/qqmlboundsignal_p.h>
#include <private/qqmlcomponentattached_p.h>
#include <private/qqmlcomponent_p.h>
#include <private/qqmlcustomparser_p.h>
#include <private/qqmlscriptstring_p.h>
#include <private/qqmlpropertyvalueinterceptor_p.h>
#include <private/qqmlvaluetypeproxybinding_p.h>
#include <private/qqmldebugconnector_p.h>
#include <private/qqmldebugserviceinterfaces_p.h>
#include <private/qqmlscriptdata_p.h>
#include <private/qqmlsourcecoordinate_p.h>
#include <private/qjsvalue_p.h>
#include <private/qv4generatorobject_p.h>
#include <private/qv4resolvedtypereference_p.h>
#include <private/qqmlpropertybinding_p.h>

#include <QScopedValueRollback>

#include <qtqml_tracepoints_p.h>
#include <QScopedValueRollback>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcQmlDefaultMethod, "qt.qml.defaultmethod")

QT_USE_NAMESPACE

QQmlObjectCreator::QQmlObjectCreator(
        QQmlRefPointer<QQmlContextData> parentContext,
        const QQmlRefPointer<QV4::ExecutableCompilationUnit> &compilationUnit,
        const QQmlRefPointer<QQmlContextData> &creationContext,
        QQmlIncubatorPrivate *incubator)
    : phase(Startup)
    , compilationUnit(compilationUnit)
    , propertyCaches(&compilationUnit->propertyCaches)
    , sharedState(new QQmlObjectCreatorSharedState, QQmlRefPointer<QQmlObjectCreatorSharedState>::Adopt)
    , topLevelCreator(true)
    , incubator(incubator)
{
    init(std::move(parentContext));

    sharedState->componentAttached = nullptr;
    sharedState->allCreatedBindings.allocate(compilationUnit->totalBindingsCount());
    sharedState->allParserStatusCallbacks.allocate(compilationUnit->totalParserStatusCount());
    sharedState->allCreatedObjects.allocate(compilationUnit->totalObjectCount());
    sharedState->allJavaScriptObjects = nullptr;
    sharedState->creationContext = creationContext;
    sharedState->rootContext = nullptr;
    sharedState->hadRequiredProperties = false;

    if (auto profiler = QQmlEnginePrivate::get(engine)->profiler) {
        Q_QML_PROFILE_IF_ENABLED(QQmlProfilerDefinitions::ProfileCreating, profiler,
                sharedState->profiler.init(profiler, compilationUnit->totalParserStatusCount()));
    } else {
        Q_UNUSED(profiler);
    }
}

QQmlObjectCreator::QQmlObjectCreator(
        QQmlRefPointer<QQmlContextData> parentContext,
        const QQmlRefPointer<QV4::ExecutableCompilationUnit> &compilationUnit,
        QQmlObjectCreatorSharedState *inheritedSharedState)
    : phase(Startup)
    , compilationUnit(compilationUnit)
    , propertyCaches(&compilationUnit->propertyCaches)
    , sharedState(inheritedSharedState)
    , topLevelCreator(false)
    , incubator(nullptr)
{
    init(std::move(parentContext));
}

void QQmlObjectCreator::init(QQmlRefPointer<QQmlContextData> providedParentContext)
{
    parentContext = std::move(providedParentContext);
    engine = parentContext->engine();
    v4 = engine->handle();

    if (compilationUnit && !compilationUnit->engine)
        compilationUnit->linkToEngine(v4);

    qmlUnit = compilationUnit->unitData();
    context = nullptr;
    _qobject = nullptr;
    _scopeObject = nullptr;
    _bindingTarget = nullptr;
    _valueTypeProperty = nullptr;
    _compiledObject = nullptr;
    _compiledObjectIndex = -1;
    _ddata = nullptr;
    _propertyCache = nullptr;
    _vmeMetaObject = nullptr;
    _qmlContext = nullptr;
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
                ps->d = nullptr;
        }
        while (sharedState->componentAttached) {
            QQmlComponentAttached *a = sharedState->componentAttached;
            a->removeFromList();
        }
    }
}

QObject *QQmlObjectCreator::create(int subComponentIndex, QObject *parent, QQmlInstantiationInterrupt *interrupt, int flags)
{
    if (phase == CreatingObjectsPhase2) {
        phase = ObjectsCreated;
        return context->contextObject();
    }
    Q_ASSERT(phase == Startup);
    phase = CreatingObjects;

    int objectToCreate;
    bool isComponentRoot = false; // either a "real" component of or an inline component

    if (subComponentIndex == -1) {
        objectToCreate = /*root object*/0;
        isComponentRoot = true;
    } else {
        Q_ASSERT(subComponentIndex >= 0);
        if (flags & CreationFlags::InlineComponent) {
            objectToCreate = subComponentIndex;
            isComponentRoot = true;
        } else {
            Q_ASSERT(flags & CreationFlags::NormalObject);
            const QV4::CompiledData::Object *compObj = compilationUnit->objectAt(subComponentIndex);
            objectToCreate = compObj->bindingTable()->value.objectIndex;
        }
    }

    context = QQmlContextData::createRefCounted(parentContext);
    context->setInternal(true);
    context->setImports(compilationUnit->typeNameCache);
    context->initFromTypeCompilationUnit(compilationUnit, subComponentIndex);

    if (!sharedState->rootContext) {
        sharedState->rootContext = context;
        sharedState->rootContext->setIncubator(incubator);
        sharedState->rootContext->setRootObjectInCreation(true);
    }

    QV4::Scope scope(v4);

    Q_ASSERT(sharedState->allJavaScriptObjects || topLevelCreator);
    if (topLevelCreator)
        sharedState->allJavaScriptObjects = scope.alloc(compilationUnit->totalObjectCount());

    if (isComponentRoot && compilationUnit->dependentScripts.count()) {
        QV4::ScopedObject scripts(scope, v4->newArrayObject(compilationUnit->dependentScripts.count()));
        context->setImportedScripts(QV4::PersistentValue(v4, scripts.asReturnedValue()));
        QV4::ScopedValue v(scope);
        for (int i = 0; i < compilationUnit->dependentScripts.count(); ++i) {
            QQmlRefPointer<QQmlScriptData> s = compilationUnit->dependentScripts.at(i);
            scripts->put(i, (v = s->scriptValueForContext(context)));
        }
    } else if (sharedState->creationContext) {
        context->setImportedScripts(sharedState->creationContext->importedScripts());
    }

    QObject *instance = createInstance(objectToCreate, parent, /*isContextObject*/true);
    if (instance) {
        QQmlData *ddata = QQmlData::get(instance);
        Q_ASSERT(ddata);
        ddata->compilationUnit = compilationUnit;
    }

    if (topLevelCreator)
        sharedState->allJavaScriptObjects = nullptr;

    phase = CreatingObjectsPhase2;

    if (interrupt && interrupt->shouldInterrupt())
        return nullptr;

    phase = ObjectsCreated;

    if (instance) {
        if (QQmlEngineDebugService *service
                = QQmlDebugConnector::service<QQmlEngineDebugService>()) {
            if (!parentContext->isInternal())
                parentContext->asQQmlContextPrivate()->appendInstance(instance);
            service->objectCreated(engine, instance);
        } else if (!parentContext->isInternal() && QQmlDebugConnector::service<QV4DebugService>()) {
            parentContext->asQQmlContextPrivate()->appendInstance(instance);
        }
    }

    return instance;
}

void QQmlObjectCreator::beginPopulateDeferred(const QQmlRefPointer<QQmlContextData> &newContext)
{
    context = newContext;
    sharedState->rootContext = newContext;

    Q_ASSERT(topLevelCreator);
    Q_ASSERT(!sharedState->allJavaScriptObjects);

    QV4::Scope valueScope(v4);
    sharedState->allJavaScriptObjects = valueScope.alloc(compilationUnit->totalObjectCount());
}

void QQmlObjectCreator::populateDeferred(QObject *instance, int deferredIndex,
                                         const QQmlPropertyPrivate *qmlProperty,
                                         const QV4::CompiledData::Binding *binding)
{
    QQmlData *declarativeData = QQmlData::get(instance);
    QObject *bindingTarget = instance;

    QQmlRefPointer<QQmlPropertyCache> cache = declarativeData->propertyCache;
    QQmlVMEMetaObject *vmeMetaObject = QQmlVMEMetaObject::get(instance);

    QObject *scopeObject = instance;
    qSwap(_scopeObject, scopeObject);

    QV4::Scope valueScope(v4);
    QScopedValueRollback<QV4::Value*> jsObjectGuard(sharedState->allJavaScriptObjects,
                                                    valueScope.alloc(compilationUnit->totalObjectCount()));

    Q_ASSERT(topLevelCreator);
    QV4::QmlContext *qmlContext = static_cast<QV4::QmlContext *>(valueScope.alloc());

    qSwap(_qmlContext, qmlContext);

    qSwap(_propertyCache, cache);
    qSwap(_qobject, instance);

    int objectIndex = deferredIndex;
    qSwap(_compiledObjectIndex, objectIndex);

    const QV4::CompiledData::Object *obj = compilationUnit->objectAt(_compiledObjectIndex);
    qSwap(_compiledObject, obj);
    qSwap(_ddata, declarativeData);
    qSwap(_bindingTarget, bindingTarget);
    qSwap(_vmeMetaObject, vmeMetaObject);

    if (binding) {
        Q_ASSERT(qmlProperty);
        Q_ASSERT(binding->flags & QV4::CompiledData::Binding::IsDeferredBinding);

        QQmlListProperty<void> savedList;
        qSwap(_currentList, savedList);

        const QQmlPropertyData &property = qmlProperty->core;

        if (property.isQList()) {
            void *argv[1] = { (void*)&_currentList };
            QMetaObject::metacall(_qobject, QMetaObject::ReadProperty, property.coreIndex(), argv);
        } else if (_currentList.object) {
            _currentList = QQmlListProperty<void>();
        }

        setPropertyBinding(&property, binding);

        qSwap(_currentList, savedList);
    } else {
        setupBindings(/*applyDeferredBindings=*/true);
    }

    qSwap(_vmeMetaObject, vmeMetaObject);
    qSwap(_bindingTarget, bindingTarget);
    qSwap(_ddata, declarativeData);
    qSwap(_compiledObject, obj);
    qSwap(_compiledObjectIndex, objectIndex);
    qSwap(_qobject, instance);
    qSwap(_propertyCache, cache);

    qSwap(_qmlContext, qmlContext);
    qSwap(_scopeObject, scopeObject);
}

bool QQmlObjectCreator::populateDeferredProperties(QObject *instance,
                                                   const QQmlData::DeferredData *deferredData)
{
    beginPopulateDeferred(deferredData->context);
    populateDeferred(instance, deferredData->deferredIdx);
    finalizePopulateDeferred();
    return errors.isEmpty();
}

void QQmlObjectCreator::populateDeferredBinding(const QQmlProperty &qmlProperty, int deferredIndex,
                                                const QV4::CompiledData::Binding *binding)
{
    populateDeferred(qmlProperty.object(), deferredIndex, QQmlPropertyPrivate::get(qmlProperty),
                     binding);
}

void QQmlObjectCreator::finalizePopulateDeferred()
{
    phase = ObjectsCreated;
}

void QQmlObjectCreator::setPropertyValue(const QQmlPropertyData *property, const QV4::CompiledData::Binding *binding)
{
    QQmlPropertyData::WriteFlags propertyWriteFlags = QQmlPropertyData::BypassInterceptor | QQmlPropertyData::RemoveBindingOnAliasWrite;
    QV4::Scope scope(v4);

    int propertyType = property->propType().id();

    if (property->isEnum()) {
        if (binding->flags & QV4::CompiledData::Binding::IsResolvedEnum) {
            propertyType = QMetaType::Int;
        } else {
            // ### This should be resolved earlier at compile time and the binding value should be changed accordingly.
            QVariant value = compilationUnit->bindingValueAsString(binding);
            bool ok = QQmlPropertyPrivate::write(_qobject, *property, value, context);
            Q_ASSERT(ok);
            Q_UNUSED(ok);
            return;
        }
    }

    auto assertOrNull = [&](bool ok)
    {
        Q_ASSERT(ok || binding->type == QV4::CompiledData::Binding::Type_Null);
        Q_UNUSED(ok);
    };

    auto assertType = [&](QV4::CompiledData::Binding::ValueType type)
    {
        Q_ASSERT(binding->type == type || binding->type == QV4::CompiledData::Binding::Type_Null);
        Q_UNUSED(type);
    };

    if (property->isQObject()) {
        if (binding->type == QV4::CompiledData::Binding::Type_Null) {
            QObject *value = nullptr;
            const bool ok = property->writeProperty(_qobject, &value, propertyWriteFlags);
            Q_ASSERT(ok);
            Q_UNUSED(ok);
            return;
        }
    }

    switch (propertyType) {
    case QMetaType::QVariant: {
        if (binding->type == QV4::CompiledData::Binding::Type_Number) {
            double n = compilationUnit->bindingValueAsNumber(binding);
            if (double(int(n)) == n) {
                if (property->isVarProperty()) {
                    _vmeMetaObject->setVMEProperty(property->coreIndex(), QV4::Value::fromInt32(int(n)));
                } else {
                    int i = int(n);
                    QVariant value(i);
                    property->writeProperty(_qobject, &value, propertyWriteFlags);
                }
            } else {
                if (property->isVarProperty()) {
                    _vmeMetaObject->setVMEProperty(property->coreIndex(), QV4::Value::fromDouble(n));
                } else {
                    QVariant value(n);
                    property->writeProperty(_qobject, &value, propertyWriteFlags);
                }
            }
        } else if (binding->type == QV4::CompiledData::Binding::Type_Boolean) {
            if (property->isVarProperty()) {
                _vmeMetaObject->setVMEProperty(property->coreIndex(), QV4::Value::fromBoolean(binding->valueAsBoolean()));
            } else {
                QVariant value(binding->valueAsBoolean());
                property->writeProperty(_qobject, &value, propertyWriteFlags);
            }
        } else if (binding->type == QV4::CompiledData::Binding::Type_Null) {
            if (property->isVarProperty()) {
                _vmeMetaObject->setVMEProperty(property->coreIndex(), QV4::Value::nullValue());
            } else {
                QVariant nullValue = QVariant::fromValue(nullptr);
                property->writeProperty(_qobject, &nullValue, propertyWriteFlags);
            }
        } else {
            QString stringValue = compilationUnit->bindingValueAsString(binding);
            if (property->isVarProperty()) {
                QV4::ScopedString s(scope, v4->newString(stringValue));
                _vmeMetaObject->setVMEProperty(property->coreIndex(), s);
            } else {
                QVariant value = stringValue;
                property->writeProperty(_qobject, &value, propertyWriteFlags);
            }
        }
    }
    break;
    case QMetaType::QString: {
        assertOrNull(binding->evaluatesToString());
        QString value = compilationUnit->bindingValueAsString(binding);
        property->writeProperty(_qobject, &value, propertyWriteFlags);
    }
    break;
    case QMetaType::QStringList: {
        assertOrNull(binding->evaluatesToString());
        QStringList value(compilationUnit->bindingValueAsString(binding));
        property->writeProperty(_qobject, &value, propertyWriteFlags);
    }
    break;
    case QMetaType::QByteArray: {
        assertType(QV4::CompiledData::Binding::Type_String);
        QByteArray value(compilationUnit->bindingValueAsString(binding).toUtf8());
        property->writeProperty(_qobject, &value, propertyWriteFlags);
    }
    break;
    case QMetaType::QUrl: {
        assertType(QV4::CompiledData::Binding::Type_String);
        const QString string = compilationUnit->bindingValueAsString(binding);
        QUrl value = (!string.isEmpty() && QQmlPropertyPrivate::resolveUrlsOnAssignment())
                ? compilationUnit->finalUrl().resolved(QUrl(string))
                : QUrl(string);
        property->writeProperty(_qobject, &value, propertyWriteFlags);
    }
    break;
    case QMetaType::UInt: {
        assertType(QV4::CompiledData::Binding::Type_Number);
        double d = compilationUnit->bindingValueAsNumber(binding);
        uint value = uint(d);
        property->writeProperty(_qobject, &value, propertyWriteFlags);
        break;
    }
    break;
    case QMetaType::Int: {
        assertType(QV4::CompiledData::Binding::Type_Number);
        double d = compilationUnit->bindingValueAsNumber(binding);
        int value = int(d);
        property->writeProperty(_qobject, &value, propertyWriteFlags);
        break;
    }
    break;
    case QMetaType::Float: {
        assertType(QV4::CompiledData::Binding::Type_Number);
        float value = float(compilationUnit->bindingValueAsNumber(binding));
        property->writeProperty(_qobject, &value, propertyWriteFlags);
    }
    break;
    case QMetaType::Double: {
        assertType(QV4::CompiledData::Binding::Type_Number);
        double value = compilationUnit->bindingValueAsNumber(binding);
        property->writeProperty(_qobject, &value, propertyWriteFlags);
    }
    break;
    case QMetaType::QColor: {
        QVariant data;
        if (QQml_valueTypeProvider()->createValueType(
                    QMetaType::QColor, compilationUnit->bindingValueAsString(binding), data)) {
            property->writeProperty(_qobject, data.data(), propertyWriteFlags);
        }
    }
    break;
#if QT_CONFIG(datestring)
    case QMetaType::QDate: {
        bool ok = false;
        QDate value = QQmlStringConverters::dateFromString(compilationUnit->bindingValueAsString(binding), &ok);
        assertOrNull(ok);
        property->writeProperty(_qobject, &value, propertyWriteFlags);
    }
    break;
    case QMetaType::QTime: {
        bool ok = false;
        QTime value = QQmlStringConverters::timeFromString(compilationUnit->bindingValueAsString(binding), &ok);
        assertOrNull(ok);
        property->writeProperty(_qobject, &value, propertyWriteFlags);
    }
    break;
    case QMetaType::QDateTime: {
        bool ok = false;
        QDateTime value = QQmlStringConverters::dateTimeFromString(
                    compilationUnit->bindingValueAsString(binding), &ok);
        assertOrNull(ok);
        property->writeProperty(_qobject, &value, propertyWriteFlags);
    }
    break;
#endif // datestring
    case QMetaType::QPoint: {
        bool ok = false;
        QPoint value = QQmlStringConverters::pointFFromString(compilationUnit->bindingValueAsString(binding), &ok).toPoint();
        assertOrNull(ok);
        property->writeProperty(_qobject, &value, propertyWriteFlags);
    }
    break;
    case QMetaType::QPointF: {
        bool ok = false;
        QPointF value = QQmlStringConverters::pointFFromString(compilationUnit->bindingValueAsString(binding), &ok);
        assertOrNull(ok);
        property->writeProperty(_qobject, &value, propertyWriteFlags);
    }
    break;
    case QMetaType::QSize: {
        bool ok = false;
        QSize value = QQmlStringConverters::sizeFFromString(compilationUnit->bindingValueAsString(binding), &ok).toSize();
        assertOrNull(ok);
        property->writeProperty(_qobject, &value, propertyWriteFlags);
    }
    break;
    case QMetaType::QSizeF: {
        bool ok = false;
        QSizeF value = QQmlStringConverters::sizeFFromString(compilationUnit->bindingValueAsString(binding), &ok);
        assertOrNull(ok);
        property->writeProperty(_qobject, &value, propertyWriteFlags);
    }
    break;
    case QMetaType::QRect: {
        bool ok = false;
        QRect value = QQmlStringConverters::rectFFromString(compilationUnit->bindingValueAsString(binding), &ok).toRect();
        assertOrNull(ok);
        property->writeProperty(_qobject, &value, propertyWriteFlags);
    }
    break;
    case QMetaType::QRectF: {
        bool ok = false;
        QRectF value = QQmlStringConverters::rectFFromString(compilationUnit->bindingValueAsString(binding), &ok);
        assertOrNull(ok);
        property->writeProperty(_qobject, &value, propertyWriteFlags);
    }
    break;
    case QMetaType::Bool: {
        assertType(QV4::CompiledData::Binding::Type_Boolean);
        bool value = binding->valueAsBoolean();
        property->writeProperty(_qobject, &value, propertyWriteFlags);
    }
    break;
    case QMetaType::QVector2D:
    case QMetaType::QVector3D:
    case QMetaType::QVector4D:
    case QMetaType::QQuaternion: {
        QVariant result;
        bool ok = QQml_valueTypeProvider()->createValueType(
                    propertyType, compilationUnit->bindingValueAsString(binding), result);
        assertOrNull(ok);
        Q_UNUSED(ok);
        property->writeProperty(_qobject, result.data(), propertyWriteFlags);
        break;
    }
    default: {
        // generate single literal value assignment to a list property if required
        if (propertyType == qMetaTypeId<QList<qreal> >()) {
            assertType(QV4::CompiledData::Binding::Type_Number);
            QList<qreal> value;
            value.append(compilationUnit->bindingValueAsNumber(binding));
            property->writeProperty(_qobject, &value, propertyWriteFlags);
            break;
        } else if (propertyType == qMetaTypeId<QList<int> >()) {
            assertType(QV4::CompiledData::Binding::Type_Number);
            double n = compilationUnit->bindingValueAsNumber(binding);
            QList<int> value;
            value.append(int(n));
            property->writeProperty(_qobject, &value, propertyWriteFlags);
            break;
        } else if (propertyType == qMetaTypeId<QList<bool> >()) {
            assertType(QV4::CompiledData::Binding::Type_Boolean);
            QList<bool> value;
            value.append(binding->valueAsBoolean());
            property->writeProperty(_qobject, &value, propertyWriteFlags);
            break;
        } else if (propertyType == qMetaTypeId<QList<QUrl> >()) {
            assertType(QV4::CompiledData::Binding::Type_String);
            const QUrl url(compilationUnit->bindingValueAsString(binding));
            QList<QUrl> value {
                QQmlPropertyPrivate::resolveUrlsOnAssignment()
                        ? compilationUnit->finalUrl().resolved(url)
                        : url
            };
            property->writeProperty(_qobject, &value, propertyWriteFlags);
            break;
        } else if (propertyType == qMetaTypeId<QList<QString> >()) {
            assertOrNull(binding->evaluatesToString());
            QList<QString> value;
            value.append(compilationUnit->bindingValueAsString(binding));
            property->writeProperty(_qobject, &value, propertyWriteFlags);
            break;
        } else if (propertyType == qMetaTypeId<QJSValue>()) {
            QJSValue value;
            if (binding->type == QV4::CompiledData::Binding::Type_Boolean) {
                value = QJSValue(binding->valueAsBoolean());
            } else if (binding->type == QV4::CompiledData::Binding::Type_Number) {
                double n = compilationUnit->bindingValueAsNumber(binding);
                if (double(int(n)) == n) {
                    value = QJSValue(int(n));
                } else
                    value = QJSValue(n);
            } else if (binding->type == QV4::CompiledData::Binding::Type_Null) {
                value = QJSValue::NullValue;
            } else {
                value = QJSValue(compilationUnit->bindingValueAsString(binding));
            }
            property->writeProperty(_qobject, &value, propertyWriteFlags);
            break;
        }

        // string converters are not exposed, so ending up here indicates an error
        QString stringValue = compilationUnit->bindingValueAsString(binding);
        QMetaProperty metaProperty = _qobject->metaObject()->property(property->coreIndex());
        recordError(binding->location, tr("Cannot assign value %1 to property"
" %2").arg(stringValue, QString::fromUtf8(metaProperty.name())));
    }
    break;
    }
}

static QQmlType qmlTypeForObject(QObject *object)
{
    QQmlType type;
    const QMetaObject *mo = object->metaObject();
    while (mo && !type.isValid()) {
        type = QQmlMetaType::qmlType(mo);
        mo = mo->superClass();
    }
    return type;
}

void QQmlObjectCreator::setupBindings(bool applyDeferredBindings)
{
    QQmlListProperty<void> savedList;
    qSwap(_currentList, savedList);

    const QV4::BindingPropertyData &propertyData = compilationUnit->bindingPropertyDataPerObject.at(_compiledObjectIndex);

    if (_compiledObject->idNameIndex) {
        const QQmlPropertyData *idProperty = propertyData.last();
        Q_ASSERT(!idProperty || !idProperty->isValid() || idProperty->name(_qobject) == QLatin1String("id"));
        if (idProperty && idProperty->isValid() && idProperty->isWritable() && idProperty->propType().id() == QMetaType::QString) {
            QV4::CompiledData::Binding idBinding;
            idBinding.propertyNameIndex = 0; // Not used
            idBinding.flags = 0;
            idBinding.type = QV4::CompiledData::Binding::Type_String;
            idBinding.stringIndex = _compiledObject->idNameIndex;
            idBinding.location = _compiledObject->location; // ###
            setPropertyValue(idProperty, &idBinding);
        }
    }

    // ### this is best done through type-compile-time binding skip lists.
    if (_valueTypeProperty) {
        QQmlAbstractBinding *binding = QQmlPropertyPrivate::binding(_bindingTarget, QQmlPropertyIndex(_valueTypeProperty->coreIndex()));

        if (binding && !binding->isValueTypeProxy()) {
            QQmlPropertyPrivate::removeBinding(_bindingTarget, QQmlPropertyIndex(_valueTypeProperty->coreIndex()));
        } else if (binding) {
            QQmlValueTypeProxyBinding *proxy = static_cast<QQmlValueTypeProxyBinding *>(binding);

            if (qmlTypeForObject(_bindingTarget).isValid()) {
                quint32 bindingSkipList = 0;

                QQmlPropertyData *defaultProperty = _compiledObject->indexOfDefaultPropertyOrAlias != -1 ? _propertyCache->parent()->defaultProperty() : _propertyCache->defaultProperty();

                const QV4::CompiledData::Binding *binding = _compiledObject->bindingTable();
                for (quint32 i = 0; i < _compiledObject->nBindings; ++i, ++binding) {
                    QQmlPropertyData *property = binding->propertyNameIndex != 0
                            ? _propertyCache->property(stringAt(binding->propertyNameIndex),
                                                       _qobject, context)
                            : defaultProperty;
                    if (property)
                        bindingSkipList |= (1 << property->coreIndex());
                }

                proxy->removeBindings(bindingSkipList);
            }
        }
    }

    int currentListPropertyIndex = -1;

    const QV4::CompiledData::Binding *binding = _compiledObject->bindingTable();
    for (quint32 i = 0; i < _compiledObject->nBindings; ++i, ++binding) {
        QQmlPropertyData *const property = propertyData.at(i);
        if (property) {
            QQmlPropertyData* targetProperty = property;
            if (targetProperty->isAlias()) {
                // follow alias
                QQmlPropertyIndex originalIndex(targetProperty->coreIndex(), _valueTypeProperty ? _valueTypeProperty->coreIndex() : -1);
                auto [targetObject, targetIndex] = QQmlPropertyPrivate::findAliasTarget(_bindingTarget, originalIndex);
                QQmlData *data = QQmlData::get(targetObject);
                Q_ASSERT(data && data->propertyCache);
                targetProperty = data->propertyCache->property(targetIndex.coreIndex());
            }
            sharedState->requiredProperties.remove(targetProperty);
        }


        if (binding->flags & QV4::CompiledData::Binding::IsCustomParserBinding)
            continue;

        if (binding->flags & QV4::CompiledData::Binding::IsDeferredBinding) {
            if (!applyDeferredBindings)
                continue;
        } else {
            if (applyDeferredBindings)
                continue;
        }

        if (property && property->isQList()) {
            if (property->coreIndex() != currentListPropertyIndex) {
                void *argv[1] = { (void*)&_currentList };
                QMetaObject::metacall(_qobject, QMetaObject::ReadProperty, property->coreIndex(), argv);
                currentListPropertyIndex = property->coreIndex();

                // manage override behavior
                const QMetaObject *const metaobject = _qobject->metaObject();
                const int qmlListBehavorClassInfoIndex = metaobject->indexOfClassInfo("QML.ListPropertyAssignBehavior");
                if (qmlListBehavorClassInfoIndex != -1) { // QML.ListPropertyAssignBehavior class info is set
                    const char *overrideBehavior =
                            metaobject->classInfo(qmlListBehavorClassInfoIndex).value();
                    if (!strcmp(overrideBehavior,
                                "Replace")) {
                        if (_currentList.clear) {
                            _currentList.clear(&_currentList);
                        }
                    } else {
                        bool isDefaultProperty =
                                (property->name(_qobject)
                                 == QString::fromUtf8(
                                         metaobject
                                                 ->classInfo(metaobject->indexOfClassInfo(
                                                         "DefaultProperty"))
                                                 .value()));
                        if (!isDefaultProperty
                            && (!strcmp(overrideBehavior,
                                        "ReplaceIfNotDefault"))) {
                            if (_currentList.clear) {
                                _currentList.clear(&_currentList);
                            }
                        }
                    }
                }
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

bool QQmlObjectCreator::setPropertyBinding(const QQmlPropertyData *bindingProperty, const QV4::CompiledData::Binding *binding)
{
    if (binding->type == QV4::CompiledData::Binding::Type_AttachedProperty) {
        Q_ASSERT(stringAt(compilationUnit->objectAt(binding->value.objectIndex)->inheritedTypeNameIndex).isEmpty());
        QV4::ResolvedTypeReference *tr = resolvedType(binding->propertyNameIndex);
        Q_ASSERT(tr);
        QQmlType attachedType = tr->type();
        if (!attachedType.isValid()) {
            QQmlTypeNameCache::Result res = context->imports()->query(
                        stringAt(binding->propertyNameIndex));
            if (res.isValid())
                attachedType = res.type;
            else
                return false;
        }
        QObject *qmlObject = qmlAttachedPropertiesObject(
                _qobject, attachedType.attachedPropertiesFunction(QQmlEnginePrivate::get(engine)));
        if (!populateInstance(binding->value.objectIndex, qmlObject, qmlObject,
                              /*value type property*/ nullptr, binding))
            return false;
        return true;
    }

    // ### resolve this at compile time
    if (bindingProperty && bindingProperty->propType() == QMetaType::fromType<QQmlScriptString>()) {
        QQmlScriptString ss(compilationUnit->bindingValueAsScriptString(binding),
                            context->asQQmlContext(), _scopeObject);
        ss.d.data()->bindingId = binding->type == QV4::CompiledData::Binding::Type_Script ? binding->value.compiledScriptIndex : (quint32)QQmlBinding::Invalid;
        ss.d.data()->lineNumber = binding->location.line;
        ss.d.data()->columnNumber = binding->location.column;
        ss.d.data()->isStringLiteral = binding->type == QV4::CompiledData::Binding::Type_String;
        ss.d.data()->isNumberLiteral = binding->type == QV4::CompiledData::Binding::Type_Number;
        ss.d.data()->numberValue = compilationUnit->bindingValueAsNumber(binding);

        QQmlPropertyData::WriteFlags propertyWriteFlags = QQmlPropertyData::BypassInterceptor |
                                                            QQmlPropertyData::RemoveBindingOnAliasWrite;
        int propertyWriteStatus = -1;
        void *argv[] = { &ss, nullptr, &propertyWriteStatus, &propertyWriteFlags };
        QMetaObject::metacall(_qobject, QMetaObject::WriteProperty, bindingProperty->coreIndex(), argv);
        return true;
    }

    QObject *createdSubObject = nullptr;
    if (binding->type == QV4::CompiledData::Binding::Type_Object) {
        createdSubObject = createInstance(binding->value.objectIndex, _bindingTarget);
        if (!createdSubObject)
            return false;
    }

    if (!bindingProperty) // ### error
        return true;

    if (binding->type == QV4::CompiledData::Binding::Type_GroupProperty) {
        const QV4::CompiledData::Object *obj = compilationUnit->objectAt(binding->value.objectIndex);
        if (stringAt(obj->inheritedTypeNameIndex).isEmpty()) {

            QObject *groupObject = nullptr;
            QQmlGadgetPtrWrapper *valueType = nullptr;
            const QQmlPropertyData *valueTypeProperty = nullptr;
            QObject *bindingTarget = _bindingTarget;

            if (QQmlMetaType::isValueType(bindingProperty->propType())) {
                valueType = QQmlGadgetPtrWrapper::instance(engine, bindingProperty->propType());
                if (!valueType) {
                    recordError(binding->location, tr("Cannot set properties on %1 as it is null").arg(stringAt(binding->propertyNameIndex)));
                    return false;
                }

                valueType->read(_qobject, bindingProperty->coreIndex());

                groupObject = valueType;
                valueTypeProperty = bindingProperty;
            } else {
                void *argv[1] = { &groupObject };
                QMetaObject::metacall(_qobject, QMetaObject::ReadProperty, bindingProperty->coreIndex(), argv);
                if (!groupObject) {
                    recordError(binding->location, tr("Cannot set properties on %1 as it is null").arg(stringAt(binding->propertyNameIndex)));
                    return false;
                }

                bindingTarget = groupObject;
            }

            if (!populateInstance(binding->value.objectIndex, groupObject, bindingTarget,
                                  valueTypeProperty, binding))
                return false;

            if (valueType)
                valueType->write(_qobject, bindingProperty->coreIndex(), QQmlPropertyData::BypassInterceptor);

            return true;
        }
    }

    const bool allowedToRemoveBinding = !(binding->flags & QV4::CompiledData::Binding::IsSignalHandlerExpression)
            && !(binding->flags & QV4::CompiledData::Binding::IsOnAssignment)
            && !(binding->flags & QV4::CompiledData::Binding::IsPropertyObserver)
            && !_valueTypeProperty;

    if (_ddata->hasBindingBit(bindingProperty->coreIndex()) && allowedToRemoveBinding) {
        QQmlPropertyPrivate::removeBinding(_bindingTarget, QQmlPropertyIndex(bindingProperty->coreIndex()));
    } else if (bindingProperty->isBindable() && allowedToRemoveBinding) {
        QList<DeferredQPropertyBinding> &pendingBindings = sharedState.data()->allQPropertyBindings;
        auto it = std::remove_if(pendingBindings.begin(), pendingBindings.end(), [&](const DeferredQPropertyBinding &deferred) {
            return deferred.properyIndex == bindingProperty->coreIndex() && deferred.target == _bindingTarget;
        });
        pendingBindings.erase(it, pendingBindings.end());
    }

    if (binding->type == QV4::CompiledData::Binding::Type_Script || binding->isTranslationBinding()) {
        if (binding->flags & QV4::CompiledData::Binding::IsSignalHandlerExpression
            || binding->flags & QV4::CompiledData::Binding::IsPropertyObserver) {
            QV4::Function *runtimeFunction = compilationUnit->runtimeFunctions[binding->value.compiledScriptIndex];
            int signalIndex = _propertyCache->methodIndexToSignalIndex(bindingProperty->coreIndex());
            QQmlBoundSignalExpression *expr = new QQmlBoundSignalExpression(
                        _bindingTarget, signalIndex, context,
                        _scopeObject, runtimeFunction, currentQmlContext());

            if (bindingProperty->isBindable()) {
                auto target = _bindingTarget;
                if (bindingProperty->isAlias()) {
                    // If the property is an alias, we cannot obtain the bindable interface directly with qt_metacall
                    // so instead, we resolve the alias to obtain the actual target
                    // This should be faster than doing a detour through the metaobject of the target, and relying on
                    // QMetaObject::metacall doing the correct resolution
                    QQmlPropertyIndex originalIndex(bindingProperty->coreIndex(), _valueTypeProperty ? _valueTypeProperty->coreIndex() : -1);
                    auto [aliasTargetObject, aliasTargetIndex] = QQmlPropertyPrivate::findAliasTarget(target, originalIndex);
                    target = aliasTargetObject;
                    QQmlData *data = QQmlData::get(target);
                    Q_ASSERT(data && data->propertyCache);
                    bindingProperty = data->propertyCache->property(aliasTargetIndex.coreIndex());
                }
                auto &observer = QQmlData::get(_scopeObject)->propertyObservers.emplace_back(expr);
                QUntypedBindable bindable;
                void *argv[] = { &bindable };
                target->qt_metacall(QMetaObject::BindableProperty, bindingProperty->coreIndex(), argv);
                Q_ASSERT(bindable.isValid());
                bindable.observe(&observer);
            } else {
                QQmlBoundSignal *bs = new QQmlBoundSignal(_bindingTarget, signalIndex, _scopeObject, engine);
                bs->takeExpression(expr);
            }
        } else if (bindingProperty->isBindable()) {
            QUntypedPropertyBinding qmlBinding;
            if (binding->isTranslationBinding()) {
                qmlBinding = QQmlTranslationPropertyBinding::create(bindingProperty, compilationUnit, binding);
            } else {
                QV4::Function *runtimeFunction = compilationUnit->runtimeFunctions[binding->value.compiledScriptIndex];
                QQmlPropertyIndex index(bindingProperty->coreIndex(), -1);
                qmlBinding = QQmlPropertyBinding::create(bindingProperty, runtimeFunction, _scopeObject, context, currentQmlContext(), _bindingTarget, index);
            }
            sharedState.data()->allQPropertyBindings.push_back(DeferredQPropertyBinding {_bindingTarget, bindingProperty->coreIndex(), qmlBinding });
        } else {
            // When writing bindings to grouped properties implemented as value types,
            // such as point.x: { someExpression; }, then the binding is installed on
            // the point property (_qobjectForBindings) and after evaluating the expression,
            // the result is written to a value type virtual property, that contains the sub-index
            // of the "x" property.
            QQmlBinding::Ptr qmlBinding;
            const QQmlPropertyData *targetProperty = bindingProperty;
            const QQmlPropertyData *subprop = nullptr;
            if (_valueTypeProperty) {
                targetProperty = _valueTypeProperty;
                subprop = bindingProperty;
            }
            if (binding->isTranslationBinding()) {
                qmlBinding = QQmlBinding::createTranslationBinding(
                            compilationUnit, binding, _scopeObject, context);
            } else {
                QV4::Function *runtimeFunction = compilationUnit->runtimeFunctions[binding->value.compiledScriptIndex];
                qmlBinding = QQmlBinding::create(targetProperty, runtimeFunction, _scopeObject,
                                                 context, currentQmlContext());
            }

            auto bindingTarget = _bindingTarget;
            auto valueTypeProperty = _valueTypeProperty;
            auto assignBinding = [qmlBinding, bindingTarget, targetProperty, subprop, bindingProperty, valueTypeProperty](QQmlObjectCreatorSharedState *sharedState) mutable -> bool {
                if (!qmlBinding->setTarget(bindingTarget, *targetProperty, subprop) && targetProperty->isAlias())
                    return false;

                sharedState->allCreatedBindings.push(qmlBinding);

                if (bindingProperty->isAlias()) {
                    QQmlPropertyPrivate::setBinding(qmlBinding.data(), QQmlPropertyPrivate::DontEnable);
                } else {
                    qmlBinding->addToObject();

                    if (!valueTypeProperty) {
                        QQmlData *targetDeclarativeData = QQmlData::get(bindingTarget);
                        Q_ASSERT(targetDeclarativeData);
                        targetDeclarativeData->setPendingBindingBit(bindingTarget, bindingProperty->coreIndex());
                    }
                }

                return true;
            };
            if (!assignBinding(sharedState.data()))
                pendingAliasBindings.push_back(assignBinding);
        }
        return true;
    }

    if (binding->type == QV4::CompiledData::Binding::Type_Object) {
        if (binding->flags & QV4::CompiledData::Binding::IsOnAssignment) {
            // ### determine value source and interceptor casts ahead of time.
            QQmlType type = qmlTypeForObject(createdSubObject);
            Q_ASSERT(type.isValid());

            int valueSourceCast = type.propertyValueSourceCast();
            if (valueSourceCast != -1) {
                QQmlPropertyValueSource *vs = reinterpret_cast<QQmlPropertyValueSource *>(reinterpret_cast<char *>(createdSubObject) + valueSourceCast);
                QObject *target = createdSubObject->parent();
                QQmlProperty prop;
                if (_valueTypeProperty) {
                    prop = QQmlPropertyPrivate::restore(target, *_valueTypeProperty,
                                                        bindingProperty, context);
                } else {
                    prop = QQmlPropertyPrivate::restore(target, *bindingProperty, nullptr, context);
                }
                vs->setTarget(prop);
                return true;
            }
            int valueInterceptorCast = type.propertyValueInterceptorCast();
            if (valueInterceptorCast != -1) {
                QQmlPropertyValueInterceptor *vi = reinterpret_cast<QQmlPropertyValueInterceptor *>(reinterpret_cast<char *>(createdSubObject) + valueInterceptorCast);
                QObject *target = createdSubObject->parent();

                QQmlPropertyIndex propertyIndex;
                if (bindingProperty->isAlias()) {
                    QQmlPropertyIndex originalIndex(bindingProperty->coreIndex(), _valueTypeProperty ? _valueTypeProperty->coreIndex() : -1);
                    auto aliasTarget = QQmlPropertyPrivate::findAliasTarget(target, originalIndex);
                    target = aliasTarget.targetObject;
                    QQmlData *data = QQmlData::get(target);
                    if (!data || !data->propertyCache) {
                        qWarning() << "can't resolve property alias for 'on' assignment";
                        return false;
                    }

                    // we can't have aliasses on subproperties of value types, so:
                    QQmlPropertyData targetPropertyData = *data->propertyCache->property(aliasTarget.targetIndex.coreIndex());
                    auto prop = QQmlPropertyPrivate::restore(
                                target, targetPropertyData, nullptr, context);
                    vi->setTarget(prop);
                    propertyIndex = QQmlPropertyPrivate::propertyIndex(prop);
                } else {
                    QQmlProperty prop;
                    if (_valueTypeProperty) {
                        prop = QQmlPropertyPrivate::restore(
                                    target, *_valueTypeProperty, bindingProperty, context);
                    } else {
                        prop = QQmlPropertyPrivate::restore(
                                    target, *bindingProperty, nullptr, context);
                    }
                    vi->setTarget(prop);
                    propertyIndex = QQmlPropertyPrivate::propertyIndex(prop);
                }

                QQmlInterceptorMetaObject *mo = QQmlInterceptorMetaObject::get(target);
                if (!mo)
                    mo = new QQmlInterceptorMetaObject(target, QQmlData::get(target)->propertyCache);
                mo->registerInterceptor(propertyIndex, vi);
                return true;
            }
            return false;
        }

        // Assigning object to signal property? ### Qt 7: Remove that functionality
        if (binding->flags & QV4::CompiledData::Binding::IsSignalHandlerObject) {
            if (!bindingProperty->isFunction()) {
                recordError(binding->valueLocation, tr("Cannot assign an object to signal property %1").arg(bindingProperty->name(_qobject)));
                return false;
            }
            QMetaMethod method = QQmlMetaType::defaultMethod(createdSubObject);
            if (!method.isValid()) {
                recordError(binding->valueLocation, tr("Cannot assign object type %1 with no default method").arg(QString::fromLatin1(createdSubObject->metaObject()->className())));
                return false;
            }
            qCWarning(lcQmlDefaultMethod) << "Assigning an object to a signal handler is deprecated."
                                             "Instead, create the object, give it an id, and call the desired slot from the signal handler."
                                             ;

            QMetaMethod signalMethod = _qobject->metaObject()->method(bindingProperty->coreIndex());
            if (!QMetaObject::checkConnectArgs(signalMethod, method)) {
                recordError(binding->valueLocation,
                            tr("Cannot connect mismatched signal/slot %1 vs %2")
                            .arg(QString::fromUtf8(method.methodSignature()))
                            .arg(QString::fromUtf8(signalMethod.methodSignature())));
                return false;
            }

            QQmlPropertyPrivate::connect(_qobject, bindingProperty->coreIndex(), createdSubObject, method.methodIndex());
            return true;
        }

        QQmlPropertyData::WriteFlags propertyWriteFlags = QQmlPropertyData::BypassInterceptor |
                                                            QQmlPropertyData::RemoveBindingOnAliasWrite;
        int propertyWriteStatus = -1;
        void *argv[] = { nullptr, nullptr, &propertyWriteStatus, &propertyWriteFlags };

        if (const char *iid = QQmlMetaType::interfaceIId(bindingProperty->propType().id())) {
            void *ptr = createdSubObject->qt_metacast(iid);
            if (ptr) {
                argv[0] = &ptr;
                QMetaObject::metacall(_qobject, QMetaObject::WriteProperty, bindingProperty->coreIndex(), argv);
            } else {
                recordError(binding->location, tr("Cannot assign object to interface property"));
                return false;
            }
        } else if (bindingProperty->propType() == QMetaType::fromType<QVariant>()) {
            if (bindingProperty->isVarProperty()) {
                QV4::Scope scope(v4);
                QV4::ScopedValue wrappedObject(scope, QV4::QObjectWrapper::wrap(engine->handle(), createdSubObject));
                _vmeMetaObject->setVMEProperty(bindingProperty->coreIndex(), wrappedObject);
            } else {
                QVariant value = QVariant::fromValue(createdSubObject);
                argv[0] = &value;
                QMetaObject::metacall(_qobject, QMetaObject::WriteProperty, bindingProperty->coreIndex(), argv);
            }
        } else if (bindingProperty->propType() == QMetaType::fromType<QJSValue>()) {
            QV4::Scope scope(v4);
            QV4::ScopedValue wrappedObject(scope, QV4::QObjectWrapper::wrap(engine->handle(), createdSubObject));
            if (bindingProperty->isVarProperty()) {
                _vmeMetaObject->setVMEProperty(bindingProperty->coreIndex(), wrappedObject);
            } else {
                QJSValue value;
                QJSValuePrivate::setValue(&value, wrappedObject);
                argv[0] = &value;
                QMetaObject::metacall(_qobject, QMetaObject::WriteProperty, bindingProperty->coreIndex(), argv);
            }
        } else if (bindingProperty->isQList()) {
            Q_ASSERT(_currentList.object);

            void *itemToAdd = createdSubObject;

            QMetaType listItemType = QQmlMetaType::listType(bindingProperty->propType());
            if (listItemType.isValid()) {
                const char *iid = QQmlMetaType::interfaceIId(listItemType.id());
                if (iid)
                    itemToAdd = createdSubObject->qt_metacast(iid);
            }

            if (_currentList.append)
                _currentList.append(&_currentList, itemToAdd);
            else {
                recordError(binding->location, tr("Cannot assign object to read only list"));
                return false;
            }

        } else {
            // pointer compatibility was tested in QQmlPropertyValidator at type compile time
            argv[0] = &createdSubObject;
            QMetaObject::metacall(_qobject, QMetaObject::WriteProperty, bindingProperty->coreIndex(), argv);
        }
        return true;
    }

    if (bindingProperty->isQList()) {
        recordError(binding->location, tr("Cannot assign primitives to lists"));
        return false;
    }

    setPropertyValue(bindingProperty, binding);
    return true;
}

void QQmlObjectCreator::setupFunctions()
{
    QV4::Scope scope(v4);
    QV4::ScopedValue function(scope);
    QV4::ScopedContext qmlContext(scope, currentQmlContext());

    const quint32_le *functionIdx = _compiledObject->functionOffsetTable();
    for (quint32 i = 0; i < _compiledObject->nFunctions; ++i, ++functionIdx) {
        QV4::Function *runtimeFunction = compilationUnit->runtimeFunctions[*functionIdx];
        const QString name = runtimeFunction->name()->toQString();

        QQmlPropertyData *property = _propertyCache->property(name, _qobject, context);
        if (!property->isVMEFunction())
            continue;

        if (runtimeFunction->isGenerator())
            function = QV4::GeneratorFunction::create(qmlContext, runtimeFunction);
        else
            function = QV4::FunctionObject::createScriptFunction(qmlContext, runtimeFunction);
        _vmeMetaObject->setVmeMethod(property->coreIndex(), function);
    }
}

void QQmlObjectCreator::recordError(const QV4::CompiledData::Location &location, const QString &description)
{
    QQmlError error;
    error.setUrl(compilationUnit->url());
    error.setLine(qmlConvertSourceCoordinate<quint32, int>(location.line));
    error.setColumn(qmlConvertSourceCoordinate<quint32, int>(location.column));
    error.setDescription(description);
    errors << error;
}

void QQmlObjectCreator::registerObjectWithContextById(const QV4::CompiledData::Object *object, QObject *instance) const
{
    if (object->id >= 0)
        context->setIdValue(object->id, instance);
}

QObject *QQmlObjectCreator::createInstance(int index, QObject *parent, bool isContextObject)
{
    const QV4::CompiledData::Object *obj = compilationUnit->objectAt(index);
    QQmlObjectCreationProfiler profiler(sharedState->profiler.profiler, obj);
    Q_TRACE(QQmlObjectCreator_createInstance_entry, compilationUnit.data(), obj, context->url());
    QString typeName;
    Q_TRACE_EXIT(QQmlObjectCreator_createInstance_exit, typeName);

    QScopedValueRollback<QQmlObjectCreator*> ocRestore(QQmlEnginePrivate::get(engine)->activeObjectCreator, this);

    bool isComponent = false;
    QObject *instance = nullptr;
    QQmlData *ddata = nullptr;
    QQmlCustomParser *customParser = nullptr;
    QQmlParserStatus *parserStatus = nullptr;
    bool installPropertyCache = true;

    if (obj->flags & QV4::CompiledData::Object::IsComponent) {
        isComponent = true;
        QQmlComponent *component = new QQmlComponent(engine, compilationUnit.data(), index, parent);
        typeName = QStringLiteral("<component>");
        QQmlComponentPrivate::get(component)->creationContext = context;
        instance = component;
        ddata = QQmlData::get(instance, /*create*/true);
    } else {
        QV4::ResolvedTypeReference *typeRef = resolvedType(obj->inheritedTypeNameIndex);
        Q_ASSERT(typeRef);
        installPropertyCache = !typeRef->isFullyDynamicType();
        const QQmlType type = typeRef->type();
        if (type.isValid() && !type.isInlineComponentType()) {
            typeName = type.qmlTypeName();

            void *ddataMemory = nullptr;
            type.create(&instance, &ddataMemory, sizeof(QQmlData));
            if (!instance) {
                recordError(obj->location, tr("Unable to create object of type %1").arg(stringAt(obj->inheritedTypeNameIndex)));
                return nullptr;
            }

            {
                QQmlData *ddata = new (ddataMemory) QQmlData;
                ddata->ownMemory = false;
                QObjectPrivate* p = QObjectPrivate::get(instance);
                Q_ASSERT(!p->isDeletingChildren);
                p->declarativeData = ddata;
            }

            const int parserStatusCast = type.parserStatusCast();
            if (parserStatusCast != -1)
                parserStatus = reinterpret_cast<QQmlParserStatus*>(reinterpret_cast<char *>(instance) + parserStatusCast);

            customParser = type.customParser();

            if (sharedState->rootContext && sharedState->rootContext->isRootObjectInCreation()) {
                QQmlData *ddata = QQmlData::get(instance, /*create*/true);
                ddata->rootObjectInCreation = true;
                sharedState->rootContext->setRootObjectInCreation(false);
            }

            sharedState->allCreatedObjects.push(instance);
        } else {
            const auto compilationUnit = typeRef->compilationUnit();
            Q_ASSERT(compilationUnit);
            typeName = compilationUnit->fileName();
            // compilation unit is shared between root type and its inline component types
            // so isSingleton errorneously returns true for inline components
            if (compilationUnit->unitData()->isSingleton() && !type.isInlineComponentType()) {
                recordError(obj->location, tr("Composite Singleton Type %1 is not creatable").arg(stringAt(obj->inheritedTypeNameIndex)));
                return nullptr;
            }

            if (!type.isInlineComponentType()) {
                QQmlObjectCreator subCreator(context, compilationUnit, sharedState.data());
                instance = subCreator.create();
                if (!instance) {
                    errors += subCreator.errors;
                    return nullptr;
                }
            } else {
                int subObjectId = type.inlineComponentId();
                QScopedValueRollback<int> rollback {compilationUnit->icRoot, subObjectId};
                QQmlObjectCreator subCreator(context, compilationUnit, sharedState.data());
                instance = subCreator.create(subObjectId, nullptr, nullptr, CreationFlags::InlineComponent);
                if (!instance) {
                    errors += subCreator.errors;
                    return nullptr;
                }
            }
        }
        if (instance->isWidgetType()) {
            if (parent && parent->isWidgetType()) {
                QAbstractDeclarativeData::setWidgetParent(instance, parent);
            } else {
                // No parent! Layouts need to handle this through a default property that
                // reparents accordingly. Otherwise the garbage collector will collect.
            }
        } else if (parent) {
            QQml_setParent_noEvent(instance, parent);
        }

        ddata = QQmlData::get(instance, /*create*/true);
    }

    Q_QML_OC_PROFILE(sharedState->profiler, profiler.update(
        compilationUnit.data(), obj, typeName, context->url()));
    Q_UNUSED(typeName); // only relevant for tracing

    ddata->lineNumber = obj->location.line;
    ddata->columnNumber = obj->location.column;

    ddata->setImplicitDestructible();
    // inline components are root objects, but their index is != 0, so we need
    // an additional check
    const bool isInlineComponent = obj->flags & QV4::CompiledData::Object::IsInlineComponentRoot;
    if (static_cast<quint32>(index) == /*root object*/0 || ddata->rootObjectInCreation || isInlineComponent) {
        if (ddata->context) {
            Q_ASSERT(ddata->context != context.data());
            Q_ASSERT(ddata->outerContext);
            Q_ASSERT(ddata->outerContext != context.data());
            QQmlRefPointer<QQmlContextData> c = ddata->context;
            while (QQmlRefPointer<QQmlContextData> linked = c->linkedContext())
                c = linked;
            c->setLinkedContext(context);
        } else {
            ddata->context = context.data();
        }
        ddata->ownContext = ddata->context;
    } else if (!ddata->context) {
        ddata->context = context.data();
    }

    context->addOwnedObject(ddata);

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
        context->setContextObject(instance);

    if (customParser && obj->flags & QV4::CompiledData::Object::HasCustomParserBindings) {
        customParser->engine = QQmlEnginePrivate::get(engine);
        customParser->imports = compilationUnit->typeNameCache.data();

        QList<const QV4::CompiledData::Binding *> bindings;
        const QV4::CompiledData::Object *obj = compilationUnit->objectAt(index);
        const QV4::CompiledData::Binding *binding = obj->bindingTable();
        for (quint32 i = 0; i < obj->nBindings; ++i, ++binding) {
            if (binding->flags & QV4::CompiledData::Binding::IsCustomParserBinding) {
                bindings << binding;
            }
        }
        customParser->applyBindings(instance, compilationUnit.data(), bindings);

        customParser->engine = nullptr;
        customParser->imports = (QQmlTypeNameCache*)nullptr;
    }

    if (isComponent) {
        registerObjectWithContextById(obj, instance);
        return instance;
    }

    QQmlRefPointer<QQmlPropertyCache> cache = propertyCaches->at(index);
    Q_ASSERT(!cache.isNull());
    if (installPropertyCache) {
        if (ddata->propertyCache)
            ddata->propertyCache->release();;
        ddata->propertyCache = cache.data();
        ddata->propertyCache->addref();
    }

    QObject *scopeObject = instance;
    qSwap(_scopeObject, scopeObject);

    Q_ASSERT(sharedState->allJavaScriptObjects);
    *sharedState->allJavaScriptObjects = QV4::QObjectWrapper::wrap(v4, instance);
    ++sharedState->allJavaScriptObjects;

    QV4::Scope valueScope(v4);
    QV4::QmlContext *qmlContext = static_cast<QV4::QmlContext *>(valueScope.alloc());

    qSwap(_qmlContext, qmlContext);

    bool ok = populateInstance(index, instance, /*binding target*/instance, /*value type property*/nullptr);
    if (ok) {
        if (isContextObject && !pendingAliasBindings.empty()) {
            bool processedAtLeastOneBinding = false;
            do {
                processedAtLeastOneBinding = false;
                for (std::vector<PendingAliasBinding>::iterator it = pendingAliasBindings.begin();
                        it != pendingAliasBindings.end(); ) {
                    if ((*it)(sharedState.data())) {
                        it = pendingAliasBindings.erase(it);
                        processedAtLeastOneBinding = true;
                    } else {
                        ++it;
                    }
                }
            } while (processedAtLeastOneBinding && pendingAliasBindings.empty());
            Q_ASSERT(pendingAliasBindings.empty());
        }
    } else {
        // an error occurred, so we can't setup the pending alias bindings
        pendingAliasBindings.clear();
    }

    qSwap(_qmlContext, qmlContext);
    qSwap(_scopeObject, scopeObject);

    return ok ? instance : nullptr;
}

bool QQmlObjectCreator::finalize(QQmlInstantiationInterrupt &interrupt)
{
    Q_ASSERT(phase == ObjectsCreated || phase == Finalizing);
    phase = Finalizing;

    QQmlObjectCreatorRecursionWatcher watcher(this);
    QScopedValueRollback<QQmlObjectCreator*> ocRestore(QQmlEnginePrivate::get(engine)->activeObjectCreator, this);

    /* We install all pending bindings (both plain QML and QProperty), and remove the ones which do not
       actually have dependencies.
       It is necessary to install the binding so that it runs at least once, which causes it to capture any
       dependencies.
       We then check for the following conditions:
       - Is the binding in an error state?
       - Does the binding has any dependencies (from properties)?
       - Does it depend on anything in the context, which has not been resolved yet (and thus couldn't be
         captured)?
       If the answer to all of those questions is "no", it is safe to remove the binding, as there is no
       way for it to change its value afterwards from that point on.
    */

    while (!sharedState->allCreatedBindings.isEmpty()) {
        QQmlAbstractBinding::Ptr b = sharedState->allCreatedBindings.pop();
        Q_ASSERT(b);
        // skip, if b is not added to an object
        if (!b->isAddedToObject())
            continue;
        QQmlData *data = QQmlData::get(b->targetObject());
        Q_ASSERT(data);
        data->clearPendingBindingBit(b->targetPropertyIndex().coreIndex());
        b->setEnabled(true, QQmlPropertyData::BypassInterceptor |
                      QQmlPropertyData::DontRemoveBinding);
        if (!b->isValueTypeProxy()) {
            QQmlBinding *binding = static_cast<QQmlBinding*>(b.data());
            if (!binding->hasError() && !binding->hasDependencies()
                    && !binding->hasUnresolvedNames()) {
                b->removeFromObject();
            }
        }

        if (watcher.hasRecursed() || interrupt.shouldInterrupt())
            return false;
    }

    while (!sharedState->allQPropertyBindings.isEmpty()) {
        auto& [target, index, qmlBinding] = sharedState->allQPropertyBindings.first();
        QUntypedBindable bindable;
        void *argv[] = { &bindable };
        // allow interception
        target->metaObject()->metacall(target, QMetaObject::BindableProperty, index, argv);
        bindable.setBinding(qmlBinding);
        sharedState->allQPropertyBindings.pop_front();
        if (auto priv = QPropertyBindingPrivate::get(qmlBinding); priv->hasCustomVTable()) {
            auto qmlBindingPriv = static_cast<QQmlPropertyBinding *>(priv);
            auto jsExpression = qmlBindingPriv->jsExpression();
            const bool canRemove = !qmlBinding.error().hasError() && !qmlBindingPriv->hasDependencies()
                    && !jsExpression->hasUnresolvedNames();
            if (canRemove)
                bindable.takeBinding();
        }
        if (watcher.hasRecursed() || interrupt.shouldInterrupt())
            return false;
    }

    if (QQmlVME::componentCompleteEnabled()) { // the qml designer does the component complete later
        while (!sharedState->allParserStatusCallbacks.isEmpty()) {
            QQmlObjectCompletionProfiler profiler(&sharedState->profiler);
            QQmlParserStatus *status = sharedState->allParserStatusCallbacks.pop();

            if (status && status->d) {
                status->d = nullptr;
                status->componentComplete();
            }

            if (watcher.hasRecursed() || interrupt.shouldInterrupt())
                return false;
        }
    }

    for (int ii = 0; ii < sharedState->finalizeCallbacks.count(); ++ii) {
        QQmlEnginePrivate::FinalizeCallback callback = sharedState->finalizeCallbacks.at(ii);
        QObject *obj = callback.first;
        if (obj) {
            void *args[] = { nullptr };
            QMetaObject::metacall(obj, QMetaObject::InvokeMetaMethod, callback.second, args);
        }
        if (watcher.hasRecursed())
            return false;
    }
    sharedState->finalizeCallbacks.clear();

    while (sharedState->componentAttached) {
        QQmlComponentAttached *a = sharedState->componentAttached;
        a->removeFromList();
        QQmlData *d = QQmlData::get(a->parent());
        Q_ASSERT(d);
        Q_ASSERT(d->context);
        d->context->addComponentAttached(a);
        if (QQmlVME::componentCompleteEnabled())
            emit a->completed();

        if (watcher.hasRecursed() || interrupt.shouldInterrupt())
            return false;
    }

    phase = Done;

    return true;
}

void QQmlObjectCreator::clear()
{
    if (phase == Done || phase == Finalizing || phase == Startup)
        return;
    Q_ASSERT(phase != Startup);

    while (!sharedState->allCreatedObjects.isEmpty()) {
        auto object = sharedState->allCreatedObjects.pop();
        if (engine->objectOwnership(object) != QQmlEngine::CppOwnership) {
            delete object;
        }
    }

    while (sharedState->componentAttached) {
        QQmlComponentAttached *a = sharedState->componentAttached;
        a->removeFromList();
    }

    phase = Done;
}

bool QQmlObjectCreator::populateInstance(int index, QObject *instance, QObject *bindingTarget,
                                         const QQmlPropertyData *valueTypeProperty,
                                         const QV4::CompiledData::Binding *binding)
{
    QQmlData *declarativeData = QQmlData::get(instance, /*create*/true);

    qSwap(_qobject, instance);
    qSwap(_valueTypeProperty, valueTypeProperty);
    qSwap(_compiledObjectIndex, index);
    const QV4::CompiledData::Object *obj = compilationUnit->objectAt(_compiledObjectIndex);
    qSwap(_compiledObject, obj);
    qSwap(_ddata, declarativeData);
    qSwap(_bindingTarget, bindingTarget);

    QV4::Scope valueScope(v4);
    QV4::ScopedValue scopeObjectProtector(valueScope);

    QQmlRefPointer<QQmlPropertyCache> cache = propertyCaches->at(_compiledObjectIndex);

    QQmlVMEMetaObject *vmeMetaObject = nullptr;
    if (propertyCaches->needsVMEMetaObject(_compiledObjectIndex)) {
        Q_ASSERT(!cache.isNull());
        // install on _object
        vmeMetaObject = new QQmlVMEMetaObject(v4, _qobject, cache, compilationUnit, _compiledObjectIndex);
        if (_ddata->propertyCache)
            _ddata->propertyCache->release();
        _ddata->propertyCache = cache.data();
        _ddata->propertyCache->addref();
        scopeObjectProtector = _ddata->jsWrapper.value();
    } else {
        vmeMetaObject = QQmlVMEMetaObject::get(_qobject);
    }

    registerObjectWithContextById(_compiledObject, _qobject);

    qSwap(_propertyCache, cache);
    qSwap(_vmeMetaObject, vmeMetaObject);

    if (_compiledObject->flags & QV4::CompiledData::Object::HasDeferredBindings)
        _ddata->deferData(_compiledObjectIndex, compilationUnit, context);

    const qsizetype oldRequiredPropertiesCount = sharedState->requiredProperties.size();
    QSet<QString> postHocRequired;
    for (auto it = _compiledObject->requiredPropertyExtraDataBegin(); it != _compiledObject->requiredPropertyExtraDataEnd(); ++it)
        postHocRequired.insert(stringAt(it->nameIndex));
    bool hadInheritedRequiredProperties = !postHocRequired.empty();

    for (int propertyIndex = 0; propertyIndex != _compiledObject->propertyCount(); ++propertyIndex) {
        const QV4::CompiledData::Property* property = _compiledObject->propertiesBegin() + propertyIndex;
        QQmlPropertyData *propertyData = _propertyCache->property(_propertyCache->propertyOffset() + propertyIndex);
        // only compute stringAt if there's a chance for the lookup to succeed
        auto postHocIt = postHocRequired.isEmpty() ? postHocRequired.end() : postHocRequired.find(stringAt(property->nameIndex));
        if (!property->isRequired && postHocRequired.end() == postHocIt)
            continue;
        if (postHocIt != postHocRequired.end())
            postHocRequired.erase(postHocIt);
        sharedState->hadRequiredProperties = true;
        sharedState->requiredProperties.insert(propertyData,
                                               RequiredPropertyInfo {compilationUnit->stringAt(property->nameIndex), compilationUnit->finalUrl(), property->location, {}});

    }

    const auto getPropertyCacheRange = [&]() -> std::pair<int, int> {
        // the logic in a nutshell: we work with QML instances here. every
        // instance has a QQmlType:
        // * if QQmlType is valid && not an inline component, it's a C++ type
        // * otherwise, it's a QML-defined type (a.k.a. Composite type), where
        //   invalid type == "comes from another QML document"
        //
        // 1. if the type we inherit from comes from C++, we must check *all*
        //    properties in the property cache so far - since we can have
        //    required properties defined in C++
        // 2. otherwise - the type comes from QML, it's enough to check just
        //    *own* properties in the property cache, because there's a previous
        //    type in the hierarchy that has checked the C++ properties (via 1.)
        // 3. required attached properties are explicitly not supported. to
        //    achieve that, go through all its properties
        // 4. required group properties: the group itself is covered by 1.
        //    required sub-properties are not properly handled (QTBUG-96544), so
        //    just return the old range here for consistency
        QV4::ResolvedTypeReference *typeRef = resolvedType(_compiledObject->inheritedTypeNameIndex);
        if (!typeRef) { // inside a binding on attached/group property
            Q_ASSERT(binding);
            if (binding->isAttachedProperty())
                return { 0, _propertyCache->propertyCount() }; // 3.
            Q_ASSERT(binding->isGroupProperty());
            return { 0, _propertyCache->propertyOffset() + 1 }; // 4.
        }
        Q_ASSERT(!(_compiledObject->flags & QV4::CompiledData::Object::IsComponent));
        QQmlType type = typeRef->type();
        if (type.isValid() && !type.isInlineComponentType()) {
            return { 0, _propertyCache->propertyCount() }; // 1.
        }
        // Q_ASSERT(type.isComposite());
        return { _propertyCache->propertyOffset(), _propertyCache->propertyCount() }; // 2.
    };
    const auto [offset, count] = getPropertyCacheRange();
    for (int i = offset; i < count; ++i) {
        QQmlPropertyData *propertyData = _propertyCache->maybeUnresolvedProperty(i);
        if (!propertyData)
            continue;
        // TODO: the property might be a group property (in which case we need
        // to dive into its sub-properties and check whether there are any
        // required elements there) - QTBUG-96544
        if (!propertyData->isRequired() && postHocRequired.isEmpty())
            continue;
        QString name = propertyData->name(_qobject);
        auto postHocIt = postHocRequired.find(name);
        if (!propertyData->isRequired() && postHocRequired.end() == postHocIt )
            continue;

        if (postHocIt != postHocRequired.end())
            postHocRequired.erase(postHocIt);

        sharedState->hadRequiredProperties = true;
        sharedState->requiredProperties.insert(
                propertyData,
                RequiredPropertyInfo {
                        name, compilationUnit->finalUrl(), _compiledObject->location, {} });
    }

    if (binding && binding->isAttachedProperty()
        && sharedState->requiredProperties.size() != oldRequiredPropertiesCount) {
        recordError(
                binding->location,
                QLatin1String("Attached property has required properties. This is not supported"));
    }

    // Note: there's a subtle case with the above logic: if we process a random
    // QML-defined leaf type, it could have a required attribute overwrite on an
    // *existing* property: `import QtQuick; Text { required text }`. in this
    // case, we must add the property to a required list
    if (!postHocRequired.isEmpty()) {
        // NB: go through [0, offset) range as [offset, count) is already done
        for (int i = 0; i < offset; ++i) {
            QQmlPropertyData *propertyData = _propertyCache->maybeUnresolvedProperty(i);
            if (!propertyData)
                continue;
            QString name = propertyData->name(_qobject);
            auto postHocIt = postHocRequired.find(name);
            if (postHocRequired.end() == postHocIt)
                continue;
            postHocRequired.erase(postHocIt);

            sharedState->hadRequiredProperties = true;
            sharedState->requiredProperties.insert(
                    propertyData,
                    RequiredPropertyInfo {
                            name, compilationUnit->finalUrl(), _compiledObject->location, {} });
        }
    }

    if (!postHocRequired.isEmpty() && hadInheritedRequiredProperties)
        recordError({}, QLatin1String("Property %1 was marked as required but does not exist").arg(*postHocRequired.begin()));

    if (_compiledObject->nFunctions > 0)
        setupFunctions();
    setupBindings();

    for (int aliasIndex = 0; aliasIndex != _compiledObject->aliasCount(); ++aliasIndex) {
        const QV4::CompiledData::Alias* alias = _compiledObject->aliasesBegin() + aliasIndex;
        const auto originalAlias = alias;
        while (alias->aliasToLocalAlias)
            alias = _compiledObject->aliasesBegin() + alias->localAliasIndex;
        Q_ASSERT(alias->flags & QV4::CompiledData::Alias::Resolved);
        if (!context->isIdValueSet(0)) // TODO: Do we really want 0 here?
            continue;
        QObject *target = context->idValue(alias->targetObjectId);
        if (!target)
            continue;
        QQmlData *targetDData = QQmlData::get(target, /*create*/false);
        if (targetDData == nullptr || targetDData->propertyCache == nullptr)
            continue;
        int coreIndex = QQmlPropertyIndex::fromEncoded(alias->encodedMetaPropertyIndex).coreIndex();
        QQmlPropertyData *const targetProperty = targetDData->propertyCache->property(coreIndex);
        if (!targetProperty)
            continue;
        auto it = sharedState->requiredProperties.find(targetProperty);
        if (it != sharedState->requiredProperties.end())
            it->aliasesToRequired.push_back(AliasToRequiredInfo {compilationUnit->stringAt(originalAlias->nameIndex), compilationUnit->finalUrl()});
    }

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
