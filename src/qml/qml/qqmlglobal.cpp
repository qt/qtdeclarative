// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtQml/private/qjsvalue_p.h>
#include <QtQml/private/qqmlglobal_p.h>
#include <QtQml/private/qqmlmetatype_p.h>
#include <QtQml/private/qv4qobjectwrapper_p.h>
#include <QtQml/qqmlengine.h>

#include <QtCore/private/qvariant_p.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qdebug.h>
#include <QtCore/qstringlist.h>

QT_BEGIN_NAMESPACE

// Pre-filter the metatype before poking QQmlMetaType::qmlType() and locking its mutex.
static bool isConstructibleMetaType(const QMetaType metaType)
{
    switch (metaType.id()) {
        // The builtins are not constructible this way.
        case QMetaType::Void:
        case QMetaType::Nullptr:
        case QMetaType::QVariant:
        case QMetaType::Int:
        case QMetaType::UInt:
        case QMetaType::LongLong:
        case QMetaType::ULongLong:
        case QMetaType::Float:
        case QMetaType::Double:
        case QMetaType::Long:
        case QMetaType::ULong:
        case QMetaType::Short:
        case QMetaType::UShort:
        case QMetaType::Char:
        case QMetaType::SChar:
        case QMetaType::UChar:
        case QMetaType::QChar:
        case QMetaType::QString:
        case QMetaType::Bool:
        case QMetaType::QDateTime:
        case QMetaType::QDate:
        case QMetaType::QTime:
        case QMetaType::QUrl:
        case QMetaType::QRegularExpression:
        case QMetaType::QByteArray:
        case QMetaType::QLocale:
        return false;
    default:
        break;
    }

    // QJSValue is also builtin
    if (metaType == QMetaType::fromType<QJSValue>())
        return false;

    // We also don't want to construct pointers of any kind, or lists, or enums.
    if (metaType.flags() &
            (QMetaType::PointerToQObject
             | QMetaType::IsEnumeration
             | QMetaType::SharedPointerToQObject
             | QMetaType::WeakPointerToQObject
             | QMetaType::TrackingPointerToQObject
             | QMetaType::IsUnsignedEnumeration
             | QMetaType::PointerToGadget
             | QMetaType::IsPointer
             | QMetaType::IsQmlList)) {
        return false;
    }

    return true;
}

template<typename JSValue>
JSValue nullValue()
{
    if constexpr (std::is_same_v<JSValue, QJSValue>)
        return QJSValue::NullValue;
    if constexpr (std::is_same_v<JSValue, QJSPrimitiveValue>)
        return QJSPrimitiveNull();
}

template<typename JSValue>
bool coerceToJSValue(QMetaType fromType, const void *from, JSValue *to)
{
    if (!fromType.isValid()) {
        *to = JSValue();
        return true;
    }

    if ((fromType.flags() & QMetaType::PointerToQObject)
            && *static_cast<QObject *const *>(from) == nullptr) {
        *to = nullValue<JSValue>();
        return true;
    }

    switch (fromType.id()) {
    case QMetaType::Void:
        *to = JSValue();
        return true;
    case QMetaType::Bool:
        *to = JSValue(*static_cast<const bool *>(from));
        return true;
    case QMetaType::Int:
        *to = JSValue(*static_cast<const int *>(from));
        return true;
    case QMetaType::Double:
        *to = JSValue(*static_cast<const double *>(from));
        return true;
    case QMetaType::QString:
        *to = JSValue(*static_cast<const QString *>(from));
        return true;
    case QMetaType::Nullptr:
        *to = nullValue<JSValue>();
        return true;
    }

    return false;
}

static bool coerceValue(
        QMetaType fromType, const void *from, QMetaType toType, void *to,
        QV4::ExecutionEngine *engine)
{
    // We would like to convert via metaTypeFromJS here, but that would allow conversion of
    // anything to numbers and strings. That would mean a value type with a ctor that takes
    // a number or a string would be constructible from any other value. While this would be
    // quite JavaScript-y, it's probably too permissive here.

    if (QMetaType::convert(fromType, from, toType, to))
        return true;

    if (toType == QMetaType::fromType<QJSPrimitiveValue>())
        return coerceToJSValue(fromType, from, static_cast<QJSPrimitiveValue *>(to));

    if (toType == QMetaType::fromType<QJSValue>()) {
        if (coerceToJSValue(fromType, from, static_cast<QJSValue *>(to)))
            return true;

        if (!engine)
            return false;

        QV4::Scope scope(engine);
        QV4::ScopedValue v(scope, scope.engine->metaTypeToJS(fromType, from));
        *static_cast<QJSValue *>(to) = QJSValuePrivate::fromReturnedValue(v->asReturnedValue());
        return true;
    }

    if (toType == QMetaType::fromType<QJSManagedValue>()) {
        if (!engine)
            return false;

        QV4::Scope scope(engine);
        QV4::ScopedValue v(scope, scope.engine->metaTypeToJS(fromType, from));
        *static_cast<QJSManagedValue *>(to) = QJSManagedValue(
                QJSValuePrivate::fromReturnedValue(v->asReturnedValue()), engine->jsEngine());
        return true;
    }


    return false;
}

static void *createVariantData(QMetaType type, QVariant *variant)
{
    const QtPrivate::QMetaTypeInterface *iface = type.iface();
    QVariant::Private *d = &variant->data_ptr();
    Q_ASSERT(d->is_null && !d->is_shared);
    *d = QVariant::Private(iface);
    if (QVariant::Private::canUseInternalSpace(iface))
        return d->data.data;

    // This is not exception safe.
    // If your value type throws an exception from its ctor bad things will happen anyway.
    d->data.shared = QVariant::PrivateShared::create(iface->size, iface->alignment);
    d->is_shared = true;
    return d->data.shared->data();
}

static void callConstructor(
        const QMetaObject *targetMetaObject, int i, void *source, void *target)
{
    void *p[] = { target, source };
    targetMetaObject->static_metacall(QMetaObject::ConstructInPlace, i, p);
}

template<typename Allocate>
static void fromVerifiedType(
        const QMetaObject *targetMetaObject, int ctorIndex, void *source, Allocate &&allocate)
{
    const QMetaMethod ctor = targetMetaObject->constructor(ctorIndex);
    Q_ASSERT_X(ctor.parameterCount() == 1, "fromVerifiedType",
               "Value type constructor must take exactly one argument");
    callConstructor(targetMetaObject, ctorIndex, source, allocate());
}


template<typename Allocate, typename Retrieve>
static bool fromMatchingType(
        const QMetaObject *targetMetaObject, Allocate &&allocate, Retrieve &&retrieve,
        QV4::ExecutionEngine *engine)
{
    const int end = targetMetaObject->constructorCount();
    for (int i = 0; i < end; ++i) {
        // Try construction from exact matches. (Score 0)
        const QMetaMethod ctor = targetMetaObject->constructor(i);
        if (ctor.parameterCount() != 1)
            continue;

        if (retrieve([&](QMetaType sourceMetaType, void *sourceData) {
            if (sourceMetaType != ctor.parameterMetaType(0))
                return false;
            callConstructor(targetMetaObject, i, sourceData, allocate());
            return true;
        })) {
            return true;
        }
    }

    for (int i = 0; i < end; ++i) {
        // Try construction from derived types. (Score 1)
        const QMetaMethod ctor = targetMetaObject->constructor(i);
        if (ctor.parameterCount() != 1)
            continue;

        const QMetaType parameterType = ctor.parameterMetaType(0);
        const QMetaObject *parameterMetaObject = parameterType.metaObject();
        if (!parameterMetaObject)
            continue;

        if (retrieve([&](QMetaType sourceMetaType, void *sourceData) {
            Q_ASSERT(sourceMetaType != parameterType);
            if (const QMetaObject *sourceMetaObject = sourceMetaType.metaObject();
                    sourceMetaObject && sourceMetaObject->inherits(parameterMetaObject)) {
                callConstructor(targetMetaObject, i, sourceData, allocate());
                return true;
            }
            return false;
        })) {
            return true;
        }
    }

    for (int i = 0; i < end; ++i) {
        // Try construction from converted types.
        // Do not recursively try to create parameters here. This may end up in infinite recursion.

        const QMetaMethod ctor = targetMetaObject->constructor(i);
        if (ctor.parameterCount() != 1)
            continue;

        const QMetaType parameterType = ctor.parameterMetaType(0);

        if (retrieve([&](QMetaType sourceMetaType, void *sourceData) {
            QVariant converted(parameterType);
            if (coerceValue(sourceMetaType, sourceData, parameterType, converted.data(), engine)) {
                callConstructor(targetMetaObject, i, converted.data(), allocate());
                return true;
            }

            return false;
        })) {
            return true;
        }
    }

    return false;
}

template<typename Allocate>
static bool fromMatchingType(
        const QMetaObject *targetMetaObject, const QV4::Value &source, Allocate &&allocate,
        QV4::ExecutionEngine *engine)
{
    QVariant variant;
    return fromMatchingType(targetMetaObject, std::forward<Allocate>(allocate), [&](auto callback) {
        if (!variant.isValid())
            variant = QV4::ExecutionEngine::toVariant(source, QMetaType());
        return callback(variant.metaType(), variant.data());
    }, engine);
}

template<typename Allocate>
static bool fromMatchingType(
        const QMetaObject *targetMetaObject, QVariant source, Allocate &&allocate,
        QV4::ExecutionEngine *engine)
{
    return fromMatchingType(targetMetaObject, std::forward<Allocate>(allocate), [&](auto callback) {
        return callback(source.metaType(), source.data());
    }, engine);
}

template<typename Allocate>
static bool fromString(const QMetaObject *mo, QString s, Allocate &&allocate)
{
    for (int i = 0, end = mo->constructorCount(); i < end; ++i) {
        const QMetaMethod ctor = mo->constructor(i);
        if (ctor.parameterCount() != 1)
            continue;

        if (ctor.parameterMetaType(0) == QMetaType::fromType<QString>()) {
            callConstructor(mo, i, &s, allocate());
            return true;
        }
    }

    return false;
}

template<typename Get, typename Convert>
static bool doWriteProperty(
        const QMetaProperty &metaProperty, void *target, Get &&get, Convert &&convert,
        QV4::ExecutionEngine *engine)
{
    const QMetaType propertyType = metaProperty.metaType();
    QVariant property = get(propertyType);
    if (property.metaType() == propertyType) {
        metaProperty.writeOnGadget(target, std::move(property));
        return true;
    }

    QVariant converted = convert(propertyType);
    if (converted.isValid()) {
        metaProperty.writeOnGadget(target, std::move(converted));
        return true;
    }

    converted = QVariant(propertyType);
    if (coerceValue(
                property.metaType(), property.constData(), propertyType, converted.data(),
                engine)) {
        metaProperty.writeOnGadget(target, std::move(converted));
        return true;
    }

    return false;
}

static void doWriteProperties(
        const QMetaObject *targetMetaObject, void *target, const QV4::Value &source,
        QV4::ExecutionEngine *engine)
{
    const QV4::Object *o = static_cast<const QV4::Object *>(&source);
    QV4::Scope scope(o->engine());
    QV4::ScopedObject object(scope, o);

    for (int i = 0; i < targetMetaObject->propertyCount(); ++i) {
        const QMetaProperty metaProperty = targetMetaObject->property(i);
        const QString propertyName = QString::fromUtf8(metaProperty.name());

        QV4::ScopedString v4PropName(scope, scope.engine->newString(propertyName));
        QV4::ScopedValue v4PropValue(scope, object->get(v4PropName));

        // We assume that data is freshly constructed.
        // There is no point in reset()'ing properties of a freshly created object.
        if (v4PropValue->isUndefined())
            continue;

        if (doWriteProperty(metaProperty, target, [&](const QMetaType &propertyType) {
            return QV4::ExecutionEngine::toVariant(v4PropValue, propertyType);
        }, [&](const QMetaType &propertyType) {
            return QQmlValueTypeProvider::createValueType(v4PropValue, propertyType, engine);
        }, engine)) {
            continue;
        }

        const QMetaType propertyType = metaProperty.metaType();
        QVariant property = QV4::ExecutionEngine::toVariant(v4PropValue, propertyType);
        if (property.metaType() == propertyType) {
            metaProperty.writeOnGadget(target, std::move(property));
            continue;
        }

        QVariant converted = QQmlValueTypeProvider::createValueType(
                v4PropValue, propertyType, engine);
        if (converted.isValid()) {
            metaProperty.writeOnGadget(target, std::move(converted));
            continue;
        }

        converted = QVariant(propertyType);
        if (coerceValue(
                    property.metaType(), property.constData(), propertyType, converted.data(),
                    engine)) {
            metaProperty.writeOnGadget(target, std::move(converted));
            continue;
        }

        qWarning().noquote()
                << QLatin1String("Could not convert %1 to %2 for property %3")
                   .arg(v4PropValue->toQStringNoThrow(), QString::fromUtf8(propertyType.name()),
                        propertyName);
    }
}

static QVariant byProperties(
    const QMetaObject *targetMetaObject, QMetaType metaType, const QV4::Value &source,
        QV4::ExecutionEngine *engine)
{
    if (!source.isObject() || !targetMetaObject)
        return QVariant();

    QVariant result(metaType);
    doWriteProperties(targetMetaObject, result.data(), source, engine);
    return result;
}

template<typename Read>
static void doWriteProperties(
    const QMetaObject *targetMetaObject, void *target,
    const QMetaObject *sourceMetaObject, Read &&read, QV4::ExecutionEngine *engine)
{
    for (int i = 0; i < targetMetaObject->propertyCount(); ++i) {
        const QMetaProperty metaProperty = targetMetaObject->property(i);

        const int sourceProperty = sourceMetaObject->indexOfProperty(metaProperty.name());

        // We assume that data is freshly constructed.
        // There is no point in reset()'ing properties of a freshly created object.
        if (sourceProperty == -1)
            continue;

        const QMetaType propertyType = metaProperty.metaType();
        QVariant property = read(sourceMetaObject, sourceProperty);
        if (property.metaType() == propertyType) {
            metaProperty.writeOnGadget(target, std::move(property));
            continue;
        }

        QVariant converted = QQmlValueTypeProvider::createValueType(property, propertyType, engine);
        if (converted.isValid()) {
            metaProperty.writeOnGadget(target, std::move(converted));
            continue;
        }

        converted = QVariant(propertyType);
        if (coerceValue(
                    property.metaType(), property.constData(), propertyType, converted.data(),
                    engine)) {
            metaProperty.writeOnGadget(target, std::move(converted));
            continue;
        }

        qWarning().noquote()
            << QLatin1String("Could not convert %1 to %2 for property %3")
                   .arg(property.toString(), QString::fromUtf8(propertyType.name()),
                        QString::fromUtf8(metaProperty.name()));
    }
}


static void doWriteProperties(
        const QMetaObject *targetMeta, void *target, QObject *source, QV4::ExecutionEngine *engine)
{
    doWriteProperties(
            targetMeta, target, source->metaObject(),
            [source](const QMetaObject *sourceMetaObject, int sourceProperty) {
        return sourceMetaObject->property(sourceProperty).read(source);
    }, engine);
}

static QVariant byProperties(
        const QMetaObject *targetMetaObject, QMetaType targetMetaType, QObject *source,
        QV4::ExecutionEngine *engine)
{
    if (!source || !targetMetaObject)
        return QVariant();

    QVariant result(targetMetaType);
    doWriteProperties(targetMetaObject, result.data(), source, engine);
    return result;
}

static QVariant byProperties(
        const QMetaObject *targetMetaObject, QMetaType targetMetaType,
        const QMetaObject *sourceMetaObject, const void *source, QV4::ExecutionEngine *engine)
{
    if (!source || !sourceMetaObject || !targetMetaObject)
        return QVariant();

    QVariant result(targetMetaType);
    doWriteProperties(
            targetMetaObject, result.data(), sourceMetaObject,
            [source](const QMetaObject *sourceMetaObject, int sourceProperty) {
        return sourceMetaObject->property(sourceProperty).readOnGadget(source);
    }, engine);
    return result;
}

template<typename Map>
void doWriteProperties(
        const QMetaObject *targetMetaObject, void *target, const Map &source,
        QV4::ExecutionEngine *engine)
{
    for (int i = 0; i < targetMetaObject->propertyCount(); ++i) {
        const QMetaProperty metaProperty = targetMetaObject->property(i);

        // We assume that data is freshly constructed.
        // There is no point in reset()'ing properties of a freshly created object.
        const auto it = source.constFind(QString::fromUtf8(metaProperty.name()));
        if (it == source.constEnd())
            continue;

        const QMetaType propertyType = metaProperty.metaType();
        QVariant property = *it;
        if (property.metaType() == propertyType) {
            metaProperty.writeOnGadget(target, std::move(property));
            continue;
        }

        QVariant converted = QQmlValueTypeProvider::createValueType(property, propertyType, engine);
        if (converted.isValid()) {
            metaProperty.writeOnGadget(target, std::move(converted));
            continue;
        }

        converted = QVariant(propertyType);
        if (coerceValue(
                    property.metaType(), property.constData(), propertyType, converted.data(),
                    engine)) {
            metaProperty.writeOnGadget(target, std::move(converted));
            continue;
        }

        qWarning().noquote()
            << QLatin1String("Could not convert %1 to %2 for property %3")
                   .arg(property.toString(), QString::fromUtf8(propertyType.name()),
                        QString::fromUtf8(metaProperty.name()));
    }
}

template<typename Map>
QVariant byProperties(
        const QMetaObject *targetMetaObject, QMetaType targetMetaType, const Map &source,
        QV4::ExecutionEngine *engine)
{
    QVariant result(targetMetaType);
    doWriteProperties(targetMetaObject, result.data(), source, engine);
    return result;
}

static QVariant byProperties(
        const QMetaObject *targetMetaObject, QMetaType targetMetaType, const QVariant &source,
        QV4::ExecutionEngine *engine)
{
    if (!targetMetaObject)
        return QVariant();

    if (source.metaType() == QMetaType::fromType<QJSValue>()) {
        QJSValue val = source.value<QJSValue>();
        // persistent to the GC and thus fromReturnedValue is safe.
        return byProperties(
                targetMetaObject, targetMetaType,
                QV4::Value::fromReturnedValue(QJSValuePrivate::asReturnedValue(&val)),
                engine);
    }

    if (source.metaType() == QMetaType::fromType<QVariantMap>()) {
        return byProperties(
                targetMetaObject, targetMetaType,
                *static_cast<const QVariantMap *>(source.constData()), engine);
    }

    if (source.metaType() == QMetaType::fromType<QVariantHash>()) {
        return byProperties(
                targetMetaObject, targetMetaType,
                *static_cast<const QVariantHash *>(source.constData()), engine);
    }

    if (source.metaType().flags() & QMetaType::PointerToQObject)
        return byProperties(targetMetaObject, targetMetaType, source.value<QObject *>(), engine);

    if (const QMetaObject *sourceMeta = QQmlMetaType::metaObjectForValueType(source.metaType())) {
        return byProperties(
                targetMetaObject, targetMetaType, sourceMeta, source.constData(), engine);
    }

    return QVariant();
}

template<typename Allocate, typename DefaultConstruct>
bool createOrConstructValueType(
        const QQmlType &targetType, const QV4::Value &source,
        Allocate &&allocate, DefaultConstruct &&defaultConstruct, QV4::ExecutionEngine *engine)
{
    const auto warn = [&](const QMetaObject *targetMetaObject) {
        qWarning().noquote()
                << "Could not find any constructor for value type"
                << targetMetaObject->className() << "to call with value"
                << source.toQStringNoThrow();
    };

    if (targetType.canPopulateValueType()) {
        if (const QMetaObject *targetMetaObject = targetType.metaObjectForValueType()) {
            if (source.isObject()) {
                doWriteProperties(targetMetaObject, defaultConstruct(), source, engine);
                return true;
            }
            if (targetType.canConstructValueType()) {
                if (fromMatchingType(targetMetaObject, source, allocate, engine))
                    return true;
                warn(targetMetaObject);
            }
        }
    } else if (targetType.canConstructValueType()) {
        if (const QMetaObject *targetMetaObject = targetType.metaObjectForValueType()) {
            if (fromMatchingType(targetMetaObject, source, allocate, engine))
                return true;
            warn(targetMetaObject);
        }
    }

    if (const auto valueTypeFunction = targetType.createValueTypeFunction()) {
        const QVariant result
            = valueTypeFunction(QJSValuePrivate::fromReturnedValue(source.asReturnedValue()));
        const QMetaType resultType = result.metaType();
        if (resultType == targetType.typeId()) {
            resultType.construct(allocate(), result.constData());
            return true;
        }
    }

    return false;
}

template<typename Allocate, typename DefaultConstruct>
bool createOrConstructValueType(
        const QQmlType &targetType, QMetaType sourceMetaType, void *source,
        Allocate &&allocate, DefaultConstruct &&defaultConstruct, QV4::ExecutionEngine *engine)
{

    const auto warn = [&](const QMetaObject *targetMetaObject) {
        qWarning().noquote()
            << "Could not find any constructor for value type"
            << targetMetaObject->className() << "to call with value" << source;
    };

    if (targetType.canPopulateValueType()) {
        if (const QMetaObject *targetMetaObject = targetType.metaObjectForValueType()) {
            if (const QMetaObject *sourceMetaObject
                    = QQmlMetaType::metaObjectForValueType(sourceMetaType)) {
                doWriteProperties(
                    targetMetaObject, defaultConstruct(), sourceMetaObject,
                    [&source](const QMetaObject *sourceMetaObject, int sourceProperty) {
                        return sourceMetaObject->property(sourceProperty).readOnGadget(source);
                    }, engine);
                return true;
            }

            if (sourceMetaType == QMetaType::fromType<QVariantMap>()) {
                doWriteProperties(
                        targetMetaObject, defaultConstruct(),
                        *static_cast<const QVariantMap *>(source), engine);
                return true;
            }

            if (sourceMetaType == QMetaType::fromType<QVariantHash>()) {
                doWriteProperties(
                        targetMetaObject, defaultConstruct(),
                        *static_cast<const QVariantHash *>(source), engine);
                return true;
            }

            if (sourceMetaType.flags() & QMetaType::PointerToQObject) {
                doWriteProperties(
                        targetMetaObject, defaultConstruct(),
                        *static_cast<QObject *const *>(source), engine);
                return true;
            }
        }
    }

    if (targetType.canConstructValueType()) {
        if (const QMetaObject *targetMetaObject = targetType.metaObjectForValueType()) {
            if (fromMatchingType(
                        targetMetaObject, std::forward<Allocate>(allocate), [&](auto callback) {
                    return callback(sourceMetaType, source);
                }, engine)) {
                return true;
            }
            warn(targetMetaObject);
        }
    }

    return false;
}

/*!
 * \internal
 * Populate the value type in place at \a target, which is expected to be
 * allocated and default-constructed, for example the result of a QVariant(QMetaType).
 * This is efficient if we can do byProperties() since it can use the pre-constructed object.
 * It also avoids the creation of a QVariant in most cases. It is not
 * efficient if you're going to create a QVariant anyway.
 */
bool QQmlValueTypeProvider::populateValueType(
        QMetaType targetMetaType, void *target, QMetaType sourceMetaType, void *source,
        QV4::ExecutionEngine *engine)
{
    if (sourceMetaType == QMetaType::fromType<QJSValue>()) {
        const QJSValue *val = static_cast<const QJSValue *>(source);
        // Generally, the GC might collect a Value at any point so that
        // a `ScopedValue` should be used.
        // In this case, the Value is tied to a `QJSValue` which is
        // persistent to the GC and thus fromReturnedValue is safe.
        return populateValueType(
            targetMetaType, target,
            QV4::Value::fromReturnedValue(QJSValuePrivate::asReturnedValue(val)),
            engine);
    }

    if (!isConstructibleMetaType(targetMetaType))
        return false;

    return createOrConstructValueType(
            QQmlMetaType::qmlType(targetMetaType), sourceMetaType, source,
            [targetMetaType, target]() {
                targetMetaType.destruct(target);
                return target;
            }, [target]() {
                return target;
            }, engine);
}

/*!
 * \internal
 * Populate the value type in place at \a target, which is expected to be
 * allocated and default-constructed, for example the result of a QVariant(QMetaType).
 * This is efficient if we can do byProperties() since it can use the pre-constructed object.
 * It also avoids the creation of a QVariant in most cases. It is not
 * efficient if you're going to create a QVariant anyway.
 */
bool QQmlValueTypeProvider::populateValueType(
        QMetaType targetMetaType, void *target, const QV4::Value &source,
        QV4::ExecutionEngine *engine)
{
    if (!isConstructibleMetaType(targetMetaType))
        return false;

    return createOrConstructValueType(
        QQmlMetaType::qmlType(targetMetaType), source, [targetMetaType, target]() {
            targetMetaType.destruct(target);
            return target;
        }, [target]() {
            return target;
        }, engine);
}

/*!
 * \internal
 * Specialization that constructs the value type on the heap using new and returns a pointer to it.
 */
void *QQmlValueTypeProvider::heapCreateValueType(
        const QQmlType &targetType, const QV4::Value &source, QV4::ExecutionEngine *engine)
{
    void *target = nullptr;
    if (createOrConstructValueType(
        targetType, source, [&]() {
            const QMetaType metaType = targetType.typeId();
            const ushort align = metaType.alignOf();
            target = align > __STDCPP_DEFAULT_NEW_ALIGNMENT__
                     ? operator new(metaType.sizeOf(), std::align_val_t(align))
                     : operator new(metaType.sizeOf());
            return target;
        }, [&]() {
            target = targetType.typeId().create();
            return target;
        }, engine)) {
        Q_ASSERT(target != nullptr);
    }

    return target;
}

QVariant QQmlValueTypeProvider::constructValueType(
        QMetaType targetMetaType, const QMetaObject *targetMetaObject,
        int ctorIndex, void *ctorArg)
{
    QVariant result;
    fromVerifiedType(targetMetaObject, ctorIndex, ctorArg,
                     [&]() { return createVariantData(targetMetaType, &result); });
    return result;
}

static QVariant fromJSValue(const QQmlType &type, const QJSValue &s, QMetaType metaType)
{
    if (const auto valueTypeFunction = type.createValueTypeFunction()) {
        const QVariant result = valueTypeFunction(s);
        if (result.metaType() == metaType)
            return result;
    }

    return QVariant();
}

QVariant QQmlValueTypeProvider::createValueType(const QJSValue &s, QMetaType metaType)
{
    if (!isConstructibleMetaType(metaType))
        return QVariant();
    return fromJSValue(QQmlMetaType::qmlType(metaType), s, metaType);
}

QVariant QQmlValueTypeProvider::createValueType(const QString &s, QMetaType metaType)
{
    if (!isConstructibleMetaType(metaType))
        return QVariant();
    const QQmlType type = QQmlMetaType::qmlType(metaType);
    if (type.canConstructValueType()) {
        if (const QMetaObject *mo = type.metaObjectForValueType()) {
            QVariant result;
            if (fromString(mo, s, [&]() { return createVariantData(metaType, &result); }))
                return result;
        }
    }

    return fromJSValue(type, s, metaType);
}

QVariant QQmlValueTypeProvider::createValueType(
        const QV4::Value &s, QMetaType metaType, QV4::ExecutionEngine *engine)
{
    if (!isConstructibleMetaType(metaType))
        return QVariant();
    const QQmlType type = QQmlMetaType::qmlType(metaType);
    const auto warn = [&](const QMetaObject *mo) {
        qWarning().noquote()
                << "Could not find any constructor for value type"
                << mo->className() << "to call with value" << s.toQStringNoThrow();
    };

    if (type.canPopulateValueType()) {
        if (const QMetaObject *mo = type.metaObject()) {
            QVariant result = byProperties(mo, metaType, s, engine);
            if (result.isValid())
                return result;
            if (type.canConstructValueType()) {
                if (fromMatchingType(
                            mo, s, [&]() { return createVariantData(metaType, &result); },
                            engine)) {
                    return result;
                }
                warn(mo);
            }
        }
    } else if (type.canConstructValueType()) {
        if (const QMetaObject *mo = type.metaObject()) {
            QVariant result;
            if (fromMatchingType(
                        mo, s, [&]() { return createVariantData(metaType, &result); },
                        engine)) {
                return result;
            }
            warn(mo);
        }
    }

    return fromJSValue(type, QJSValuePrivate::fromReturnedValue(s.asReturnedValue()), metaType);

}

/*!
 * \internal
 * This should only be called with either builtin types or wrapped QJSValues as source.
 */
QVariant QQmlValueTypeProvider::createValueType(
        const QVariant &s, QMetaType metaType, QV4::ExecutionEngine *engine)
{
    if (!isConstructibleMetaType(metaType))
        return QVariant();
    const QQmlType type = QQmlMetaType::qmlType(metaType);
    const auto warn = [&](const QMetaObject *mo) {
        qWarning().noquote()
                << "Could not find any constructor for value type"
                << mo->className() << "to call with value" << s;
    };

    if (type.canPopulateValueType()) {
        if (const QMetaObject *mo = type.metaObjectForValueType()) {
            QVariant result = byProperties(mo, metaType, s, engine);
            if (result.isValid())
                return result;
            if (type.canConstructValueType()) {
                if (fromMatchingType(
                            mo, s, [&]() { return createVariantData(metaType, &result); },
                            engine)) {
                    return result;
                }
                warn(mo);
            }
        }
    } else if (type.canConstructValueType()) {
        if (const QMetaObject *mo = type.metaObjectForValueType()) {
            QVariant result;
            if (fromMatchingType(
                        mo, s, [&]() { return createVariantData(metaType, &result); },
                        engine)) {
                return result;
            }
            warn(mo);
        }
    }

    return QVariant();
}

QQmlColorProvider::~QQmlColorProvider() {}
QVariant QQmlColorProvider::colorFromString(const QString &, bool *ok) { if (ok) *ok = false; return QVariant(); }
unsigned QQmlColorProvider::rgbaFromString(const QString &, bool *ok) { if (ok) *ok = false; return 0; }
QVariant QQmlColorProvider::fromRgbF(double, double, double, double) { return QVariant(); }
QVariant QQmlColorProvider::fromHslF(double, double, double, double) { return QVariant(); }
QVariant QQmlColorProvider::fromHsvF(double, double, double, double) { return QVariant(); }
QVariant QQmlColorProvider::lighter(const QVariant &, qreal) { return QVariant(); }
QVariant QQmlColorProvider::darker(const QVariant &, qreal) { return QVariant(); }
QVariant QQmlColorProvider::alpha(const QVariant &, qreal)
{
    return QVariant();
}
QVariant QQmlColorProvider::tint(const QVariant &, const QVariant &) { return QVariant(); }

static QQmlColorProvider *colorProvider = nullptr;

Q_QML_EXPORT QQmlColorProvider *QQml_setColorProvider(QQmlColorProvider *newProvider)
{
    QQmlColorProvider *old = colorProvider;
    colorProvider = newProvider;
    return old;
}

static QQmlColorProvider **getColorProvider(void)
{
    if (colorProvider == nullptr) {
        qWarning() << "Warning: QQml_colorProvider: no color provider has been set!";
        static QQmlColorProvider nullColorProvider;
        colorProvider = &nullColorProvider;
    }

    return &colorProvider;
}

Q_AUTOTEST_EXPORT QQmlColorProvider *QQml_colorProvider(void)
{
    static QQmlColorProvider **providerPtr = getColorProvider();
    return *providerPtr;
}


QQmlGuiProvider::~QQmlGuiProvider() {}
QQmlApplication *QQmlGuiProvider::application(QObject *parent)
{
    return new QQmlApplication(parent);
}
QStringList QQmlGuiProvider::fontFamilies() { return QStringList(); }
bool QQmlGuiProvider::openUrlExternally(const QUrl &) { return false; }

QObject *QQmlGuiProvider::inputMethod()
{
    // We don't have any input method code by default
    QObject *o = new QObject();
    o->setObjectName(QStringLiteral("No inputMethod available"));
    QQmlEngine::setObjectOwnership(o, QQmlEngine::JavaScriptOwnership);
    return o;
}

QObject *QQmlGuiProvider::styleHints()
{
    QObject *o = new QObject();
    o->setObjectName(QStringLiteral("No styleHints available"));
    QQmlEngine::setObjectOwnership(o, QQmlEngine::JavaScriptOwnership);
    return o;
}

QString QQmlGuiProvider::pluginName() const { return QString(); }

static QQmlGuiProvider *guiProvider = nullptr;

Q_QML_EXPORT QQmlGuiProvider *QQml_setGuiProvider(QQmlGuiProvider *newProvider)
{
    QQmlGuiProvider *old = guiProvider;
    guiProvider = newProvider;
    return old;
}

static QQmlGuiProvider **getGuiProvider(void)
{
    if (guiProvider == nullptr) {
        static QQmlGuiProvider nullGuiProvider; //Still provides an application with no GUI support
        guiProvider = &nullGuiProvider;
    }

    return &guiProvider;
}

Q_AUTOTEST_EXPORT QQmlGuiProvider *QQml_guiProvider(void)
{
    static QQmlGuiProvider **providerPtr = getGuiProvider();
    return *providerPtr;
}

//Docs in qqmlengine.cpp
QQmlApplication::QQmlApplication(QObject *parent)
    : QQmlApplication(*(new QQmlApplicationPrivate), parent)
{
}

QQmlApplication::QQmlApplication(QQmlApplicationPrivate &dd, QObject *parent)
    : QObject(dd, parent)
{
    connect(QCoreApplication::instance(), SIGNAL(aboutToQuit()),
            this, SIGNAL(aboutToQuit()));
    connect(QCoreApplication::instance(), SIGNAL(applicationNameChanged()),
            this, SIGNAL(nameChanged()));
    connect(QCoreApplication::instance(), SIGNAL(applicationVersionChanged()),
            this, SIGNAL(versionChanged()));
    connect(QCoreApplication::instance(), SIGNAL(organizationNameChanged()),
            this, SIGNAL(organizationChanged()));
    connect(QCoreApplication::instance(), SIGNAL(organizationDomainChanged()),
            this, SIGNAL(domainChanged()));
}

QStringList QQmlApplication::args()
{
    Q_D(QQmlApplication);
    if (!d->argsInit) {
        d->argsInit = true;
        d->args = QCoreApplication::arguments();
    }
    return d->args;
}

QString QQmlApplication::name() const
{
    return QCoreApplication::instance()->applicationName();
}

QString QQmlApplication::version() const
{
    return QCoreApplication::instance()->applicationVersion();
}

QString QQmlApplication::organization() const
{
    return QCoreApplication::instance()->organizationName();
}

QString QQmlApplication::domain() const
{
    return QCoreApplication::instance()->organizationDomain();
}

void QQmlApplication::setName(const QString &arg)
{
    QCoreApplication::instance()->setApplicationName(arg);
}

void QQmlApplication::setVersion(const QString &arg)
{
    QCoreApplication::instance()->setApplicationVersion(arg);
}

void QQmlApplication::setOrganization(const QString &arg)
{
    QCoreApplication::instance()->setOrganizationName(arg);
}

void QQmlApplication::setDomain(const QString &arg)
{
    QCoreApplication::instance()->setOrganizationDomain(arg);
}

static const QQmlData *ddata_for_cast(QObject *object)
{
    Q_ASSERT(object);
    auto ddata = QQmlData::get(object, false);
    return (ddata && ddata->propertyCache) ? ddata : nullptr;
}

bool qmlobject_can_cpp_cast(QObject *object, const QMetaObject *mo)
{
    Q_ASSERT(mo);
    if (const QQmlData *ddata = ddata_for_cast(object))
        return ddata->propertyCache->firstCppMetaObject()->inherits(mo);
    return object->metaObject()->inherits(mo);
}

bool qmlobject_can_qml_cast(QObject *object, const QQmlType &type)
{
    Q_ASSERT(type.isValid());

    // A non-composite type will always have a metaobject.
    const QMetaObject *typeMetaObject = type.metaObject();
    const QQmlPropertyCache::ConstPtr typePropertyCache = typeMetaObject
            ? QQmlPropertyCache::ConstPtr()
            : QQmlMetaType::findPropertyCacheInCompositeTypes(type.typeId());

    if (const QQmlData *ddata = ddata_for_cast(object)) {
        for (const QQmlPropertyCache *propertyCache = ddata->propertyCache.data(); propertyCache;
             propertyCache = propertyCache->parent().data()) {

            if (typeMetaObject) {
                // Prefer the metaobject inheritance mechanism, since it is more accurate.
                //
                // Assume the object can be casted to the type. Then, if we have a type metaobject,
                // the object's property cache inheritance has to contain it. Otherwise we would
                // end up with diverging metaobject hierarchies if we created the object's
                // metaobject. This would be a disaster.
                if (const QMetaObject *objectMetaObject = propertyCache->metaObject())
                    return objectMetaObject->inherits(typeMetaObject);
            } else {
                // This is a best effort attempt. There are a number of ways for the
                // property caches to be unrelated but the types still convertible.
                // Multiple property caches can hold the same metaobject, for example for
                // versions of non-composite types.
                if (propertyCache == typePropertyCache.data())
                    return true;
            }
        }
    }

    // If nothing else works, we have to create the metaobjects.

    return object->metaObject()->inherits(typeMetaObject
            ? typeMetaObject
            : (typePropertyCache ? typePropertyCache->createMetaObject() : nullptr));
}

QT_END_NAMESPACE

#include "moc_qqmlglobal_p.cpp"
