// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
#include <private/qqmlanybinding_p.h>
#include <QtQml/private/qqmlvme_p.h>

#include <QScopedValueRollback>

#include <qtqml_tracepoints_p.h>
#include <QScopedValueRollback>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcQmlDefaultMethod, "qt.qml.defaultmethod")

QT_USE_NAMESPACE

Q_TRACE_PREFIX(qtqml,
"namespace QV4 {" \
"struct ExecutionEngine;" \
"class ExecutableCompilationUnit;" \
"namespace CompiledData {" \
"struct Object;" \
"}}" \
"class QQmlEngine;"
)

Q_TRACE_POINT(qtqml, QQmlObjectCreator_createInstance_entry, const QV4::ExecutableCompilationUnit *compilationUnit, const QV4::CompiledData::Object *object, const QUrl &url)
Q_TRACE_POINT(qtqml, QQmlObjectCreator_createInstance_exit, const QString &typeName)

QQmlObjectCreator::QQmlObjectCreator(
        const QQmlRefPointer<QQmlContextData> &parentContext,
        const QQmlRefPointer<QV4::ExecutableCompilationUnit> &compilationUnit,
        const QQmlRefPointer<QQmlContextData> &creationContext,
        const QString &inlineComponentName,
        QQmlIncubatorPrivate *incubator)
    : phase(Startup)
    , m_inlineComponentName(inlineComponentName)
    , compilationUnit(compilationUnit)
    , propertyCaches(compilationUnit->propertyCachesPtr())
    , sharedState(new QQmlObjectCreatorSharedState, QQmlRefPointer<QQmlObjectCreatorSharedState>::Adopt)
    , topLevelCreator(true)
    , isContextObject(true)
    , incubator(incubator)
{
    init(parentContext);

    sharedState->componentAttached = nullptr;
    sharedState->allCreatedBindings.allocate(compilationUnit->totalBindingsCount(inlineComponentName));
    sharedState->allParserStatusCallbacks.allocate(compilationUnit->totalParserStatusCount(inlineComponentName));
    sharedState->allCreatedObjects.allocate(compilationUnit->totalObjectCount(inlineComponentName));
    sharedState->allJavaScriptObjects = ObjectInCreationGCAnchorList();
    sharedState->creationContext = creationContext;
    sharedState->rootContext.reset();
    sharedState->hadTopLevelRequiredProperties = false;

    if (auto profiler = QQmlEnginePrivate::get(engine)->profiler) {
        Q_QML_PROFILE_IF_ENABLED(QQmlProfilerDefinitions::ProfileCreating, profiler,
                sharedState->profiler.init(profiler, compilationUnit->totalParserStatusCount(inlineComponentName)));
    } else {
        Q_UNUSED(profiler);
    }
}

QQmlObjectCreator::QQmlObjectCreator(const QQmlRefPointer<QQmlContextData> &parentContext,
                                     const QQmlRefPointer<QV4::ExecutableCompilationUnit> &compilationUnit, const QString inlineComponentName,
                                     QQmlObjectCreatorSharedState *inheritedSharedState, bool isContextObject)
    : phase(Startup)
    , m_inlineComponentName(inlineComponentName)
    , compilationUnit(compilationUnit)
    , propertyCaches(compilationUnit->propertyCachesPtr())
    , sharedState(inheritedSharedState)
    , topLevelCreator(false)
    , isContextObject(isContextObject)
    , incubator(nullptr)
{
    init(parentContext);
}

void QQmlObjectCreator::init(const QQmlRefPointer<QQmlContextData> &providedParentContext)
{
    parentContext = providedParentContext;
    engine = parentContext->engine();
    v4 = engine->handle();

    Q_ASSERT(compilationUnit);
    Q_ASSERT(compilationUnit->engine == v4);
    if (!compilationUnit->runtimeStrings)
        compilationUnit->populate();

    qmlUnit = compilationUnit->unitData();
    _qobject = nullptr;
    _scopeObject = nullptr;
    _bindingTarget = nullptr;
    _valueTypeProperty = nullptr;
    _compiledObject = nullptr;
    _compiledObjectIndex = -1;
    _ddata = nullptr;
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
            if (compilationUnit->componentsAreBound()
                    && compilationUnit != parentContext->typeCompilationUnit()) {
                recordError({}, tr("Cannot instantiate bound inline component in different file"));
                phase = ObjectsCreated;
                return nullptr;
            }
            objectToCreate = subComponentIndex;
            isComponentRoot = true;
        } else {
            Q_ASSERT(flags & CreationFlags::NormalObject);
            if (compilationUnit->componentsAreBound()
                    && sharedState->creationContext != parentContext) {
                recordError({}, tr("Cannot instantiate bound component "
                                   "outside its creation context"));
                phase = ObjectsCreated;
                return nullptr;
            }
            const QV4::CompiledData::Object *compObj = compilationUnit->objectAt(subComponentIndex);
            objectToCreate = compObj->bindingTable()->value.objectIndex;
        }
    }

    context = QQmlEnginePrivate::get(engine)->createInternalContext(
            compilationUnit, parentContext, subComponentIndex, isComponentRoot);

    if (!sharedState->rootContext) {
        sharedState->rootContext = context;
        sharedState->rootContext->setIncubator(incubator);
        sharedState->rootContext->setRootObjectInCreation(true);
    }

    QV4::Scope scope(v4);

    Q_ASSERT(sharedState->allJavaScriptObjects.canTrack() || topLevelCreator);
    if (topLevelCreator)
        sharedState->allJavaScriptObjects = ObjectInCreationGCAnchorList(scope, compilationUnit->totalObjectCount(m_inlineComponentName));

    if (!isComponentRoot && sharedState->creationContext) {
        // otherwise QQmlEnginePrivate::createInternalContext() handles it
        QV4::ScopedValue scripts(scope, sharedState->creationContext->importedScripts());
        context->setImportedScripts(v4, scripts);
    }

    QObject *instance = createInstance(objectToCreate, parent, /*isContextObject*/true);
    if (instance) {
        QQmlData *ddata = QQmlData::get(instance);
        Q_ASSERT(ddata);
        ddata->compilationUnit = compilationUnit;
    }

    if (topLevelCreator)
        sharedState->allJavaScriptObjects = ObjectInCreationGCAnchorList();

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
    Q_ASSERT(!sharedState->allJavaScriptObjects.canTrack());

    // FIXME (QTBUG-122956): allocating from the short lived scope does not make any sense
    QV4::Scope valueScope(v4);
    sharedState->allJavaScriptObjects = ObjectInCreationGCAnchorList(valueScope, compilationUnit->totalObjectCount(m_inlineComponentName));
}

void QQmlObjectCreator::populateDeferred(QObject *instance, int deferredIndex,
                                         const QQmlPropertyPrivate *qmlProperty,
                                         const QV4::CompiledData::Binding *binding)
{
    doPopulateDeferred(instance, deferredIndex, [this, qmlProperty, binding]() {
        Q_ASSERT(qmlProperty);
        Q_ASSERT(binding->hasFlag(QV4::CompiledData::Binding::IsDeferredBinding));

        QQmlListProperty<void> savedList;
        qSwap(_currentList, savedList);

        const QQmlPropertyData &property = qmlProperty->core;

        if (property.propType().flags().testFlag(QMetaType::IsQmlList)) {
            void *argv[1] = { (void*)&_currentList };
            QMetaObject::metacall(_qobject, QMetaObject::ReadProperty, property.coreIndex(), argv);
        } else if (_currentList.object) {
            _currentList = QQmlListProperty<void>();
        }

        setPropertyBinding(&property, binding);

        qSwap(_currentList, savedList);
    });
}

void QQmlObjectCreator::populateDeferred(QObject *instance, int deferredIndex)
{
    doPopulateDeferred(instance, deferredIndex, [this]() { setupBindings(ApplyDeferred); });
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
    if (binding) {
        populateDeferred(qmlProperty.object(), deferredIndex, QQmlPropertyPrivate::get(qmlProperty),
                         binding);
    } else {
        populateDeferred(qmlProperty.object(), deferredIndex);
    }
}

void QQmlObjectCreator::populateDeferredInstance(
        QObject *outerObject, int deferredIndex, int index, QObject *instance,
        QObject *bindingTarget, const QQmlPropertyData *valueTypeProperty,
        const QV4::CompiledData::Binding *binding)
{
    doPopulateDeferred(outerObject, deferredIndex, [&]() {
        populateInstance(index, instance, bindingTarget, valueTypeProperty, binding);
    });
}

void QQmlObjectCreator::finalizePopulateDeferred()
{
    phase = ObjectsCreated;
}

void QQmlObjectCreator::setPropertyValue(const QQmlPropertyData *property, const QV4::CompiledData::Binding *binding)
{
    QQmlPropertyData::WriteFlags propertyWriteFlags = QQmlPropertyData::BypassInterceptor | QQmlPropertyData::RemoveBindingOnAliasWrite;
    QV4::Scope scope(v4);

    QMetaType propertyType = property->propType();

    if (property->isEnum()) {
        if (binding->hasFlag(QV4::CompiledData::Binding::IsResolvedEnum) ||
                // TODO: For historical reasons you can assign any number to an enum property alias
                //       This can be fixed with an opt-out mechanism, for example a pragma.
                (property->isAlias() && binding->isNumberBinding())) {
            propertyType = property->propType().underlyingType();
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
        Q_ASSERT(ok || binding->type() == QV4::CompiledData::Binding::Type_Null);
        Q_UNUSED(ok);
    };

    auto assertType = [&](QV4::CompiledData::Binding::Type type)
    {
        Q_ASSERT(binding->type()== type || binding->type() == QV4::CompiledData::Binding::Type_Null);
        Q_UNUSED(type);
    };

    if (property->isQObject()) {
        if (binding->type() == QV4::CompiledData::Binding::Type_Null) {
            QObject *value = nullptr;
            const bool ok = property->writeProperty(_qobject, &value, propertyWriteFlags);
            Q_ASSERT(ok);
            Q_UNUSED(ok);
            return;
        }
    }

    switch (propertyType.id()) {
    case QMetaType::QVariant: {
        if (binding->type() == QV4::CompiledData::Binding::Type_Number) {
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
        } else if (binding->type() == QV4::CompiledData::Binding::Type_Boolean) {
            if (property->isVarProperty()) {
                _vmeMetaObject->setVMEProperty(property->coreIndex(), QV4::Value::fromBoolean(binding->valueAsBoolean()));
            } else {
                QVariant value(binding->valueAsBoolean());
                property->writeProperty(_qobject, &value, propertyWriteFlags);
            }
        } else if (binding->type() == QV4::CompiledData::Binding::Type_Null) {
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
    case QMetaType::SChar: {
        assertType(QV4::CompiledData::Binding::Type_Number);
        double d = compilationUnit->bindingValueAsNumber(binding);
        qint8 value = qint8(d);
        property->writeProperty(_qobject, &value, propertyWriteFlags);
        break;
    }
    case QMetaType::UChar: {
        assertType(QV4::CompiledData::Binding::Type_Number);
        double d = compilationUnit->bindingValueAsNumber(binding);
        quint8 value = quint8(d);
        property->writeProperty(_qobject, &value, propertyWriteFlags);
        break;
    }
    case QMetaType::Short: {
        assertType(QV4::CompiledData::Binding::Type_Number);
        double d = compilationUnit->bindingValueAsNumber(binding);
        qint16 value = qint16(d);
        property->writeProperty(_qobject, &value, propertyWriteFlags);
        break;
    }
    case QMetaType::UShort: {
        assertType(QV4::CompiledData::Binding::Type_Number);
        double d = compilationUnit->bindingValueAsNumber(binding);
        quint16 value = quint16(d);
        property->writeProperty(_qobject, &value, propertyWriteFlags);
        break;
    }
    case QMetaType::LongLong: {
        assertType(QV4::CompiledData::Binding::Type_Number);
        double d = compilationUnit->bindingValueAsNumber(binding);
        qint64 value = qint64(d);
        property->writeProperty(_qobject, &value, propertyWriteFlags);
        break;
    }
    case QMetaType::ULongLong: {
        assertType(QV4::CompiledData::Binding::Type_Number);
        double d = compilationUnit->bindingValueAsNumber(binding);
        quint64 value = quint64(d);
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
        QVariant data = QQmlValueTypeProvider::createValueType(
                    compilationUnit->bindingValueAsString(binding), propertyType);
        if (data.isValid()) {
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
        QVariant result = QQmlValueTypeProvider::createValueType(
                    compilationUnit->bindingValueAsString(binding), propertyType);
        assertOrNull(result.isValid());
        property->writeProperty(_qobject, result.data(), propertyWriteFlags);
        break;
    }
    default: {
        // generate single literal value assignment to a list property if required
        if (propertyType == QMetaType::fromType<QList<qreal>>()) {
            assertType(QV4::CompiledData::Binding::Type_Number);
            QList<qreal> value;
            value.append(compilationUnit->bindingValueAsNumber(binding));
            property->writeProperty(_qobject, &value, propertyWriteFlags);
            break;
        } else if (propertyType == QMetaType::fromType<QList<int>>()) {
            assertType(QV4::CompiledData::Binding::Type_Number);
            double n = compilationUnit->bindingValueAsNumber(binding);
            QList<int> value;
            value.append(int(n));
            property->writeProperty(_qobject, &value, propertyWriteFlags);
            break;
        } else if (propertyType == QMetaType::fromType<QList<bool>>()) {
            assertType(QV4::CompiledData::Binding::Type_Boolean);
            QList<bool> value;
            value.append(binding->valueAsBoolean());
            property->writeProperty(_qobject, &value, propertyWriteFlags);
            break;
        } else if (propertyType == QMetaType::fromType<QList<QUrl>>()) {
            assertType(QV4::CompiledData::Binding::Type_String);
            const QUrl url(compilationUnit->bindingValueAsString(binding));
            QList<QUrl> value {
                QQmlPropertyPrivate::resolveUrlsOnAssignment()
                        ? compilationUnit->finalUrl().resolved(url)
                        : url
            };
            property->writeProperty(_qobject, &value, propertyWriteFlags);
            break;
        } else if (propertyType == QMetaType::fromType<QList<QString>>()) {
            assertOrNull(binding->evaluatesToString());
            QList<QString> value;
            value.append(compilationUnit->bindingValueAsString(binding));
            property->writeProperty(_qobject, &value, propertyWriteFlags);
            break;
        } else if (propertyType == QMetaType::fromType<QJSValue>()) {
            QJSValue value;
            switch (binding->type()) {
            case QV4::CompiledData::Binding::Type_Boolean:
                value = QJSValue(binding->valueAsBoolean());
                break;
            case QV4::CompiledData::Binding::Type_Number: {
                const double n = compilationUnit->bindingValueAsNumber(binding);
                if (double(int(n)) == n)
                    value = QJSValue(int(n));
                else
                    value = QJSValue(n);
                break;
            }
            case QV4::CompiledData::Binding::Type_Null:
                value = QJSValue::NullValue;
                break;
            default:
                value = QJSValue(compilationUnit->bindingValueAsString(binding));
                break;
            }
            property->writeProperty(_qobject, &value, propertyWriteFlags);
            break;
        } else {
            QVariant source;
            switch (binding->type()) {
            case QV4::CompiledData::Binding::Type_Boolean:
                source = binding->valueAsBoolean();
                break;
            case QV4::CompiledData::Binding::Type_Number: {
                const double n = compilationUnit->bindingValueAsNumber(binding);
                if (double(int(n)) == n)
                    source = int(n);
                else
                    source = n;
                break;
            }
            case QV4::CompiledData::Binding::Type_Null:
                source = QVariant::fromValue<std::nullptr_t>(nullptr);
                break;
            case QV4::CompiledData::Binding::Type_Invalid:
                break;
            default:
                source = compilationUnit->bindingValueAsString(binding);
                break;
            }

            QVariant target = QQmlValueTypeProvider::createValueType(
                    source, propertyType, engine->handle());
            if (target.isValid()) {
                property->writeProperty(_qobject, target.data(), propertyWriteFlags);
                break;
            }
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

void QQmlObjectCreator::setupBindings(BindingSetupFlags mode)
{
    QQmlListProperty<void> savedList;
    qSwap(_currentList, savedList);

    const QV4::CompiledData::BindingPropertyData *propertyData
            = compilationUnit->bindingPropertyDataPerObjectAt(_compiledObjectIndex);

    if (_compiledObject->idNameIndex) {
        const QQmlPropertyData *idProperty = propertyData->last();
        Q_ASSERT(!idProperty || !idProperty->isValid() || idProperty->name(_qobject) == QLatin1String("id"));
        if (idProperty && idProperty->isValid() && idProperty->isWritable() && idProperty->propType().id() == QMetaType::QString) {
            QV4::CompiledData::Binding idBinding;
            idBinding.propertyNameIndex = 0; // Not used
            idBinding.clearFlags();
            idBinding.setType(QV4::CompiledData::Binding::Type_String);
            idBinding.stringIndex = _compiledObject->idNameIndex;
            idBinding.location = _compiledObject->location; // ###
            idBinding.value.nullMarker = 0; // zero the value field to make codechecker happy
            setPropertyValue(idProperty, &idBinding);
        }
    }

    // ### this is best done through type-compile-time binding skip lists.
    if (_valueTypeProperty) {
        QQmlAbstractBinding *binding = QQmlPropertyPrivate::binding(_bindingTarget, QQmlPropertyIndex(_valueTypeProperty->coreIndex()));

        if (binding && binding->kind() != QQmlAbstractBinding::ValueTypeProxy) {
            QQmlPropertyPrivate::removeBinding(_bindingTarget, QQmlPropertyIndex(_valueTypeProperty->coreIndex()));
        } else if (binding) {
            QQmlValueTypeProxyBinding *proxy = static_cast<QQmlValueTypeProxyBinding *>(binding);

            if (qmlTypeForObject(_bindingTarget).isValid()) {
                quint32 bindingSkipList = 0;

                const QQmlPropertyData *defaultProperty = _compiledObject->indexOfDefaultPropertyOrAlias != -1 ? _propertyCache->parent()->defaultProperty() : _propertyCache->defaultProperty();

                const QV4::CompiledData::Binding *binding = _compiledObject->bindingTable();
                for (quint32 i = 0; i < _compiledObject->nBindings; ++i, ++binding) {
                    const QQmlPropertyData *property = binding->propertyNameIndex != 0
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
        const QQmlPropertyData *const property = propertyData->at(i);
        if (property) {
            const QQmlPropertyData *targetProperty = property;
            if (targetProperty->isAlias()) {
                // follow alias
                QQmlPropertyIndex originalIndex(targetProperty->coreIndex(), _valueTypeProperty ? _valueTypeProperty->coreIndex() : -1);
                auto [targetObject, targetIndex] = QQmlPropertyPrivate::findAliasTarget(_bindingTarget, originalIndex);
                QQmlData *data = QQmlData::get(targetObject);
                Q_ASSERT(data && data->propertyCache);
                targetProperty = data->propertyCache->property(targetIndex.coreIndex());
                sharedState->requiredProperties.remove({targetObject, targetProperty});
            }
            sharedState->requiredProperties.remove({_bindingTarget, property});
        }


        if (binding->hasFlag(QV4::CompiledData::Binding::IsCustomParserBinding))
            continue;

        if (binding->hasFlag(QV4::CompiledData::Binding::IsDeferredBinding)) {
            if (!(mode & ApplyDeferred))
                continue;
        } else if (!(mode & ApplyImmediate)) {
            continue;
        }

        if (property && property->propType().flags().testFlag(QMetaType::IsQmlList)) {
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
    const QV4::CompiledData::Binding::Type bindingType = binding->type();
    if (bindingType == QV4::CompiledData::Binding::Type_AttachedProperty) {
        Q_ASSERT(stringAt(compilationUnit->objectAt(binding->value.objectIndex)->inheritedTypeNameIndex).isEmpty());
        QV4::ResolvedTypeReference *tr = resolvedType(binding->propertyNameIndex);
        Q_ASSERT(tr);
        QQmlType attachedType = tr->type();
        QQmlEnginePrivate *enginePrivate = QQmlEnginePrivate::get(engine);
        if (!attachedType.isValid()) {
            QQmlTypeNameCache::Result res = context->imports()->query(
                    stringAt(binding->propertyNameIndex), QQmlTypeLoader::get(enginePrivate));
            if (res.isValid())
                attachedType = res.type;
            else
                return false;
        }
        QObject *qmlObject = qmlAttachedPropertiesObject(
                _qobject, attachedType.attachedPropertiesFunction(enginePrivate));
        if (!qmlObject) {
            recordError(binding->location,
                        QStringLiteral("Could not create attached properties object '%1'")
                        .arg(QString::fromUtf8(attachedType.typeName())));
            return false;
        }

        if (!populateInstance(binding->value.objectIndex, qmlObject, qmlObject,
                              /*value type property*/ nullptr, binding))
            return false;
        return true;
    }

    // ### resolve this at compile time
    if (bindingProperty && bindingProperty->propType() == QMetaType::fromType<QQmlScriptString>()) {
        QQmlScriptString ss(compilationUnit->bindingValueAsScriptString(binding),
                            context->asQQmlContext(), _scopeObject);
        ss.d.data()->bindingId = bindingType == QV4::CompiledData::Binding::Type_Script ? binding->value.compiledScriptIndex : (quint32)QQmlBinding::Invalid;
        ss.d.data()->lineNumber = binding->location.line();
        ss.d.data()->columnNumber = binding->location.column();
        ss.d.data()->isStringLiteral = bindingType == QV4::CompiledData::Binding::Type_String;
        ss.d.data()->isNumberLiteral = bindingType == QV4::CompiledData::Binding::Type_Number;
        ss.d.data()->numberValue = compilationUnit->bindingValueAsNumber(binding);

        QQmlPropertyData::WriteFlags propertyWriteFlags = QQmlPropertyData::BypassInterceptor |
                                                            QQmlPropertyData::RemoveBindingOnAliasWrite;
        int propertyWriteStatus = -1;
        void *argv[] = { &ss, nullptr, &propertyWriteStatus, &propertyWriteFlags };
        QMetaObject::metacall(_qobject, QMetaObject::WriteProperty, bindingProperty->coreIndex(), argv);
        return true;
    }

    QObject *createdSubObject = nullptr;
    if (bindingType == QV4::CompiledData::Binding::Type_Object) {
        // This is not a top level object. Its required properties don't count towards the
        // top level required properties.
        QScopedValueRollback topLevelRequired(sharedState->hadTopLevelRequiredProperties);
        createdSubObject = createInstance(binding->value.objectIndex, _bindingTarget);
        if (!createdSubObject)
            return false;
    }

    if (bindingType == QV4::CompiledData::Binding::Type_GroupProperty) {
        const QV4::CompiledData::Object *obj = compilationUnit->objectAt(binding->value.objectIndex);
        if (stringAt(obj->inheritedTypeNameIndex).isEmpty()) {

            QObject *groupObject = nullptr;
            QQmlGadgetPtrWrapper *valueType = nullptr;
            const QQmlPropertyData *valueTypeProperty = nullptr;
            QObject *bindingTarget = _bindingTarget;
            int groupObjectIndex = binding->value.objectIndex;

            if (!bindingProperty) {
                for (int i = 0, end = compilationUnit->objectCount(); i != end; ++i) {
                    const QV4::CompiledData::Object *external = compilationUnit->objectAt(i);
                    if (external->idNameIndex == binding->propertyNameIndex) {
                        bindingTarget = groupObject = context->idValue(external->objectId());
                        break;
                    }
                }
                if (!groupObject)
                    return true;
            } else if (QQmlMetaType::isValueType(bindingProperty->propType())) {
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
                    QQmlPropertyIndex index(bindingProperty->coreIndex());
                    auto anyBinding = QQmlAnyBinding::ofProperty(_qobject, index);
                    if (anyBinding) {
                        // if there is a binding, try to force-evaluate it now
                        // this might instantiate a necessary part of a grouped property
                        anyBinding.refresh();
                        QMetaObject::metacall(_qobject, QMetaObject::ReadProperty, bindingProperty->coreIndex(), argv);
                    }
                    if (!groupObject) {
                        recordError(binding->location, tr("Cannot set properties on %1 as it is null").arg(stringAt(binding->propertyNameIndex)));
                        return false;
                    }
                }

                bindingTarget = groupObject;
            }

            if (!populateInstance(groupObjectIndex, groupObject, bindingTarget, valueTypeProperty,
                                  binding)) {
                return false;
            }

            if (valueType)
                valueType->write(_qobject, bindingProperty->coreIndex(), QQmlPropertyData::BypassInterceptor);

            return true;
        }
    }

    if (!bindingProperty) // ### error
        return true;

    const QV4::CompiledData::Binding::Flags bindingFlags = binding->flags();
    const bool allowedToRemoveBinding
            = !(bindingFlags & QV4::CompiledData::Binding::IsSignalHandlerExpression)
            && !(bindingFlags & QV4::CompiledData::Binding::IsOnAssignment)
            && !(bindingFlags & QV4::CompiledData::Binding::IsPropertyObserver)
            && !_valueTypeProperty;

    if (allowedToRemoveBinding) {
        if (bindingProperty->acceptsQBinding()) {
            removePendingBinding(_bindingTarget, bindingProperty->coreIndex());
        } else {
            QQmlPropertyPrivate::removeBinding(
                    _bindingTarget, QQmlPropertyIndex(bindingProperty->coreIndex()));
        }
    }

    if (bindingType == QV4::CompiledData::Binding::Type_Script || binding->isTranslationBinding()) {
        if (bindingFlags & QV4::CompiledData::Binding::IsSignalHandlerExpression
            || bindingFlags & QV4::CompiledData::Binding::IsPropertyObserver) {
            QV4::Function *runtimeFunction = compilationUnit->runtimeFunctions[binding->value.compiledScriptIndex];
            int signalIndex = _propertyCache->methodIndexToSignalIndex(bindingProperty->coreIndex());
            QQmlBoundSignalExpression *expr = new QQmlBoundSignalExpression(
                        _bindingTarget, signalIndex, context,
                        _scopeObject, runtimeFunction, currentQmlContext());

            if (bindingProperty->notifiesViaBindable()) {
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
        } else if (bindingProperty->acceptsQBinding()) {
            QUntypedPropertyBinding qmlBinding;
            if (binding->isTranslationBinding()) {
                qmlBinding = QQmlTranslationPropertyBinding::create(bindingProperty, compilationUnit, binding);
            } else {
                QV4::Function *runtimeFunction = compilationUnit->runtimeFunctions[binding->value.compiledScriptIndex];
                QQmlPropertyIndex index(bindingProperty->coreIndex(), -1);
                qmlBinding = QQmlPropertyBinding::create(bindingProperty, runtimeFunction, _scopeObject, context, currentQmlContext(), _bindingTarget, index);
            }
            sharedState.data()->allQPropertyBindings.push_back(DeferredQPropertyBinding {_bindingTarget, bindingProperty->coreIndex(), qmlBinding });

            QQmlData *data = QQmlData::get(_bindingTarget, true);
            data->setBindingBit(_bindingTarget, bindingProperty->coreIndex());
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

    if (bindingType == QV4::CompiledData::Binding::Type_Object) {
        if (bindingFlags & QV4::CompiledData::Binding::IsOnAssignment) {
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
        if (bindingFlags & QV4::CompiledData::Binding::IsSignalHandlerObject) {
            if (!bindingProperty->isFunction()) {
                recordError(binding->valueLocation, tr("Cannot assign an object to signal property %1").arg(bindingProperty->name(_qobject)));
                return false;
            }
            QMetaMethod method = QQmlMetaType::defaultMethod(createdSubObject);
            if (!method.isValid()) {
                recordError(binding->valueLocation, tr("Cannot assign object type %1 with no default method").arg(QString::fromLatin1(createdSubObject->metaObject()->className())));
                return false;
            }
            qCWarning(lcQmlDefaultMethod) << "Assigning an object to a signal handler is deprecated. "
                                             "Instead, create the object, give it an id, and call the desired slot "
                                             "from the signal handler. The object is:" << createdSubObject;

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

        if (const char *iid = QQmlMetaType::interfaceIId(bindingProperty->propType())) {
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
        } else if (bindingProperty->propType().flags().testFlag(QMetaType::IsQmlList)) {
            Q_ASSERT(_currentList.object);

            void *itemToAdd = createdSubObject;

            QMetaType listItemType = QQmlMetaType::listValueType(bindingProperty->propType());
            if (listItemType.isValid()) {
                const char *iid = QQmlMetaType::interfaceIId(listItemType);
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

        const QQmlPropertyData *property = _propertyCache->property(name, _qobject, context);
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
    error.setLine(qmlConvertSourceCoordinate<quint32, int>(location.line()));
    error.setColumn(qmlConvertSourceCoordinate<quint32, int>(location.column()));
    error.setDescription(description);
    errors << error;
}

void QQmlObjectCreator::registerObjectWithContextById(const QV4::CompiledData::Object *object, QObject *instance) const
{
    if (object->objectId() >= 0)
        context->setIdValue(object->objectId(), instance);
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

    if (obj->hasFlag(QV4::CompiledData::Object::IsComponent)) {
        isComponent = true;
        instance = createComponent(engine, compilationUnit.data(), index, parent, context);
        typeName = QStringLiteral("<component>");
        ddata = QQmlData::get(instance);
        Q_ASSERT(ddata); // we just created it inside createComponent
    } else {
        QV4::ResolvedTypeReference *typeRef = resolvedType(obj->inheritedTypeNameIndex);
        Q_ASSERT(typeRef);
        installPropertyCache = !typeRef->isFullyDynamicType();
        const QQmlType type = typeRef->type();
        if (type.isValid() && !type.isInlineComponentType()) {
            typeName = type.qmlTypeName();

            instance = type.createWithQQmlData();
            if (!instance) {
                recordError(obj->location, tr("Unable to create object of type %1").arg(stringAt(obj->inheritedTypeNameIndex)));
                return nullptr;
            }

            const int finalizerCast = type.finalizerCast();
            if (finalizerCast != -1) {
                auto hook = reinterpret_cast<QQmlFinalizerHook *>(reinterpret_cast<char *>(instance) + finalizerCast);
                sharedState->finalizeHooks.push_back(hook);
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
            auto compilationUnit = typeRef->compilationUnit();
            Q_ASSERT(compilationUnit);
            typeName = compilationUnit->fileName();
            // compilation unit is shared between root type and its inline component types
            // so isSingleton errorneously returns true for inline components
            if (compilationUnit->unitData()->isSingleton() && !type.isInlineComponentType()) {
                recordError(obj->location, tr("Composite Singleton Type %1 is not creatable").arg(stringAt(obj->inheritedTypeNameIndex)));
                return nullptr;
            }

            if (!type.isInlineComponentType()) {
                QQmlObjectCreator subCreator(
                        context, engine->handle()->executableCompilationUnit(
                                         std::move(compilationUnit)),
                        QString(), // not an inline component
                        sharedState.data(), isContextObject);
                instance = subCreator.create();
                if (!instance) {
                    errors += subCreator.errors;
                    return nullptr;
                }
            } else {
                const QString inlineComponentName = type.elementName();

                const int inlineComponentId
                        = compilationUnit->inlineComponentId(inlineComponentName);
                QQmlObjectCreator subCreator(
                        context,
                        engine->handle()->executableCompilationUnit(
                                QQmlRefPointer<QV4::CompiledData::CompilationUnit>(
                                        compilationUnit)),
                        inlineComponentName,
                        sharedState.data(),
                        isContextObject);
                instance = subCreator.create(
                        inlineComponentId, nullptr, nullptr, CreationFlags::InlineComponent);
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

    ddata->lineNumber = obj->location.line();
    ddata->columnNumber = obj->location.column();

    ddata->setImplicitDestructible();
    // inline components are root objects, but their index is != 0, so we need
    // an additional check
    const bool documentRoot = static_cast<quint32>(index) == /*root object*/ 0
            || ddata->rootObjectInCreation
            || obj->hasFlag(QV4::CompiledData::Object::IsInlineComponentRoot);
    context->installContext(
            ddata, documentRoot ? QQmlContextData::DocumentRoot : QQmlContextData::OrdinaryObject);

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

    if (customParser && obj->hasFlag(QV4::CompiledData::Object::HasCustomParserBindings)) {
        customParser->engine = QQmlEnginePrivate::get(engine);
        customParser->imports = compilationUnit->typeNameCache().data();

        QList<const QV4::CompiledData::Binding *> bindings;
        const QV4::CompiledData::Object *obj = compilationUnit->objectAt(index);
        const QV4::CompiledData::Binding *binding = obj->bindingTable();
        for (quint32 i = 0; i < obj->nBindings; ++i, ++binding) {
            if (binding->hasFlag(QV4::CompiledData::Binding::IsCustomParserBinding))
                bindings << binding;
        }
        customParser->applyBindings(instance, compilationUnit, bindings);

        customParser->engine = nullptr;
        customParser->imports = (QQmlTypeNameCache*)nullptr;
    }

    if (isComponent) {
        registerObjectWithContextById(obj, instance);
        return instance;
    }

    QQmlPropertyCache::ConstPtr cache = propertyCaches->at(index);
    Q_ASSERT(!cache.isNull());
    if (installPropertyCache)
        ddata->propertyCache = cache;

    QObject *scopeObject = instance;
    qSwap(_scopeObject, scopeObject);

    Q_ASSERT(sharedState->allJavaScriptObjects.canTrack());
    sharedState->allJavaScriptObjects.trackObject(v4, instance);

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
        if (b->kind() == QQmlAbstractBinding::QmlBinding) {
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

        QQmlData *data = QQmlData::get(target);
        if (!data || !data->hasBindingBit(index)) {
            // The target property has been overwritten since we stashed the binding.
            sharedState->allQPropertyBindings.pop_front();
            continue;
        }

        QUntypedBindable bindable;
        void *argv[] = { &bindable };
        // allow interception
        target->metaObject()->metacall(target, QMetaObject::BindableProperty, index, argv);
        const bool success = bindable.setBinding(qmlBinding);

        const auto bindingPrivateRefCount = QPropertyBindingPrivate::get(qmlBinding)->refCount();

        // Only pop_front after setting the binding as the bindings are refcounted.
        sharedState->allQPropertyBindings.pop_front();

        // If the binding was actually not set, it's deleted now.
        if (success && bindingPrivateRefCount > 1) {
            if (auto priv = QPropertyBindingPrivate::get(qmlBinding); priv->hasCustomVTable()) {
                auto qmlBindingPriv = static_cast<QQmlPropertyBinding *>(priv);
                auto jsExpression = qmlBindingPriv->jsExpression();
                const bool canRemove = !qmlBinding.error().hasError()
                        && !qmlBindingPriv->hasDependencies()
                        && !jsExpression->hasUnresolvedNames();
                if (canRemove)
                    bindable.takeBinding();
            }
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

    for (QQmlFinalizerHook *hook: sharedState->finalizeHooks) {
        hook->componentFinalized();
        if (watcher.hasRecursed())
            return false;
    }
    sharedState->finalizeHooks.clear();

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
    Q_ASSERT(instance);
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

    QQmlPropertyCache::ConstPtr cache = propertyCaches->at(_compiledObjectIndex);

    QQmlVMEMetaObject *vmeMetaObject = nullptr;
    if (propertyCaches->needsVMEMetaObject(_compiledObjectIndex)) {
        Q_ASSERT(!cache.isNull());
        // install on _object
        vmeMetaObject = new QQmlVMEMetaObject(v4, _qobject, cache, compilationUnit, _compiledObjectIndex);
        _ddata->propertyCache = cache;
        scopeObjectProtector = _ddata->jsWrapper.value();
    } else {
        vmeMetaObject = QQmlVMEMetaObject::get(_qobject);
    }

    registerObjectWithContextById(_compiledObject, _qobject);

    qSwap(_propertyCache, cache);
    qSwap(_vmeMetaObject, vmeMetaObject);

    _ddata->compilationUnit = compilationUnit;
    if (_compiledObject->hasFlag(QV4::CompiledData::Object::HasDeferredBindings))
        _ddata->deferData(_compiledObjectIndex, compilationUnit, context, m_inlineComponentName);

    const qsizetype oldRequiredPropertiesCount = sharedState->requiredProperties.size();
    QSet<QString> postHocRequired;
    for (auto it = _compiledObject->requiredPropertyExtraDataBegin(); it != _compiledObject->requiredPropertyExtraDataEnd(); ++it)
        postHocRequired.insert(stringAt(it->nameIndex));
    bool hadInheritedRequiredProperties = !postHocRequired.empty();

    for (int propertyIndex = 0; propertyIndex != _compiledObject->propertyCount(); ++propertyIndex) {
        const QV4::CompiledData::Property* property = _compiledObject->propertiesBegin() + propertyIndex;
        const QQmlPropertyData *propertyData = _propertyCache->property(_propertyCache->propertyOffset() + propertyIndex);
        // only compute stringAt if there's a chance for the lookup to succeed
        auto postHocIt = postHocRequired.isEmpty() ? postHocRequired.end() : postHocRequired.find(stringAt(property->nameIndex));
        if (!property->isRequired() && postHocRequired.end() == postHocIt)
            continue;
        if (postHocIt != postHocRequired.end())
            postHocRequired.erase(postHocIt);
        if (isContextObject)
            sharedState->hadTopLevelRequiredProperties = true;
        sharedState->requiredProperties.insert({_qobject, propertyData},
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
        Q_ASSERT(!_compiledObject->hasFlag(QV4::CompiledData::Object::IsComponent));
        QQmlType type = typeRef->type();
        if (type.isValid() && !type.isInlineComponentType()) {
            return { 0, _propertyCache->propertyCount() }; // 1.
        }
        // Q_ASSERT(type.isComposite());
        return { _propertyCache->propertyOffset(), _propertyCache->propertyCount() }; // 2.
    };
    const auto [offset, count] = getPropertyCacheRange();
    for (int i = offset; i < count; ++i) {
        const QQmlPropertyData *propertyData = _propertyCache->maybeUnresolvedProperty(i);
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

        if (isContextObject)
            sharedState->hadTopLevelRequiredProperties = true;
        sharedState->requiredProperties.insert(
                {_qobject, propertyData},
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
            const QQmlPropertyData *propertyData = _propertyCache->maybeUnresolvedProperty(i);
            if (!propertyData)
                continue;
            QString name = propertyData->name(_qobject);
            auto postHocIt = postHocRequired.find(name);
            if (postHocRequired.end() == postHocIt)
                continue;
            postHocRequired.erase(postHocIt);

            if (isContextObject)
                sharedState->hadTopLevelRequiredProperties = true;
            sharedState->requiredProperties.insert(
                    {_qobject, propertyData},
                    RequiredPropertyInfo {
                            name, compilationUnit->finalUrl(), _compiledObject->location, {} });
        }
    }

    if (!postHocRequired.isEmpty() && hadInheritedRequiredProperties)
        recordError({}, QLatin1String("Property %1 was marked as required but does not exist").arg(*postHocRequired.begin()));

    if (_compiledObject->nFunctions > 0)
        setupFunctions();
    setupBindings((binding && binding->hasFlag(QV4::CompiledData::Binding::IsDeferredBinding))
                  ? BindingMode::ApplyAll
                  : BindingMode::ApplyImmediate);

    for (int aliasIndex = 0; aliasIndex != _compiledObject->aliasCount(); ++aliasIndex) {
        const QV4::CompiledData::Alias* alias = _compiledObject->aliasesBegin() + aliasIndex;
        const auto originalAlias = alias;
        while (alias->isAliasToLocalAlias())
            alias = _compiledObject->aliasesBegin() + alias->localAliasIndex;
        Q_ASSERT(alias->hasFlag(QV4::CompiledData::Alias::Resolved));
        if (!context->isIdValueSet(0)) // TODO: Do we really want 0 here?
            continue;
        QObject *target = context->idValue(alias->targetObjectId());
        if (!target)
            continue;
        QQmlData *targetDData = QQmlData::get(target, /*create*/false);
        if (targetDData == nullptr || targetDData->propertyCache.isNull())
            continue;
        int coreIndex = QQmlPropertyIndex::fromEncoded(alias->encodedMetaPropertyIndex).coreIndex();
        const QQmlPropertyData *const targetProperty = targetDData->propertyCache->property(coreIndex);
        if (!targetProperty)
            continue;
        auto it = sharedState->requiredProperties.find({target, targetProperty});
        if (it != sharedState->requiredProperties.end())
            it->aliasesToRequired.push_back(
                    AliasToRequiredInfo {
                            compilationUnit->stringAt(originalAlias->nameIndex()),
                            compilationUnit->finalUrl()
                    });
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

/*!
    \internal
*/
QQmlComponent *QQmlObjectCreator::createComponent(QQmlEngine *engine,
                                                  QV4::ExecutableCompilationUnit *compilationUnit,
                                                  int index, QObject *parent,
                                                  const QQmlRefPointer<QQmlContextData> &context)
{
    QQmlComponent *component = new QQmlComponent(engine, compilationUnit, index, parent);
    QQmlComponentPrivate::get(component)->creationContext = context;
    QQmlData::get(component, /*create*/ true);
    return component;
}

QQmlObjectCreatorRecursionWatcher::QQmlObjectCreatorRecursionWatcher(QQmlObjectCreator *creator)
    : sharedState(creator->sharedState)
    , watcher(creator->sharedState.data())
{
}

void ObjectInCreationGCAnchorList::trackObject(QV4::ExecutionEngine *engine, QObject *instance)
{
    *allJavaScriptObjects = QV4::QObjectWrapper::wrap(engine, instance);
    // we have to handle the case where the gc is already running, but the scope is discarded
    // before the collector runs again. In that case, rescanning won't help us. Thus, mark the
    // object.
    QV4::WriteBarrier::markCustom(engine, [this](QV4::MarkStack *ms) {
        allJavaScriptObjects->heapObject()->mark(ms);
    });
    ++allJavaScriptObjects;
}
