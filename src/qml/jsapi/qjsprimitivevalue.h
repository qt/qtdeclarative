/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
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

#ifndef QJSPRIMITIVEVALUE_H
#define QJSPRIMITIVEVALUE_H

#include <QtQml/qtqmlglobal.h>
#include <QtQml/qjsnumbercoercion.h>

#include <QtCore/qstring.h>
#include <QtCore/qnumeric.h>
#include <QtCore/qvariant.h>

#include <variant>
#include <cmath>

QT_BEGIN_NAMESPACE

struct QJSPrimitiveUndefined {};
struct QJSPrimitiveNull {};

class QJSPrimitiveValue
{
    template<typename Concrete>
    struct StringNaNOperators
    {
        static constexpr double op(const QString &, QJSPrimitiveUndefined)
        {
            return std::numeric_limits<double>::quiet_NaN();
        }

        static constexpr double op(QJSPrimitiveUndefined, const QString &)
        {
            return std::numeric_limits<double>::quiet_NaN();
        }

        static double op(const QString &lhs, QJSPrimitiveNull) { return op(lhs, 0); }
        static double op(QJSPrimitiveNull, const QString &rhs) { return op(0, rhs); }

        template<typename T>
        static double op(const QString &lhs, T rhs)
        {
            return Concrete::op(fromString(lhs).toDouble(), rhs);
        }

        template<typename T>
        static double op(T lhs, const QString &rhs)
        {
            return Concrete::op(lhs, fromString(rhs).toDouble());
        }

        static double op(const QString &lhs, const QString &rhs)
        {
            return Concrete::op(fromString(lhs).toDouble(), fromString(rhs).toDouble());
        }
    };

    struct AddOperators {
        static constexpr double op(double lhs, double rhs) { return lhs + rhs; }
        static bool opOverflow(int lhs, int rhs, int *result)
        {
            return qAddOverflow(lhs, rhs, result);
        }

        template<typename T>
        static QString op(const QString &lhs, T rhs)
        {
            return lhs + QJSPrimitiveValue(rhs).toString();
        }

        template<typename T>
        static QString op(T lhs, const QString &rhs)
        {
            return QJSPrimitiveValue(lhs).toString() + rhs;
        }

        static QString op(const QString &lhs, const QString &rhs) { return lhs + rhs; }
    };

    struct SubOperators : private StringNaNOperators<SubOperators> {
        static constexpr double op(double lhs, double rhs) { return lhs - rhs; }
        static bool opOverflow(int lhs, int rhs, int *result)
        {
            return qSubOverflow(lhs, rhs, result);
        }

        using StringNaNOperators::op;
    };

    struct MulOperators : private StringNaNOperators<MulOperators> {
        static constexpr double op(double lhs, double rhs) { return lhs * rhs; }
        static bool opOverflow(int lhs, int rhs, int *result)
        {
            return qMulOverflow(lhs, rhs, result);
        }

        using StringNaNOperators::op;
    };

    struct DivOperators : private StringNaNOperators<DivOperators> {
        static constexpr double op(double lhs, double rhs)  { return lhs / rhs; }
        static constexpr bool opOverflow(int, int, int *)
        {
            return true;
        }

        using StringNaNOperators::op;
    };

public:
    enum Type : quint8 {
        Undefined,
        Null,
        Boolean,
        Integer,
        Double,
        String
    };

    constexpr Type type() const { return Type(d.type()); }

    Q_IMPLICIT constexpr QJSPrimitiveValue() noexcept = default;
    Q_IMPLICIT constexpr QJSPrimitiveValue(QJSPrimitiveUndefined undefined) noexcept : d(undefined) {}
    Q_IMPLICIT constexpr QJSPrimitiveValue(QJSPrimitiveNull null) noexcept : d(null) {}
    Q_IMPLICIT constexpr QJSPrimitiveValue(bool value) noexcept : d(value) {}
    Q_IMPLICIT constexpr QJSPrimitiveValue(int value) noexcept : d(value) {}
    Q_IMPLICIT constexpr QJSPrimitiveValue(double value) noexcept : d(value) {}
    Q_IMPLICIT QJSPrimitiveValue(QString string) noexcept : d(std::move(string)) {}
    explicit QJSPrimitiveValue(const QVariant &variant) noexcept
    {
        switch (variant.typeId()) {
        case QMetaType::UnknownType:
            d = QJSPrimitiveUndefined();
            break;
        case QMetaType::Nullptr:
            d = QJSPrimitiveNull();
            break;
        case QMetaType::Bool:
            d = variant.toBool();
            break;
        case QMetaType::Int:
            d = variant.toInt();
            break;
        case QMetaType::Double:
            d = variant.toDouble();
            break;
        case QMetaType::QString:
            d = variant.toString();
            break;
        default:
            // Unsupported. Remains undefined.
            break;
        }
    }

    constexpr bool toBoolean() const
    {
        switch (type()) {
        case Undefined: return false;
        case Null:      return false;
        case Boolean:   return asBoolean();
        case Integer:   return asInteger() != 0;
        case Double: {
            const double v = asDouble();
            return !QJSNumberCoercion::equals(v, 0) && !std::isnan(v);
        }
        case String:    return !asString().isEmpty();
        default:        Q_UNREACHABLE();
        }

        return false;
    }

    constexpr int toInteger() const
    {
        switch (type()) {
        case Undefined: return 0;
        case Null:      return 0;
        case Boolean:   return asBoolean();
        case Integer:   return asInteger();
        case Double:    return QJSNumberCoercion::toInteger(asDouble());
        case String:    return fromString(asString()).toInteger();
        default:        Q_UNREACHABLE();
        }

        return 0;
    }

    constexpr double toDouble() const
    {
        switch (type()) {
        case Undefined: return std::numeric_limits<double>::quiet_NaN();
        case Null:      return 0;
        case Boolean:   return asBoolean();
        case Integer:   return asInteger();
        case Double:    return asDouble();
        case String:    return fromString(asString()).toDouble();
        default:        Q_UNREACHABLE();
        }

        return {};
    }

    QString toString() const
    {
        switch (type()) {
        case Undefined: return QStringLiteral("undefined");
        case Null:      return QStringLiteral("null");
        case Boolean:   return asBoolean() ? QStringLiteral("true") : QStringLiteral("false");
        case Integer:   return QString::number(asInteger());
        case Double: {
            const double result = asDouble();
            if (std::isnan(result))
                return QStringLiteral("NaN");
            if (std::isfinite(result))
                return toString(result);
            if (result > 0)
                return QStringLiteral("Infinity");
            return QStringLiteral("-Infinity");
        }
        case String: return asString();
        }

        Q_UNREACHABLE();
        return QString();
    }

    QVariant toVariant() const
    {
        switch (type()) {
        case Undefined: return QVariant();
        case Null:      return QVariant::fromValue<std::nullptr_t>(nullptr);
        case Boolean:   return QVariant(asBoolean());
        case Integer:   return QVariant(asInteger());
        case Double:    return QVariant(asDouble());
        case String:    return QVariant(asString());
        }

        Q_UNREACHABLE();
        return QVariant();
    }

    friend inline QJSPrimitiveValue operator+(const QJSPrimitiveValue &lhs,
                                              const QJSPrimitiveValue &rhs)
    {
        return operate<AddOperators>(lhs, rhs);
    }

    friend inline QJSPrimitiveValue operator-(const QJSPrimitiveValue &lhs,
                                              const QJSPrimitiveValue &rhs)
    {
        return operate<SubOperators>(lhs, rhs);
    }

    friend inline QJSPrimitiveValue operator*(const QJSPrimitiveValue &lhs,
                                              const QJSPrimitiveValue &rhs)
    {
        return operate<MulOperators>(lhs, rhs);
    }

    friend inline QJSPrimitiveValue operator/(const QJSPrimitiveValue &lhs,
                                              const QJSPrimitiveValue &rhs)
    {
        return operate<DivOperators>(lhs, rhs);
    }

    friend inline QJSPrimitiveValue operator%(const QJSPrimitiveValue &lhs,
                                              const QJSPrimitiveValue &rhs)
    {
        switch (lhs.type()) {
        case Null:
        case Boolean:
        case Integer:
            switch (rhs.type()) {
            case Boolean:
            case Integer: {
                const int leftInt = lhs.toInteger();
                const int rightInt = rhs.toInteger();
                if (leftInt >= 0 && rightInt > 0)
                    return leftInt % rightInt;
                Q_FALLTHROUGH();
            }
            default:
                break;
            }
            Q_FALLTHROUGH();
        default:
            break;
        }

        return std::fmod(lhs.toDouble(), rhs.toDouble());
    }

    constexpr bool strictlyEquals(const QJSPrimitiveValue &other) const
    {
        const Type myType = type();
        const Type otherType = other.type();

        if (myType != otherType) {
            // int -> double promotion is OK in strict mode
            if (myType == Double && otherType == Integer)
                return strictlyEquals(double(other.asInteger()));
            if (myType == Integer && otherType == Double)
                return QJSPrimitiveValue(double(asInteger())).strictlyEquals(other);
            return false;
        }

        switch (myType) {
        case Undefined:
        case Null:
            return true;
        case Boolean:
            return asBoolean() == other.asBoolean();
        case Integer:
            return asInteger() == other.asInteger();
        case Double: {
            const double l = asDouble();
            const double r = other.asDouble();
            if (std::isnan(l) || std::isnan(r))
                return false;
            if (qIsNull(l) && qIsNull(r))
                return true;
            return QJSNumberCoercion::equals(l, r);
        }
        case String:
            return asString() == other.asString();
        }

        return false;
    }

    // Loose operator==, in contrast to strict ===
    constexpr bool equals(const QJSPrimitiveValue &other) const
    {
        const Type myType = type();
        const Type otherType = other.type();

        if (myType == otherType)
            return strictlyEquals(other);

        switch (myType) {
        case Undefined:
            return otherType == Null;
        case Null:
            return otherType == Undefined;
        case Boolean:
            return QJSPrimitiveValue(int(asBoolean())).equals(other);
        case Integer:
            // prefer rhs bool -> int promotion over promoting both to double
            return otherType == Boolean
                    ? QJSPrimitiveValue(asInteger()).equals(int(other.asBoolean()))
                    : QJSPrimitiveValue(double(asInteger())).equals(other);
        case Double:
            // Promote the other side to double (or recognize lhs as undefined/null)
            return other.equals(*this);
        case String:
            return fromString(asString()).parsedEquals(other);
        }

        return false;
    }

    friend constexpr inline bool operator==(const QJSPrimitiveValue &lhs, const
                                            QJSPrimitiveValue &rhs)
    {
        return lhs.strictlyEquals(rhs);
    }

    friend constexpr inline bool operator!=(const QJSPrimitiveValue &lhs,
                                            const QJSPrimitiveValue &rhs)
    {
        return !lhs.strictlyEquals(rhs);
    }

    friend constexpr inline bool operator<(const QJSPrimitiveValue &lhs,
                                           const QJSPrimitiveValue &rhs)
    {
        switch (lhs.type()) {
        case Undefined:
            return false;
        case Null: {
            switch (rhs.type()) {
            case Undefined: return false;
            case Null:      return false;
            case Boolean:   return 0 < int(rhs.asBoolean());
            case Integer:   return 0 < rhs.asInteger();
            case Double:    return double(0) < rhs.asDouble();
            case String:    return double(0) < rhs.toDouble();
            }
            break;
        }
        case Boolean: {
            switch (rhs.type()) {
            case Undefined: return false;
            case Null:      return int(lhs.asBoolean()) < 0;
            case Boolean:   return lhs.asBoolean() < rhs.asBoolean();
            case Integer:   return int(lhs.asBoolean()) < rhs.asInteger();
            case Double:    return double(lhs.asBoolean()) < rhs.asDouble();
            case String:    return double(lhs.asBoolean()) < rhs.toDouble();
            }
            break;
        }
        case Integer: {
            switch (rhs.type()) {
            case Undefined: return false;
            case Null:      return lhs.asInteger() < 0;
            case Boolean:   return lhs.asInteger() < int(rhs.asBoolean());
            case Integer:   return lhs.asInteger() < rhs.asInteger();
            case Double:    return double(lhs.asInteger()) < rhs.asDouble();
            case String:    return double(lhs.asInteger()) < rhs.toDouble();
            }
            break;
        }
        case Double: {
            switch (rhs.type()) {
            case Undefined: return false;
            case Null:      return lhs.asDouble() < double(0);
            case Boolean:   return lhs.asDouble() < double(rhs.asBoolean());
            case Integer:   return lhs.asDouble() < double(rhs.asInteger());
            case Double:    return lhs.asDouble() < rhs.asDouble();
            case String:    return lhs.asDouble() < rhs.toDouble();
            }
            break;
        }
        case String: {
            switch (rhs.type()) {
            case Undefined: return false;
            case Null:      return lhs.toDouble() < double(0);
            case Boolean:   return lhs.toDouble() < double(rhs.asBoolean());
            case Integer:   return lhs.toDouble() < double(rhs.asInteger());
            case Double:    return lhs.toDouble() < rhs.asDouble();
            case String:    return lhs.asString() < rhs.asString();
            }
            break;
        }
        }

        return false;
    }

    friend constexpr inline bool operator>(const QJSPrimitiveValue &lhs, const QJSPrimitiveValue &rhs)
    {
        return rhs < lhs;
    }

    friend constexpr inline bool operator<=(const QJSPrimitiveValue &lhs, const QJSPrimitiveValue &rhs)
    {
        if (lhs.type() == String) {
           if (rhs.type() == String)
               return lhs.asString() <= rhs.asString();
           else
               return fromString(lhs.asString()) <= rhs;
        }
        if (rhs.type() == String)
            return lhs <= fromString(rhs.asString());

        if (lhs.isNanOrUndefined() || rhs.isNanOrUndefined())
            return false;
        return !(lhs > rhs);
    }

    friend constexpr inline bool operator>=(const QJSPrimitiveValue &lhs, const QJSPrimitiveValue &rhs)
    {
        if (lhs.type() == String) {
           if (rhs.type() == String)
               return lhs.asString() >= rhs.asString();
           else
               return fromString(lhs.asString()) >= rhs;
        }
        if (rhs.type() == String)
            return lhs >= fromString(rhs.asString());

        if (lhs.isNanOrUndefined() || rhs.isNanOrUndefined())
            return false;
        return !(lhs < rhs);
    }

private:
    friend class QJSManagedValue;
    friend class QJSValue;

    constexpr bool asBoolean() const { return d.getBool(); }
    constexpr int asInteger() const { return d.getInt(); }
    constexpr double asDouble() const { return d.getDouble(); }
    QString asString() const { return d.getString(); }

    constexpr bool parsedEquals(const QJSPrimitiveValue &other) const
    {
        return type() != Undefined && equals(other);
    }

    static QJSPrimitiveValue fromString(const QString &string)
    {
        bool ok;
        const int intValue = string.toInt(&ok);
        if (ok)
            return intValue;

        const double doubleValue = string.toDouble(&ok);
        if (ok)
            return doubleValue;
        if (string == QStringLiteral("Infinity"))
            return std::numeric_limits<double>::infinity();
        if (string == QStringLiteral("-Infinity"))
            return -std::numeric_limits<double>::infinity();
        if (string == QStringLiteral("NaN"))
            return std::numeric_limits<double>::quiet_NaN();
        return QJSPrimitiveUndefined();
    }

    static Q_QML_EXPORT QString toString(double d);

    template<typename Operators, typename Lhs, typename Rhs>
    static QJSPrimitiveValue operateOnIntegers(const QJSPrimitiveValue &lhs,
                                               const QJSPrimitiveValue &rhs)
    {
        int result;
        if (Operators::opOverflow(lhs.d.get<Lhs>(), rhs.d.get<Rhs>(), &result))
            return Operators::op(lhs.d.get<Lhs>(), rhs.d.get<Rhs>());
        return result;
    }

    template<typename Operators>
    static QJSPrimitiveValue operate(const QJSPrimitiveValue &lhs, const QJSPrimitiveValue &rhs)
    {
        switch (lhs.type()) {
        case Undefined:
            switch (rhs.type()) {
            case Undefined: return std::numeric_limits<double>::quiet_NaN();
            case Null:      return std::numeric_limits<double>::quiet_NaN();
            case Boolean:   return std::numeric_limits<double>::quiet_NaN();
            case Integer:   return std::numeric_limits<double>::quiet_NaN();
            case Double:    return std::numeric_limits<double>::quiet_NaN();
            case String:    return Operators::op(QJSPrimitiveUndefined(), rhs.asString());
            }
            break;
        case Null:
            switch (rhs.type()) {
            case Undefined: return std::numeric_limits<double>::quiet_NaN();
            case Null:      return operateOnIntegers<Operators, int, int>(0, 0);
            case Boolean:   return operateOnIntegers<Operators, int, bool>(0, rhs);
            case Integer:   return operateOnIntegers<Operators, int, int>(0, rhs);
            case Double:    return Operators::op(0, rhs.asDouble());
            case String:    return Operators::op(QJSPrimitiveNull(), rhs.asString());
            }
            break;
        case Boolean:
            switch (rhs.type()) {
            case Undefined: return std::numeric_limits<double>::quiet_NaN();
            case Null:      return operateOnIntegers<Operators, bool, int>(lhs, 0);
            case Boolean:   return operateOnIntegers<Operators, bool, bool>(lhs, rhs);
            case Integer:   return operateOnIntegers<Operators, bool, int>(lhs, rhs);
            case Double:    return Operators::op(lhs.asBoolean(), rhs.asDouble());
            case String:    return Operators::op(lhs.asBoolean(), rhs.asString());
            }
            break;
        case Integer:
            switch (rhs.type()) {
            case Undefined: return std::numeric_limits<double>::quiet_NaN();
            case Null:      return operateOnIntegers<Operators, int, int>(lhs, 0);
            case Boolean:   return operateOnIntegers<Operators, int, bool>(lhs, rhs);
            case Integer:   return operateOnIntegers<Operators, int, int>(lhs, rhs);
            case Double:    return Operators::op(lhs.asInteger(), rhs.asDouble());
            case String:    return Operators::op(lhs.asInteger(), rhs.asString());
            }
            break;
        case Double:
            switch (rhs.type()) {
            case Undefined: return std::numeric_limits<double>::quiet_NaN();
            case Null:      return Operators::op(lhs.asDouble(), 0);
            case Boolean:   return Operators::op(lhs.asDouble(), rhs.asBoolean());
            case Integer:   return Operators::op(lhs.asDouble(), rhs.asInteger());
            case Double:    return Operators::op(lhs.asDouble(), rhs.asDouble());
            case String:    return Operators::op(lhs.asDouble(), rhs.asString());
            }
            break;
        case String:
            switch (rhs.type()) {
            case Undefined: return Operators::op(lhs.asString(), QJSPrimitiveUndefined());
            case Null:      return Operators::op(lhs.asString(), QJSPrimitiveNull());
            case Boolean:   return Operators::op(lhs.asString(), rhs.asBoolean());
            case Integer:   return Operators::op(lhs.asString(), rhs.asInteger());
            case Double:    return Operators::op(lhs.asString(), rhs.asDouble());
            case String:    return Operators::op(lhs.asString(), rhs.asString());
            }
            break;
        }

        Q_UNREACHABLE();
        return QJSPrimitiveUndefined();
    }

    constexpr bool isNanOrUndefined() const
    {
        switch (type()) {
        case Undefined: return true;
        case Double:    return std::isnan(asDouble());
        default:        return false;
        }
    }

    struct QJSPrimitiveValuePrivate
    {
        // Can't be default because QString has a non-trivial ctor.
        constexpr QJSPrimitiveValuePrivate() noexcept {}

        Q_IMPLICIT constexpr QJSPrimitiveValuePrivate(QJSPrimitiveUndefined) noexcept  {}
        Q_IMPLICIT constexpr QJSPrimitiveValuePrivate(QJSPrimitiveNull) noexcept
            : m_type(Null) {}
        Q_IMPLICIT constexpr QJSPrimitiveValuePrivate(bool b) noexcept
            : m_bool(b), m_type(Boolean) {}
        Q_IMPLICIT constexpr QJSPrimitiveValuePrivate(int i) noexcept
            : m_int(i), m_type(Integer) {}
        Q_IMPLICIT constexpr QJSPrimitiveValuePrivate(double d) noexcept
            : m_double(d), m_type(Double) {}
        Q_IMPLICIT QJSPrimitiveValuePrivate(QString s) noexcept
            : m_string(std::move(s)), m_type(String) {}

        constexpr QJSPrimitiveValuePrivate(const QJSPrimitiveValuePrivate &other) noexcept
            : m_type(other.m_type)
        {
            // Not copy-and-swap since swap() would be much more complicated.
            if (!assignSimple(other))
                new (&m_string) QString(other.m_string);
        }

        constexpr QJSPrimitiveValuePrivate(QJSPrimitiveValuePrivate &&other) noexcept
            : m_type(other.m_type)
        {
            // Not move-and-swap since swap() would be much more complicated.
            if (!assignSimple(other))
                new (&m_string) QString(std::move(other.m_string));
        }

        constexpr QJSPrimitiveValuePrivate &operator=(const QJSPrimitiveValuePrivate &other) noexcept
        {
            if (this == &other)
                return *this;

            if (m_type == String) {
                if (other.m_type == String) {
                    m_type = other.m_type;
                    m_string = other.m_string;
                    return *this;
                }
                m_string.~QString();
            }

            m_type = other.m_type;
            if (!assignSimple(other))
                new (&m_string) QString(other.m_string);
            return *this;
        }

        constexpr QJSPrimitiveValuePrivate &operator=(QJSPrimitiveValuePrivate &&other) noexcept
        {
            if (this == &other)
                return *this;

            if (m_type == String) {
                if (other.m_type == String) {
                    m_type = other.m_type;
                    m_string = std::move(other.m_string);
                    return *this;
                }
                m_string.~QString();
            }

            m_type = other.m_type;
            if (!assignSimple(other))
                new (&m_string) QString(std::move(other.m_string));
            return *this;
        }

        ~QJSPrimitiveValuePrivate()
        {
            if (m_type == String)
                m_string.~QString();
        }

        constexpr Type type() const noexcept { return m_type; }
        constexpr bool getBool() const noexcept { return m_bool; }
        constexpr int getInt() const noexcept { return m_int; }
        constexpr double getDouble() const noexcept { return m_double; }
        QString getString() const noexcept { return m_string; }

        template<typename T>
        constexpr T get() const noexcept {
            if constexpr (std::is_same_v<T, QJSPrimitiveUndefined>)
                return QJSPrimitiveUndefined();
            else if constexpr (std::is_same_v<T, QJSPrimitiveNull>)
                return QJSPrimitiveNull();
            else if constexpr (std::is_same_v<T, bool>)
                return getBool();
            else if constexpr (std::is_same_v<T, int>)
                return getInt();
            else if constexpr (std::is_same_v<T, double>)
                return getDouble();
            else if constexpr (std::is_same_v<T, QString>)
                return getString();

            Q_UNREACHABLE();
            return T();
        }

    private:
        constexpr bool assignSimple(const QJSPrimitiveValuePrivate &other) noexcept
        {
            switch (other.m_type) {
            case Undefined:
            case Null:
                return true;
            case Boolean:
                m_bool = other.m_bool;
                return true;
            case Integer:
                m_int = other.m_int;
                return true;
            case Double:
                m_double = other.m_double;
                return true;
            case String:
                return false;
            default:
                Q_UNREACHABLE();
            }
            return false;
        }

        union {
            bool m_bool = false;
            int m_int;
            double m_double;
            QString m_string;
        };

        Type m_type = Undefined;
    };

    QJSPrimitiveValuePrivate d;
};

QT_END_NAMESPACE

#endif // QJSPRIMITIVEVALUE_H
