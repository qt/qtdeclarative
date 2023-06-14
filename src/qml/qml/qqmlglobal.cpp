// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <private/qqmlglobal_p.h>
#include <QtQml/private/qqmlmetatype_p.h>
#include <QtQml/private/qjsvalue_p.h>

#include <QtQml/qqmlengine.h>
#include <QtCore/qvariant.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qdebug.h>
#include <QtCore/QCoreApplication>

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

static void callConstructor(
        const QMetaObject *targetMetaObject, int i, QMetaType targetMetaType, void *target,
        void *source)
{
    // Unfortunately CreateInstance unconditionally creates the instance on the heap.
    void *gadget = nullptr;
    void *p[] = { &gadget, source };
    targetMetaObject->static_metacall(QMetaObject::CreateInstance, i, p);
    Q_ASSERT(gadget);
    targetMetaType.destruct(target);
    targetMetaType.construct(target, gadget);
    targetMetaType.destroy(gadget);
}

template<typename Retrieve>
bool fromMatchingType(
    const QMetaObject *targetMetaObject, const QMetaType targetMetaType, void *target,
    Retrieve &&retrieve)
{
    for (int i = 0, end = targetMetaObject->constructorCount(); i < end; ++i) {
        const QMetaMethod ctor = targetMetaObject->constructor(i);
        if (ctor.parameterCount() != 1)
            continue;

        const QMetaType parameterType = ctor.parameterMetaType(0);
        if (retrieve(parameterType, [&](QMetaType sourceMetaType, void *sourceData) {
            if (sourceMetaType == parameterType) {
                callConstructor(targetMetaObject, i, targetMetaType, target, sourceData);
                return true;
            }
            if (const QMetaObject *parameterMetaObject = parameterType.metaObject()) {
                if (const QMetaObject *sourceMetaObject = sourceMetaType.metaObject();
                    sourceMetaObject && sourceMetaObject->inherits(parameterMetaObject)) {
                    // Allow construction from derived types.
                    callConstructor(targetMetaObject, i, targetMetaType, target, sourceData);
                    return true;
                }
            }

            // Do not recursively try to create parameters here.
            // This may end up in infinite recursion.

            // At this point, s should be a builtin type. For builtin types
            // the QMetaType converters are good enough.
            QVariant converted(parameterType);
            if (QMetaType::convert(sourceMetaType, sourceData, parameterType, converted.data())) {
                callConstructor(targetMetaObject, i, targetMetaType, target, converted.data());
                return true;
            }
            return false;
        })) {
            return true;
        }
    }

    return false;
}

static bool fromMatchingType(
    const QMetaObject *targetMetaObject, QMetaType targetMetaType, void *target,
    const QV4::Value &source)
{
    return fromMatchingType(
        targetMetaObject, targetMetaType, target,
        [&](QMetaType parameterType, auto callback) {
            QVariant variant = QV4::ExecutionEngine::toVariant(source, parameterType);
            return callback(variant.metaType(), variant.data());
        });
}

static bool fromString(
        const QMetaObject *targetMetaObject, const QMetaType targetMetaType, void *target,
        QString source)
{
    for (int i = 0, end = targetMetaObject->constructorCount(); i < end; ++i) {
        const QMetaMethod ctor = targetMetaObject->constructor(i);
        if (ctor.parameterCount() != 1)
            continue;

        if (ctor.parameterMetaType(0) == QMetaType::fromType<QString>()) {
            callConstructor(targetMetaObject, i, targetMetaType, target, &source);
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
                    QVariant result(propertyType);
                    return QQmlValueTypeProvider::createValueType(
                            v4PropValue, propertyType, result.data())
                        ? result
                        : QVariant();
                })) {
            continue;
        }
        const QMetaType propertyType = metaProperty.metaType();
        QVariant property = QV4::ExecutionEngine::toVariant(v4PropValue, propertyType);
        if (property.metaType() == propertyType) {
            metaProperty.writeOnGadget(target, std::move(property));
            continue;
        }

        QVariant converted(propertyType);
        if (QQmlValueTypeProvider::createValueType(v4PropValue, propertyType, converted.data())) {
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

static bool byProperties(
    const QMetaObject *targetMetaObject, void *target, const QV4::Value &source)
{
    if (!source.isObject() || !targetMetaObject)
        return false;

    doWriteProperties(targetMetaObject, target, source);
    return true;
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
        QVariant converted(propertyType);
        if (QQmlValueTypeProvider::createValueType(
                property.metaType(), property.data(), propertyType, converted.data())) {
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

static bool byProperties(const QMetaObject *targetMetaObject, void *target, QObject *source)
{
    if (!source || !targetMetaObject)
        return false;
    doWriteProperties(targetMetaObject, target, source);
    return true;
}

static bool byProperties(
    const QMetaObject *targetMetaObject, void *target,
    const QMetaObject *sourceMetaObject, const void *source)
{
    if (!source || !sourceMetaObject || !targetMetaObject)
        return false;
    doWriteProperties(
        targetMetaObject, target, sourceMetaObject,
        [source](const QMetaObject *sourceMetaObject, int sourceProperty) {
            return sourceMetaObject->property(sourceProperty).readOnGadget(source);
        });
    return true;
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
        QVariant converted(propertyType);
        if (QQmlValueTypeProvider::createValueType(
                property.metaType(), property.data(), propertyType, converted.data())) {
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
bool byProperties(
    const QMetaObject *targetMetaObject, void *target, const Map &source)
{
    doWriteProperties(targetMetaObject, target, source);
    return true;
}

static bool byProperties(
    const QMetaObject *targetMetaObject, void *target,
    QMetaType sourceMetaType, const void *source)
{
    if (!targetMetaObject)
        return false;

    if (sourceMetaType == QMetaType::fromType<QJSValue>()) {
        const QJSValue *val = static_cast<const QJSValue *>(source);
        return byProperties(
            targetMetaObject, target, QV4::Value(QJSValuePrivate::asReturnedValue(val)));
    }

    if (sourceMetaType == QMetaType::fromType<QVariantMap>()) {
        return byProperties(
            targetMetaObject, target,
            *static_cast<const QVariantMap *>(source));
    }
    if (sourceMetaType == QMetaType::fromType<QVariantHash>()) {
        return byProperties(
            targetMetaObject, target,
            *static_cast<const QVariantHash *>(source));
    }
    if (sourceMetaType.flags() & QMetaType::PointerToQObject)
        return byProperties(targetMetaObject, target, *static_cast<QObject *const *>(source));
    if (const QMetaObject *sourceMeta = QQmlMetaType::metaObjectForValueType(sourceMetaType))
        return byProperties(targetMetaObject, target, sourceMeta, source);

    return false;
}

static bool fromJSValue(
        const QQmlType &targetType, QMetaType targetMetaType, void *target, const QJSValue &source)
{
    if (const auto valueTypeFunction = targetType.createValueTypeFunction()) {
        QVariant result = valueTypeFunction(source);
        if (result.metaType() == targetMetaType) {
            targetMetaType.destruct(target);
            targetMetaType.construct(target, result.constData());
            return true;
        }
    }

    return false;
}

bool QQmlValueTypeProvider::constructFromJSValue(
        const QJSValue &source, QMetaType targetMetaType, void *target)
{
    return isConstructibleMetaType(targetMetaType)
            && fromJSValue(QQmlMetaType::qmlType(targetMetaType), targetMetaType, target, source);
}

bool QQmlValueTypeProvider::createValueType(
        const QString &source, QMetaType targetMetaType, void *target)
{
    if (!isConstructibleMetaType(targetMetaType))
        return false;
    const QQmlType targetType = QQmlMetaType::qmlType(targetMetaType);
    const QMetaObject *targetMetaObject = QQmlMetaType::metaObjectForValueType(targetType);
    if (targetMetaObject && targetType.canConstructValueType()) {
        if (fromString(targetMetaObject, targetMetaType, target, source))
            return true;
    }

    return fromJSValue(targetType, targetMetaType, target, source);
}

bool QQmlValueTypeProvider::createValueType(
        const QJSValue &source, QMetaType targetMetaType, void *target)
{
    if (!isConstructibleMetaType(targetMetaType))
        return false;
    const QQmlType targetType = QQmlMetaType::qmlType(targetMetaType);
    if (const QMetaObject *targetMetaObject = QQmlMetaType::metaObjectForValueType(targetType)) {
        if (targetType.canPopulateValueType()
                    && byProperties(
                        targetMetaObject, target,
                        QV4::Value(QJSValuePrivate::asReturnedValue(&source)))) {
                return true;
        }

        if (targetType.canConstructValueType()
                && fromMatchingType(
                    targetMetaObject, targetMetaType, target,
                    [&](QMetaType parameterType, auto callback) {
                        QVariant variant = QV4::ExecutionEngine::toVariant(
                            QJSValuePrivate::asReturnedValue(&source), parameterType);
                        return callback(variant.metaType(), variant.data());
                    })) {
            return true;
        }
    }

    return fromJSValue(targetType, targetMetaType, target, source);
}

bool QQmlValueTypeProvider::createValueType(
        const QV4::Value &source, QMetaType targetMetaType, void *target)
{
    if (!isConstructibleMetaType(targetMetaType))
        return false;
    const QQmlType targetType = QQmlMetaType::qmlType(targetMetaType);
    if (const QMetaObject *targetMetaObject = QQmlMetaType::metaObjectForValueType(targetType)) {
        if (targetType.canPopulateValueType() && byProperties(targetMetaObject, target, source))
            return true;
        if (targetType.canConstructValueType()) {
            if (fromMatchingType(targetMetaObject, targetMetaType, target, source))
                return true;
            qWarning().noquote()
                    << "Could not find any constructor for value type"
                    << targetMetaObject->className() << "to call with value"
                    << source.toQStringNoThrow();
        }
    }

    return fromJSValue(
        targetType, targetMetaType, target,
        QJSValuePrivate::fromReturnedValue(source.asReturnedValue()));

}

bool QQmlValueTypeProvider::createValueType(
    const QVariant &source, QMetaType targetMetaType, void *target)
{
    if (!isConstructibleMetaType(targetMetaType))
        return false;
    const QQmlType targetType = QQmlMetaType::qmlType(targetMetaType);
    if (const QMetaObject *targetMetaObject = QQmlMetaType::metaObjectForValueType(targetType)) {
        if (targetType.canPopulateValueType()
                && byProperties(targetMetaObject, target, source.metaType(), source.data())) {
            return true;
        }

        if (targetType.canConstructValueType()) {
            if (fromMatchingType(targetMetaObject, targetMetaType, target,
                                 [&](QMetaType, auto callback) {
                QVariant nonConstSource = source;
                return callback(nonConstSource.metaType(), nonConstSource.data());
            })) {
                return true;
            }

            qWarning().noquote()
                << "Could not find any constructor for value type"
                << targetMetaObject->className() << "to call with value" << source;
        }
    }

    return false;
}

bool QQmlValueTypeProvider::createValueType(
    QMetaType sourceMetaType, void *source, QMetaType targetMetaType, void *target)
{
    if (!isConstructibleMetaType(targetMetaType))
        return false;
    const QQmlType targetType = QQmlMetaType::qmlType(targetMetaType);
    if (const QMetaObject *targetMetaObject = QQmlMetaType::metaObjectForValueType(targetType)) {
        if (targetType.canPopulateValueType()
                && byProperties(targetMetaObject, target, sourceMetaType, source)) {
            return true;
        }

        if (targetType.canConstructValueType()) {
            if (fromMatchingType(targetMetaObject, targetMetaType, target,
                                 [&](QMetaType, auto callback) {
                    return callback(sourceMetaType, source);
                })) {
                return true;
            }

            qWarning().noquote()
                << "Could not find any constructor for value type"
                << targetMetaObject->className() << "to call with value"
                << QVariant(sourceMetaType, source);
        }
    }

    return false;
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

QT_END_NAMESPACE

#include "moc_qqmlglobal_p.cpp"
