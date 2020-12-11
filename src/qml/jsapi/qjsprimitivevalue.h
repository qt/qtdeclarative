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

#include <variant>

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
    enum Type {
        Undefined,
        Null,
        Boolean,
        Integer,
        Double,
        String
    };

    constexpr Type type() const { return Type(d.index()); }

    constexpr QJSPrimitiveValue() = default;
    constexpr QJSPrimitiveValue(QJSPrimitiveUndefined undefined) : d(undefined) {}
    constexpr QJSPrimitiveValue(QJSPrimitiveNull null) : d(null) {}
    constexpr QJSPrimitiveValue(bool value) : d(value) {}
    constexpr QJSPrimitiveValue(int value) : d(value) {}
    constexpr QJSPrimitiveValue(double value) : d(value) {}
    QJSPrimitiveValue(QString string) : d(std::move(string)) {}

    constexpr bool toBoolean() const
    {
        switch (type()) {
        case Undefined: return false;
        case Null:      return false;
        case Boolean:   return std::get<bool>(d);
        case Integer:   return std::get<int>(d) != 0;
        case Double: {
            const double v = std::get<double>(d);
            return !QJSNumberCoercion::equals(v, 0) && !std::isnan(v);
        }
        case String:    return !std::get<QString>(d).isEmpty();
        }

        return false;
    }

    constexpr int toInteger() const
    {
        switch (type()) {
        case Undefined: return 0;
        case Null:      return 0;
        case Boolean:   return std::get<bool>(d);
        case Integer:   return std::get<int>(d);
        case Double:    return QJSNumberCoercion::toInteger(std::get<double>(d));
        case String:    return fromString(std::get<String>(d)).toInteger();
        }

        return 0;
    }

    constexpr double toDouble() const
    {
        switch (type()) {
        case Undefined: return std::numeric_limits<double>::quiet_NaN();
        case Null:      return 0;
        case Boolean:   return std::get<bool>(d);
        case Integer:   return std::get<int>(d);
        case Double:    return std::get<double>(d);
        case String:    return fromString(std::get<String>(d)).toDouble();
        }

        return std::numeric_limits<double>::quiet_NaN();
    }

    QString toString() const
    {
        switch (type()) {
        case Undefined: return QStringLiteral("undefined");
        case Null:      return QStringLiteral("null");
        case Boolean:   return std::get<bool>(d) ? QStringLiteral("true") : QStringLiteral("false");
        case Integer:   return QString::number(std::get<int>(d));
        case Double: {
            const double result = std::get<double>(d);
            if (std::isnan(result))
                return QStringLiteral("NaN");
            if (std::isfinite(result))
                return toString(result);
            if (result > 0)
                return QStringLiteral("Infinity");
            return QStringLiteral("-Infinity");
        }
        case String: return std::get<QString>(d);
        }

        Q_UNREACHABLE();
        return QString();
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

    constexpr bool asBoolean() const { return std::get<bool>(d); }
    constexpr int asInteger() const { return std::get<int>(d); }
    constexpr double asDouble() const { return std::get<double>(d); }
    QString asString() const { return std::get<QString>(d); }

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
        if (Operators::opOverflow(std::get<Lhs>(lhs.d), std::get<Rhs>(rhs.d), &result))
            return Operators::op(std::get<Lhs>(lhs.d), std::get<Rhs>(rhs.d));
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

    std::variant<QJSPrimitiveUndefined, QJSPrimitiveNull, bool, int, double, QString> d;
};

QT_END_NAMESPACE

#endif // QJSPRIMITIVEVALUE_H
