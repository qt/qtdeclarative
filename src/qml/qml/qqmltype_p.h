// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLTYPE_P_H
#define QQMLTYPE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <functional>

#include <private/qqmlrefcount_p.h>
#include <private/qtqmlglobal_p.h>
#include <private/qv4value_p.h>

#include <QtQml/qqmlprivate.h>
#include <QtQml/qjsvalue.h>

#include <QtCore/qloggingcategory.h>
#include <QtCore/qobject.h>
#include <QtCore/qtyperevision.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcQmlEnumValue);

class QHashedCStringRef;
class QHashedString;
class QHashedStringRef;
class QQmlCustomParser;
class QQmlEnginePrivate;
class QQmlPropertyCache;
class QQmlTypeLoader;
class QQmlTypePrivate;

namespace QV4 {
struct String;
}

class Q_QML_EXPORT QQmlType
{
public:
    QQmlType();
    QQmlType(const QQmlType &other);
    QQmlType(QQmlType &&other);
    QQmlType &operator =(const QQmlType &other);
    QQmlType &operator =(QQmlType &&other);
    explicit QQmlType(const QQmlTypePrivate *priv);
    ~QQmlType();

    bool isValid() const { return !d.isNull(); }

    QByteArray typeName() const;
    QString qmlTypeName() const;
    QString elementName() const;

    QHashedString module() const;
    QTypeRevision version() const;

    bool availableInVersion(QTypeRevision version) const;
    bool availableInVersion(const QHashedStringRef &module, QTypeRevision version) const;

    typedef QVariant (*CreateValueTypeFunc)(const QJSValue &);
    CreateValueTypeFunc createValueTypeFunction() const;

    bool canConstructValueType() const;
    bool canPopulateValueType() const;

    QObject *create() const;
    QObject *create(void **, size_t) const;
    QObject *createWithQQmlData() const;

    typedef void (*CreateFunc)(void *, void *);
    CreateFunc createFunction() const;

    QQmlCustomParser *customParser() const;

    bool isCreatable() const;
    typedef QObject *(*ExtensionFunc)(QObject *);
    ExtensionFunc extensionFunction() const;
    const QMetaObject *extensionMetaObject() const;
    bool isExtendedType() const;
    QString noCreationReason() const;

    bool isSingleton() const;
    bool isInterface() const;
    bool isComposite() const;
    bool isCompositeSingleton() const;
    bool isQObjectSingleton() const;
    bool isQJSValueSingleton() const;
    bool isSequentialContainer() const;
    bool isValueType() const;

    QMetaType typeId() const;
    QMetaType qListTypeId() const;
    QMetaSequence listMetaSequence() const;

    const QMetaObject *metaObject() const;

    // Precondition: The type is actually a value type!
    const QMetaObject *metaObjectForValueType() const;

    const QMetaObject *baseMetaObject() const;
    QTypeRevision metaObjectRevision() const;
    bool containsRevisionedAttributes() const;

    QQmlAttachedPropertiesFunc attachedPropertiesFunction(QQmlTypeLoader *typeLoader) const;
    const QMetaObject *attachedPropertiesType(QQmlTypeLoader *typeLoader) const;

    int parserStatusCast() const;
    const char *interfaceIId() const;
    int propertyValueSourceCast() const;
    int propertyValueInterceptorCast() const;
    int finalizerCast() const;

    int index() const;

    bool isInlineComponentType() const;

    struct Q_QML_EXPORT SingletonInstanceInfo final
        : public QQmlRefCounted<SingletonInstanceInfo>
    {
        using Ptr = QQmlRefPointer<SingletonInstanceInfo>;
        using ConstPtr = QQmlRefPointer<const SingletonInstanceInfo>;

        static Ptr create() { return Ptr(new SingletonInstanceInfo, Ptr::Adopt); }

        std::function<QJSValue(QQmlEngine *, QJSEngine *)> scriptCallback = {};
        std::function<QObject *(QQmlEngine *, QJSEngine *)> qobjectCallback = {};
        QByteArray typeName;
        QUrl url; // used by composite singletons

    private:
        Q_DISABLE_COPY_MOVE(SingletonInstanceInfo)
        SingletonInstanceInfo() = default;
    };
    SingletonInstanceInfo::ConstPtr singletonInstanceInfo() const;

    QUrl sourceUrl() const;

    QV4::ReturnedValue enumValue(
            QQmlTypeLoader *typeLoader, const QHashedStringRef &, bool *ok) const;
    QV4::ReturnedValue enumValue(
            QQmlTypeLoader *typeLoader, const QHashedCStringRef &, bool *ok) const;
    QV4::ReturnedValue enumValue(
            QQmlTypeLoader *typeLoader, const QV4::String *, bool *ok) const;

    int scopedEnumIndex(QQmlTypeLoader *typeLoader, const QV4::String *, bool *ok) const;
    int scopedEnumIndex(QQmlTypeLoader *typeLoader, const QString &, bool *ok) const;
    int unscopedEnumIndex(QQmlTypeLoader *typeLoader, const QV4::String *, bool *ok) const;
    int unscopedEnumIndex(QQmlTypeLoader *typeLoader, const QString &, bool *ok) const;

    QV4::ReturnedValue scopedEnumValue(
            QQmlTypeLoader *typeLoader, int index, const QV4::String *, bool *ok) const;
    QV4::ReturnedValue scopedEnumValue(
            QQmlTypeLoader *typeLoader, int index, const QString &, bool *ok) const;
    QV4::ReturnedValue scopedEnumValue(
            QQmlTypeLoader *typeLoader, const QHashedStringRef &, const QHashedStringRef &,
            bool *ok) const;

    QV4::ReturnedValue unscopedEnumValue(
            QQmlTypeLoader *typeLoader, int index, const QV4::String *, bool *ok) const;
    QV4::ReturnedValue unscopedEnumValue(
            QQmlTypeLoader *typeLoader, int index, const QString &, bool *ok) const;
    QV4::ReturnedValue unscopedEnumValue(
            QQmlTypeLoader *typeLoader, const QHashedStringRef &, const QHashedStringRef &,
            bool *ok) const;

    QString scopedEnumKey(
            QQmlTypeLoader *typeLoader, int index, const QV4::Value &value, bool *ok) const;
    QStringList scopedEnumKeys(
            QQmlTypeLoader *typeLoader, int index, const QV4::Value &value, bool *ok) const;

    QString unscopedEnumKey(
            QQmlTypeLoader *typeLoader, int index, const QV4::Value &value, bool *ok) const;
    QStringList unscopedEnumKeys(
            QQmlTypeLoader *typeLoader, int index, const QV4::Value &value, bool *ok) const;

    template<typename Int64>
    static QV4::StaticValue encodeEnumValueAsDouble(Int64 value, QMetaType type)
    {
        const auto hexString = [](quint64 value) {
            // Intentionally unsigned. Signed hex values are confusing
            QByteArray result = QByteArray::number(value, 16);
            result.prepend(16 - result.size(), '0');
            return result;
        };

        if (Int64(double(value)) == value) {
            qCInfo(lcQmlEnumValue).nospace().noquote()
                    << "Enum value 0x" << hexString(value) << " of " << type.name()
                    << " does not fit into 32bit integer; encoding as double.";
        } else {
            qCWarning(lcQmlEnumValue).nospace().noquote()
                    << "Enum value 0x" << hexString(value) << " of " << type.name()
                    << " cannot be represented exactly as JavaScript Number; precision loss!";
        }
        return QV4::StaticValue::fromDouble(double(value));
    }

    // Flags are always unsigned regardless of what the metatype reports.
    // See QTBUG-145454: the QFlags metatype doesn't reflect the signedness
    // of the underlying enum. We don't need it to — flags are inherently
    // unsigned bitmasks.
    static bool isUnsignedEnum(const QMetaEnum &metaEnum)
    {
        if (metaEnum.isFlag())
            return true;
        return metaEnum.metaType().flags() & QMetaType::IsUnsignedEnumeration;
    }

    static QV4::StaticValue encodeEnumValue(quint64 value, const QMetaEnum &metaEnum)
    {
        const QMetaType metaType = metaEnum.metaType();

        // For 32-bit flags, undo any sign extension from value64().
        // On MSVC, even unsigned-looking enum values (like 0xffff0000) have
        // signed underlying type, causing value64() to sign-extend.
        // Flags are inherently unsigned bitmasks — zero-extend instead.
        if (metaEnum.isFlag() && !metaEnum.is64Bit())
            value &= 0xFFFFFFFF;

        if (isUnsignedEnum(metaEnum)) {
            return value <= quint64(std::numeric_limits<int>::max())
                    ? QV4::StaticValue::fromInt32(int(value))
                    : encodeEnumValueAsDouble(value, metaType);
        }

        const auto signedValue = qint64(value);
        return (signedValue >= qint64(std::numeric_limits<int>::min())
                    && signedValue <= qint64(std::numeric_limits<int>::max()))
                ? QV4::StaticValue::fromInt32(int(signedValue))
                : encodeEnumValueAsDouble(signedValue, metaType);
    }

    // For a flag QMetaEnum, returns the underlying enum's QMetaEnum. Flag
    // enumerators duplicate the same keys and values as the enum, but
    // keyToValue64() and metaType() may interpret them differently
    // (QTBUG-145454). Using the underlying enum avoids these inconsistencies.
    static QMetaEnum resolveEnum(const QMetaEnum &metaEnum)
    {
        if (!metaEnum.isFlag())
            return metaEnum;

        const QMetaObject *mo = metaEnum.enclosingMetaObject();
        const int idx = mo->indexOfEnumerator(metaEnum.enumName());
        if (idx >= 0)
            return mo->enumerator(idx);
        return metaEnum;
    }

    const QQmlTypePrivate *priv() const { return d.data(); }
    static void refHandle(const QQmlTypePrivate *priv);
    static void derefHandle(const QQmlTypePrivate *priv);
    static int refCount(const QQmlTypePrivate *priv);

    enum RegistrationType {
        CppType = 0,
        SingletonType = 1,
        InterfaceType = 2,
        CompositeType = 3,
        CompositeSingletonType = 4,
        InlineComponentType = 5,
        SequentialContainerType = 6,
        JavaScriptType = 7,
        AnyRegistrationType = 255
    };

    void createProxy(QObject *instance) const;

private:
    friend class QQmlTypePrivate;
    friend size_t qHash(const QQmlType &t, size_t seed);
    friend bool operator==(const QQmlType &a, const QQmlType &b) noexcept
    {
        return a.d.data() == b.d.data();
    }
    friend bool operator!=(const QQmlType &a, const QQmlType &b) noexcept
    {
        return !(a == b);
    }

    QQmlRefPointer<const QQmlTypePrivate> d;
};

inline size_t qHash(const QQmlType &t, size_t seed = 0)
{
    return qHash(reinterpret_cast<quintptr>(t.d.data()), seed);
}

QT_END_NAMESPACE

#endif // QQMLTYPE_P_H
