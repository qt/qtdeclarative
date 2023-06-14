// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtQml/private/qjsvalue_p.h>
#include <QtQml/private/qqmlglobal_p.h>
#include <QtQml/private/qqmlmetatype_p.h>
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
        const QMetaObject *targetMetaObject, Allocate &&allocate, Retrieve &&retrieve)
{
    for (int i = 0, end = targetMetaObject->constructorCount(); i < end; ++i) {
        const QMetaMethod ctor = targetMetaObject->constructor(i);
        if (ctor.parameterCount() != 1)
            continue;

        const QMetaType parameterType = ctor.parameterMetaType(0);

        if (retrieve(parameterType, [&](QMetaType sourceMetaType, void *sourceData) {
            if (sourceMetaType == parameterType) {
                callConstructor(targetMetaObject, i, sourceData, allocate());
                return true;
            }

            if (const QMetaObject *parameterMetaObject = parameterType.metaObject()) {
                if (const QMetaObject *sourceMetaObject = sourceMetaType.metaObject();
                        sourceMetaObject && sourceMetaObject->inherits(parameterMetaObject)) {
                    // Allow construction from derived types.
                    callConstructor(targetMetaObject, i, sourceData, allocate());
                    return true;
                }
            }

            // Do not recursively try to create parameters here. This may end up in infinite recursion.

            // At this point, s should be a builtin type. For builtin types
            // the QMetaType converters are good enough.
            QVariant converted(parameterType);
            if (QMetaType::convert(sourceMetaType, sourceData, parameterType, converted.data())) {
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
    const QMetaObject *targetMetaObject, const QV4::Value &source, Allocate &&allocate)
{
    return fromMatchingType(
            targetMetaObject, std::forward<Allocate>(allocate),
            [&](QMetaType parameterType, auto callback) {
        QVariant variant = QV4::ExecutionEngine::toVariant(source, parameterType);
        return callback(variant.metaType(), variant.data());
    });
}

template<typename Allocate>
static bool fromMatchingType(
    const QMetaObject *targetMetaObject, QVariant source, Allocate &&allocate)
{
    return fromMatchingType(targetMetaObject, std::forward<Allocate>(allocate),
                            [&](QMetaType, auto callback) {
        return callback(source.metaType(), source.data());
    });
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
static bool doWriteProperty(const QMetaProperty &metaProperty, void *target,
                            Get &&get, Convert &&convert)
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
    if (QMetaType::convert(property.metaType(), property.constData(),
                           propertyType, converted.data())) {
        metaProperty.writeOnGadget(target, std::move(converted));
        return true;
    }

    return false;
}

static void doWriteProperties(
    const QMetaObject *targetMetaObject, void *target, const QV4::Value &source)
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
            return QQmlValueTypeProvider::createValueType(v4PropValue, propertyType);
        })) {
            continue;
        }

        const QMetaType propertyType = metaProperty.metaType();
        QVariant property = QV4::ExecutionEngine::toVariant(v4PropValue, propertyType);
        if (property.metaType() == propertyType) {
            metaProperty.writeOnGadget(target, std::move(property));
            continue;
        }

        QVariant converted = QQmlValueTypeProvider::createValueType(v4PropValue, propertyType);
        if (converted.isValid()) {
            metaProperty.writeOnGadget(target, std::move(converted));
            continue;
        }

        converted = QVariant(propertyType);
        if (QMetaType::convert(property.metaType(), property.constData(),
                               propertyType, converted.data())) {
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
    const QMetaObject *targetMetaObject, QMetaType metaType, const QV4::Value &source)
{
    if (!source.isObject() || !targetMetaObject)
        return QVariant();

    QVariant result(metaType);
    doWriteProperties(targetMetaObject, result.data(), source);
    return result;
}

template<typename Read>
static void doWriteProperties(
    const QMetaObject *targetMetaObject, void *target,
    const QMetaObject *sourceMetaObject, Read &&read)
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

        QVariant converted = QQmlValueTypeProvider::createValueType(property, propertyType);
        if (converted.isValid()) {
            metaProperty.writeOnGadget(target, std::move(converted));
            continue;
        }

        converted = QVariant(propertyType);
        if (QMetaType::convert(property.metaType(), property.constData(),
                               propertyType, converted.data())) {
            metaProperty.writeOnGadget(target, std::move(converted));
            continue;
        }

        qWarning().noquote()
            << QLatin1String("Could not convert %1 to %2 for property %3")
                   .arg(property.toString(), QString::fromUtf8(propertyType.name()),
                        QString::fromUtf8(metaProperty.name()));
    }
}


static void doWriteProperties(const QMetaObject *targetMeta, void *target, QObject *source)
{
    doWriteProperties(
            targetMeta, target, source->metaObject(),
            [source](const QMetaObject *sourceMetaObject, int sourceProperty) {
        return sourceMetaObject->property(sourceProperty).read(source);
    });
}

static QVariant byProperties(
    const QMetaObject *targetMetaObject, QMetaType targetMetaType, QObject *source)
{
    if (!source || !targetMetaObject)
        return QVariant();

    QVariant result(targetMetaType);
    doWriteProperties(targetMetaObject, result.data(), source);
    return result;
}

static QVariant byProperties(
    const QMetaObject *targetMetaObject, QMetaType targetMetaType,
    const QMetaObject *sourceMetaObject, const void *source)
{
    if (!source || !sourceMetaObject || !targetMetaObject)
        return QVariant();

    QVariant result(targetMetaType);
    doWriteProperties(
            targetMetaObject, result.data(), sourceMetaObject,
            [source](const QMetaObject *sourceMetaObject, int sourceProperty) {
        return sourceMetaObject->property(sourceProperty).readOnGadget(source);
    });
    return result;
}

template<typename Map>
void doWriteProperties(const QMetaObject *targetMetaObject, void *target, const Map &source)
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

        QVariant converted = QQmlValueTypeProvider::createValueType(property, propertyType);
        if (converted.isValid()) {
            metaProperty.writeOnGadget(target, std::move(converted));
            continue;
        }

        converted = QVariant(propertyType);
        if (QMetaType::convert(property.metaType(), property.constData(),
                               propertyType, converted.data())) {
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
    const QMetaObject *targetMetaObject, QMetaType targetMetaType, const Map &source)
{
    QVariant result(targetMetaType);
    doWriteProperties(targetMetaObject, result.data(), source);
    return result;
}

static QVariant byProperties(
    const QMetaObject *targetMetaObject, QMetaType targetMetaType, const QVariant &source)
{
    if (!targetMetaObject)
        return QVariant();

    if (source.metaType() == QMetaType::fromType<QJSValue>()) {
        QJSValue val = source.value<QJSValue>();
        return byProperties(
            targetMetaObject, targetMetaType, QV4::Value(QJSValuePrivate::asReturnedValue(&val)));
    }

    if (source.metaType() == QMetaType::fromType<QVariantMap>()) {
        return byProperties(
            targetMetaObject, targetMetaType,
            *static_cast<const QVariantMap *>(source.constData()));
    }

    if (source.metaType() == QMetaType::fromType<QVariantHash>()) {
        return byProperties(
            targetMetaObject, targetMetaType,
            *static_cast<const QVariantHash *>(source.constData()));
    }

    if (source.metaType().flags() & QMetaType::PointerToQObject)
        return byProperties(targetMetaObject, targetMetaType, source.value<QObject *>());

    if (const QMetaObject *sourceMeta = QQmlMetaType::metaObjectForValueType(source.metaType()))
        return byProperties(targetMetaObject, targetMetaType, sourceMeta, source.constData());

    return QVariant();
}

/*!
 * \internal
 * Specialization that creates the value type in place at \a target, which is expected to be
 * already initialized. This is more efficient if we can do byProperties() since it can use a
 * pre-constructed object. It also avoids the creation of a QVariant in most cases. It is less
 * efficient if you're going to create a QVariant anyway.
 */
bool QQmlValueTypeProvider::createValueType(
    QMetaType targetMetaType, void *target, const QV4::Value &source)
{
    if (!isConstructibleMetaType(targetMetaType))
        return false;

    auto destruct = [targetMetaType, target]() {
        targetMetaType.destruct(target);
        return target;
    };

    const QQmlType type = QQmlMetaType::qmlType(targetMetaType);
    if (const QMetaObject *targetMeta = QQmlMetaType::metaObjectForValueType(type)) {
        const auto warn = [&]() {
            qWarning().noquote()
                    << "Could not find any constructor for value type"
                    << targetMeta->className() << "to call with value" << source.toQStringNoThrow();
        };

        if (type.canPopulateValueType()) {
            if (source.isObject() && targetMeta) {
                doWriteProperties(targetMeta, target, source);
                return true;
            }
            if (type.canConstructValueType()) {
                if (fromMatchingType(targetMeta, source, destruct))
                    return true;
                warn();

            }
        } else if (type.canConstructValueType()) {
            if (fromMatchingType(targetMeta, source, destruct))
                return true;
            warn();
        }
    }

    if (const auto valueTypeFunction = type.createValueTypeFunction()) {
        const QVariant result
                = valueTypeFunction(QJSValuePrivate::fromReturnedValue(source.asReturnedValue()));
        if (result.metaType() == targetMetaType) {
            targetMetaType.construct(destruct(), result.constData());
            return true;
        }
    }

    return false;
}

bool QQmlValueTypeProvider::createValueType(
    QMetaType targetMetaType, void *target, QMetaType sourceMetaType, void *source)
{
    if (sourceMetaType == QMetaType::fromType<QJSValue>()) {
        const QJSValue *val = static_cast<const QJSValue *>(source);
        return createValueType(
            targetMetaType, target, QV4::Value(QJSValuePrivate::asReturnedValue(val)));
    }

    if (!isConstructibleMetaType(targetMetaType))
        return false;

    auto destruct = [targetMetaType, target]() {
        targetMetaType.destruct(target);
        return target;
    };

    const QQmlType type = QQmlMetaType::qmlType(targetMetaType);
    if (const QMetaObject *targetMetaObject = QQmlMetaType::metaObjectForValueType(type)) {
        const auto warn = [&]() {
            qWarning().noquote()
                << "Could not find any constructor for value type"
                << targetMetaObject->className() << "to call with value" << source;
        };

        if (type.canPopulateValueType()) {

            if (const QMetaObject *sourceMetaObject
                    = QQmlMetaType::metaObjectForValueType(sourceMetaType)) {
                doWriteProperties(
                    targetMetaObject, target, sourceMetaObject,
                    [&source](const QMetaObject *sourceMetaObject, int sourceProperty) {
                        return sourceMetaObject->property(sourceProperty).readOnGadget(source);
                    });
                return true;
            }

            if (sourceMetaType == QMetaType::fromType<QVariantMap>()) {
                doWriteProperties(
                    targetMetaObject, target, *static_cast<const QVariantMap *>(source));
                return true;
            }

            if (sourceMetaType == QMetaType::fromType<QVariantHash>()) {
                doWriteProperties(
                    targetMetaObject, target, *static_cast<const QVariantHash *>(source));
                return true;
            }

            if (sourceMetaType.flags() & QMetaType::PointerToQObject) {
                doWriteProperties(targetMetaObject, target, *static_cast<QObject *const *>(source));
                return true;
            }
        }

        if (type.canConstructValueType()) {
            if (fromMatchingType(targetMetaObject, destruct, [&](QMetaType, auto callback) {
                return callback(sourceMetaType, source);
            })) {
                return true;
            }
            warn();
        }
    }

    return false;
}

QVariant QQmlValueTypeProvider::constructValueType(
        QMetaType resultMetaType, const QMetaObject *resultMetaObject,
        int ctorIndex, void *ctorArg)
{
    QVariant result;
    fromVerifiedType(resultMetaObject, ctorIndex, ctorArg,
                     [&]() { return createVariantData(resultMetaType, &result); });
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
    const QMetaObject *mo = QQmlMetaType::metaObjectForValueType(type);
    if (mo && type.canConstructValueType()) {
        QVariant result;
        if (fromString(mo, s, [&]() { return createVariantData(metaType, &result); }))
            return result;
    }

    return fromJSValue(type, s, metaType);
}

QVariant QQmlValueTypeProvider::createValueType(const QV4::Value &s, QMetaType metaType)
{
    if (!isConstructibleMetaType(metaType))
        return QVariant();
    const QQmlType type = QQmlMetaType::qmlType(metaType);
    if (const QMetaObject *mo = QQmlMetaType::metaObjectForValueType(type)) {
        const auto warn = [&]() {
            qWarning().noquote()
                    << "Could not find any constructor for value type"
                    << mo->className() << "to call with value" << s.toQStringNoThrow();
        };

        if (type.canPopulateValueType()) {
            QVariant result = byProperties(mo, metaType, s);
            if (result.isValid())
                return result;
            if (type.canConstructValueType()) {
                if (fromMatchingType(mo, s, [&]() { return createVariantData(metaType, &result); }))
                    return result;
                warn();
            }
        } else if (type.canConstructValueType()) {
            QVariant result;
            if (fromMatchingType(mo, s, [&]() { return createVariantData(metaType, &result); }))
                return result;
            warn();
        }
    }

    return fromJSValue(type, QJSValuePrivate::fromReturnedValue(s.asReturnedValue()), metaType);

}

/*!
 * \internal
 * This should only be called with either builtin types or wrapped QJSValues as source.
 */
QVariant QQmlValueTypeProvider::createValueType(const QVariant &s, QMetaType metaType)
{
    if (!isConstructibleMetaType(metaType))
        return QVariant();
    const QQmlType type = QQmlMetaType::qmlType(metaType);
    if (const QMetaObject *mo = QQmlMetaType::metaObjectForValueType(type)) {
        const auto warn = [&]() {
            qWarning().noquote()
                    << "Could not find any constructor for value type"
                    << mo->className() << "to call with value" << s;
        };

        if (type.canPopulateValueType()) {
            QVariant result = byProperties(mo, metaType, s);
            if (result.isValid())
                return result;
            if (type.canConstructValueType()) {
                if (fromMatchingType(mo, s, [&]() { return createVariantData(metaType, &result); }))
                    return result;
                warn();
            }
        } else if (type.canConstructValueType()) {
            QVariant result;
            if (fromMatchingType(mo, s, [&]() { return createVariantData(metaType, &result); }))
                return result;
            warn();
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

Q_QML_PRIVATE_EXPORT QQmlColorProvider *QQml_setColorProvider(QQmlColorProvider *newProvider)
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

Q_QML_PRIVATE_EXPORT QQmlGuiProvider *QQml_setGuiProvider(QQmlGuiProvider *newProvider)
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
    : QObject(*(new QQmlApplicationPrivate),parent)
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

bool qmlobject_can_cast(QObject *object, const QMetaObject *mo)
{
    Q_ASSERT(object);
    Q_ASSERT(mo);
    auto ddata = QQmlData::get(object, false);
    if (!ddata || ! ddata->propertyCache)
        return object->metaObject()->inherits(mo);
   return ddata->propertyCache->firstCppMetaObject()->inherits(mo);
}

QT_END_NAMESPACE

#include "moc_qqmlglobal_p.cpp"
