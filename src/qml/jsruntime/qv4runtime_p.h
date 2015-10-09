/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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
#ifndef QMLJS_RUNTIME_H
#define QMLJS_RUNTIME_H

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

#include "qv4global_p.h"
#include "qv4value_p.h"
#include "qv4context_p.h"
#include "qv4engine_p.h"
#include "qv4math_p.h"
#include "qv4runtimeapi_p.h"
#include <QtCore/qnumeric.h>


QT_BEGIN_NAMESPACE

#undef QV4_COUNT_RUNTIME_FUNCTIONS

namespace QV4 {

#ifdef QV4_COUNT_RUNTIME_FUNCTIONS
class RuntimeCounters
{
public:
    RuntimeCounters();
    ~RuntimeCounters();

    static RuntimeCounters *instance;

    void count(const char *func);
    void count(const char *func, uint tag);
    void count(const char *func, uint tag1, uint tag2);

private:
    struct Data;
    Data *d;
};

#  define TRACE0() RuntimeCounters::instance->count(Q_FUNC_INFO);
#  define TRACE1(x) RuntimeCounters::instance->count(Q_FUNC_INFO, x->type());
#  define TRACE2(x, y) RuntimeCounters::instance->count(Q_FUNC_INFO, x->type(), y->type());
#else
#  define TRACE0()
#  define TRACE1(x)
#  define TRACE2(x, y)
#endif // QV4_COUNT_RUNTIME_FUNCTIONS

enum TypeHint {
    PREFERREDTYPE_HINT,
    NUMBER_HINT,
    STRING_HINT
};


struct Q_QML_PRIVATE_EXPORT RuntimeHelpers {
    static ReturnedValue objectDefaultValue(const Object *object, int typeHint);
    static ReturnedValue toPrimitive(const Value &value, int typeHint);

    static double stringToNumber(const QString &s);
    static Heap::String *stringFromNumber(ExecutionEngine *engine, double number);
    static double toNumber(const Value &value);
    static void numberToString(QString *result, double num, int radix = 10);

    static ReturnedValue toString(ExecutionEngine *engine, const Value &value);
    static Heap::String *convertToString(ExecutionEngine *engine, const Value &value);

    static ReturnedValue toObject(ExecutionEngine *engine, const Value &value);
    static Heap::Object *convertToObject(ExecutionEngine *engine, const Value &value);

    static Bool equalHelper(const Value &x, const Value &y);
    static Bool strictEqual(const Value &x, const Value &y);

    static ReturnedValue addHelper(ExecutionEngine *engine, const Value &left, const Value &right);
};


// type conversion and testing
#ifndef V4_BOOTSTRAP
inline ReturnedValue RuntimeHelpers::toPrimitive(const Value &value, int typeHint)
{
    const Object *o = value.as<Object>();
    if (!o)
        return value.asReturnedValue();
    return RuntimeHelpers::objectDefaultValue(o, typeHint);
}
#endif

inline double RuntimeHelpers::toNumber(const Value &value)
{
    return value.toNumber();
}

inline ReturnedValue Runtime::method_uPlus(const Value &value)
{
    TRACE1(value);

    if (value.isNumber())
        return value.asReturnedValue();
    if (value.integerCompatible())
        return Encode(value.int_32());

    double n = value.toNumberImpl();
    return Encode(n);
}

inline ReturnedValue Runtime::method_uMinus(const Value &value)
{
    TRACE1(value);

    // +0 != -0, so we need to convert to double when negating 0
    if (value.isInteger() && value.integerValue())
        return Encode(-value.integerValue());
    else {
        double n = RuntimeHelpers::toNumber(value);
        return Encode(-n);
    }
}

inline ReturnedValue Runtime::method_complement(const Value &value)
{
    TRACE1(value);

    int n = value.toInt32();
    return Encode((int)~n);
}

inline ReturnedValue Runtime::method_uNot(const Value &value)
{
    TRACE1(value);

    bool b = value.toBoolean();
    return Encode(!b);
}

// binary operators
inline ReturnedValue Runtime::bitOr(const Value &left, const Value &right)
{
    TRACE2(left, right);

    int lval = left.toInt32();
    int rval = right.toInt32();
    return Encode(lval | rval);
}

inline ReturnedValue Runtime::bitXor(const Value &left, const Value &right)
{
    TRACE2(left, right);

    int lval = left.toInt32();
    int rval = right.toInt32();
    return Encode(lval ^ rval);
}

inline ReturnedValue Runtime::bitAnd(const Value &left, const Value &right)
{
    TRACE2(left, right);

    int lval = left.toInt32();
    int rval = right.toInt32();
    return Encode(lval & rval);
}

#ifndef V4_BOOTSTRAP
inline ReturnedValue Runtime::add(ExecutionEngine *engine, const Value &left, const Value &right)
{
    TRACE2(left, right);

    if (Q_LIKELY(left.isInteger() && right.isInteger()))
        return add_int32(left.integerValue(), right.integerValue());
    if (left.isNumber() && right.isNumber())
        return Primitive::fromDouble(left.asDouble() + right.asDouble()).asReturnedValue();

    return RuntimeHelpers::addHelper(engine, left, right);
}
#endif // V4_BOOTSTRAP

inline ReturnedValue Runtime::sub(const Value &left, const Value &right)
{
    TRACE2(left, right);

    if (Q_LIKELY(left.isInteger() && right.isInteger()))
        return sub_int32(left.integerValue(), right.integerValue());

    double lval = left.isNumber() ? left.asDouble() : left.toNumberImpl();
    double rval = right.isNumber() ? right.asDouble() : right.toNumberImpl();

    return Primitive::fromDouble(lval - rval).asReturnedValue();
}

inline ReturnedValue Runtime::mul(const Value &left, const Value &right)
{
    TRACE2(left, right);

    if (Q_LIKELY(left.isInteger() && right.isInteger()))
        return mul_int32(left.integerValue(), right.integerValue());

    double lval = left.isNumber() ? left.asDouble() : left.toNumberImpl();
    double rval = right.isNumber() ? right.asDouble() : right.toNumberImpl();

    return Primitive::fromDouble(lval * rval).asReturnedValue();
}

inline ReturnedValue Runtime::div(const Value &left, const Value &right)
{
    TRACE2(left, right);

    if (Value::integerCompatible(left, right)) {
        int lval = left.integerValue();
        int rval = right.integerValue();
        if (rval != 0 && (lval % rval == 0))
            return Encode(int(lval / rval));
        else
            return Encode(double(lval) / rval);
    }

    double lval = left.toNumber();
    double rval = right.toNumber();
    return Primitive::fromDouble(lval / rval).asReturnedValue();
}

inline ReturnedValue Runtime::mod(const Value &left, const Value &right)
{
    TRACE2(left, right);

    if (Value::integerCompatible(left, right) && right.integerValue() != 0) {
        int intRes = left.integerValue() % right.integerValue();
        if (intRes != 0 || left.integerValue() >= 0)
            return Encode(intRes);
    }

    double lval = RuntimeHelpers::toNumber(left);
    double rval = RuntimeHelpers::toNumber(right);
    return Primitive::fromDouble(std::fmod(lval, rval)).asReturnedValue();
}

inline ReturnedValue Runtime::shl(const Value &left, const Value &right)
{
    TRACE2(left, right);

    int lval = left.toInt32();
    int rval = right.toInt32() & 0x1f;
    return Encode((int)(lval << rval));
}

inline ReturnedValue Runtime::shr(const Value &left, const Value &right)
{
    TRACE2(left, right);

    int lval = left.toInt32();
    unsigned rval = right.toUInt32() & 0x1f;
    return Encode((int)(lval >> rval));
}

inline ReturnedValue Runtime::ushr(const Value &left, const Value &right)
{
    TRACE2(left, right);

    unsigned lval = left.toUInt32();
    unsigned rval = right.toUInt32() & 0x1f;
    uint res = lval >> rval;

    return Encode(res);
}

inline ReturnedValue Runtime::greaterThan(const Value &left, const Value &right)
{
    TRACE2(left, right);

    bool r = Runtime::compareGreaterThan(left, right);
    return Encode(r);
}

inline ReturnedValue Runtime::lessThan(const Value &left, const Value &right)
{
    TRACE2(left, right);

    bool r = Runtime::compareLessThan(left, right);
    return Encode(r);
}

inline ReturnedValue Runtime::greaterEqual(const Value &left, const Value &right)
{
    TRACE2(left, right);

    bool r = Runtime::compareGreaterEqual(left, right);
    return Encode(r);
}

inline ReturnedValue Runtime::lessEqual(const Value &left, const Value &right)
{
    TRACE2(left, right);

    bool r = Runtime::compareLessEqual(left, right);
    return Encode(r);
}

inline Bool Runtime::compareEqual(const Value &left, const Value &right)
{
    TRACE2(left, right);

    if (left.rawValue() == right.rawValue())
        // NaN != NaN
        return !left.isNaN();

    if (left.type() == right.type()) {
        if (!left.isManaged())
            return false;
        if (left.isString() == right.isString())
            return left.cast<Managed>()->isEqualTo(right.cast<Managed>());
    }

    return RuntimeHelpers::equalHelper(left, right);
}

inline ReturnedValue Runtime::equal(const Value &left, const Value &right)
{
    TRACE2(left, right);

    bool r = Runtime::compareEqual(left, right);
    return Encode(r);
}

inline ReturnedValue Runtime::notEqual(const Value &left, const Value &right)
{
    TRACE2(left, right);

    bool r = !Runtime::compareEqual(left, right);
    return Encode(r);
}

inline ReturnedValue Runtime::strictEqual(const Value &left, const Value &right)
{
    TRACE2(left, right);

    bool r = RuntimeHelpers::strictEqual(left, right);
    return Encode(r);
}

inline ReturnedValue Runtime::strictNotEqual(const Value &left, const Value &right)
{
    TRACE2(left, right);

    bool r = ! RuntimeHelpers::strictEqual(left, right);
    return Encode(r);
}

inline Bool Runtime::compareNotEqual(const Value &left, const Value &right)
{
    TRACE2(left, right);

    return !Runtime::compareEqual(left, right);
}

inline Bool Runtime::compareStrictEqual(const Value &left, const Value &right)
{
    TRACE2(left, right);

    return RuntimeHelpers::strictEqual(left, right);
}

inline Bool Runtime::compareStrictNotEqual(const Value &left, const Value &right)
{
    TRACE2(left, right);

    return ! RuntimeHelpers::strictEqual(left, right);
}

inline Bool Runtime::method_toBoolean(const Value &value)
{
    return value.toBoolean();
}

} // namespace QV4

QT_END_NAMESPACE

#endif // QMLJS_RUNTIME_H
