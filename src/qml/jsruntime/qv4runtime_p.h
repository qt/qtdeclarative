/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#ifndef QMLJS_RUNTIME_H
#define QMLJS_RUNTIME_H

#include "qv4global_p.h"
#include "qv4value_inl_p.h"
#include "qv4context_p.h"
#include "qv4math_p.h"

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

// This is a trick to tell the code generators that functions taking a NoThrowContext won't
// throw exceptions and therefore don't need a check after the call.
struct NoThrowEngine : public ExecutionEngine
{
};

struct Q_QML_PRIVATE_EXPORT Runtime {
    // call
    static ReturnedValue callGlobalLookup(ExecutionEngine *engine, uint index, CallData *callData);
    static ReturnedValue callActivationProperty(ExecutionEngine *engine, int nameIndex, CallData *callData);
    static ReturnedValue callProperty(ExecutionEngine *engine, int nameIndex, CallData *callData);
    static ReturnedValue callPropertyLookup(ExecutionEngine *engine, uint index, CallData *callData);
    static ReturnedValue callElement(ExecutionEngine *engine, const ValueRef index, CallData *callData);
    static ReturnedValue callValue(ExecutionEngine *engine, const ValueRef func, CallData *callData);

    // construct
    static ReturnedValue constructGlobalLookup(ExecutionEngine *engine, uint index, CallData *callData);
    static ReturnedValue constructActivationProperty(ExecutionEngine *engine, int nameIndex, CallData *callData);
    static ReturnedValue constructProperty(ExecutionEngine *engine, int nameIndex, CallData *callData);
    static ReturnedValue constructPropertyLookup(ExecutionEngine *engine, uint index, CallData *callData);
    static ReturnedValue constructValue(ExecutionEngine *engine, const ValueRef func, CallData *callData);

    // set & get
    static void setActivationProperty(ExecutionEngine *engine, int nameIndex, const ValueRef value);
    static void setProperty(ExecutionEngine *engine, const ValueRef object, int nameIndex, const ValueRef value);
    static void setElement(ExecutionEngine *engine, const ValueRef object, const ValueRef index, const ValueRef value);
    static ReturnedValue getProperty(ExecutionEngine *engine, const ValueRef object, int nameIndex);
    static ReturnedValue getActivationProperty(ExecutionEngine *engine, int nameIndex);
    static ReturnedValue getElement(ExecutionEngine *engine, const ValueRef object, const ValueRef index);

    // typeof
    static ReturnedValue typeofValue(ExecutionEngine *engine, const ValueRef val);
    static ReturnedValue typeofName(ExecutionEngine *engine, int nameIndex);
    static ReturnedValue typeofMember(ExecutionEngine *engine, const ValueRef base, int nameIndex);
    static ReturnedValue typeofElement(ExecutionEngine *engine, const ValueRef base, const ValueRef index);

    // delete
    static ReturnedValue deleteElement(ExecutionEngine *engine, const ValueRef base, const ValueRef index);
    static ReturnedValue deleteMember(ExecutionEngine *engine, const ValueRef base, int nameIndex);
    static ReturnedValue deleteMemberString(ExecutionEngine *engine, const ValueRef base, String *name);
    static ReturnedValue deleteName(ExecutionEngine *engine, int nameIndex);

    // exceptions & scopes
    static void throwException(ExecutionEngine *engine, const ValueRef value);
    static ReturnedValue unwindException(ExecutionEngine *engine);
    static void pushWithScope(const ValueRef o, NoThrowEngine *engine);
    static void pushCatchScope(NoThrowEngine *engine, int exceptionVarNameIndex);
    static void popScope(ExecutionEngine *engine);

    // closures
    static ReturnedValue closure(ExecutionEngine *engine, int functionId);

    // function header
    static void declareVar(ExecutionEngine *engine, bool deletable, int nameIndex);
    static ReturnedValue setupArgumentsObject(ExecutionEngine *engine);
    static void convertThisToObject(ExecutionEngine *engine);

    // literals
    static ReturnedValue arrayLiteral(ExecutionEngine *engine, Value *values, uint length);
    static ReturnedValue objectLiteral(ExecutionEngine *engine, const Value *args, int classId, int arrayValueCount, int arrayGetterSetterCountAndFlags);
    static ReturnedValue regexpLiteral(ExecutionEngine *engine, int id);

    // foreach
    static ReturnedValue foreachIterator(ExecutionEngine *engine, const ValueRef in);
    static ReturnedValue foreachNextPropertyName(const ValueRef foreach_iterator);

    // unary operators
    typedef ReturnedValue (*UnaryOperation)(const ValueRef);
    static ReturnedValue uPlus(const ValueRef value);
    static ReturnedValue uMinus(const ValueRef value);
    static ReturnedValue uNot(const ValueRef value);
    static ReturnedValue complement(const ValueRef value);
    static ReturnedValue increment(const ValueRef value);
    static ReturnedValue decrement(const ValueRef value);

    // binary operators
    typedef ReturnedValue (*BinaryOperation)(const ValueRef left, const ValueRef right);
    typedef ReturnedValue (*BinaryOperationContext)(ExecutionEngine *engine, const ValueRef left, const ValueRef right);

    static ReturnedValue instanceof(ExecutionEngine *engine, const ValueRef left, const ValueRef right);
    static ReturnedValue in(ExecutionEngine *engine, const ValueRef left, const ValueRef right);
    static ReturnedValue add(ExecutionEngine *engine, const ValueRef left, const ValueRef right);
    static ReturnedValue addString(ExecutionEngine *engine, const ValueRef left, const ValueRef right);
    static ReturnedValue bitOr(const ValueRef left, const ValueRef right);
    static ReturnedValue bitXor(const ValueRef left, const ValueRef right);
    static ReturnedValue bitAnd(const ValueRef left, const ValueRef right);
    static ReturnedValue sub(const ValueRef left, const ValueRef right);
    static ReturnedValue mul(const ValueRef left, const ValueRef right);
    static ReturnedValue div(const ValueRef left, const ValueRef right);
    static ReturnedValue mod(const ValueRef left, const ValueRef right);
    static ReturnedValue shl(const ValueRef left, const ValueRef right);
    static ReturnedValue shr(const ValueRef left, const ValueRef right);
    static ReturnedValue ushr(const ValueRef left, const ValueRef right);
    static ReturnedValue greaterThan(const ValueRef left, const ValueRef right);
    static ReturnedValue lessThan(const ValueRef left, const ValueRef right);
    static ReturnedValue greaterEqual(const ValueRef left, const ValueRef right);
    static ReturnedValue lessEqual(const ValueRef left, const ValueRef right);
    static ReturnedValue equal(const ValueRef left, const ValueRef right);
    static ReturnedValue notEqual(const ValueRef left, const ValueRef right);
    static ReturnedValue strictEqual(const ValueRef left, const ValueRef right);
    static ReturnedValue strictNotEqual(const ValueRef left, const ValueRef right);

    // comparisons
    typedef Bool (*CompareOperation)(const ValueRef left, const ValueRef right);
    static Bool compareGreaterThan(const ValueRef l, const ValueRef r);
    static Bool compareLessThan(const ValueRef l, const ValueRef r);
    static Bool compareGreaterEqual(const ValueRef l, const ValueRef r);
    static Bool compareLessEqual(const ValueRef l, const ValueRef r);
    static Bool compareEqual(const ValueRef left, const ValueRef right);
    static Bool compareNotEqual(const ValueRef left, const ValueRef right);
    static Bool compareStrictEqual(const ValueRef left, const ValueRef right);
    static Bool compareStrictNotEqual(const ValueRef left, const ValueRef right);

    typedef Bool (*CompareOperationContext)(ExecutionEngine *engine, const ValueRef left, const ValueRef right);
    static Bool compareInstanceof(ExecutionEngine *engine, const ValueRef left, const ValueRef right);
    static Bool compareIn(ExecutionEngine *engine, const ValueRef left, const ValueRef right);

    // conversions
    static Bool toBoolean(const ValueRef value);
    static ReturnedValue toDouble(const ValueRef value);
    static int toInt(const ValueRef value);
    static int doubleToInt(const double &d);
    static unsigned toUInt(const ValueRef value);
    static unsigned doubleToUInt(const double &d);

    // qml
    static ReturnedValue getQmlIdArray(NoThrowEngine *ctx);
    static ReturnedValue getQmlImportedScripts(NoThrowEngine *ctx);
    static ReturnedValue getQmlContextObject(NoThrowEngine *ctx);
    static ReturnedValue getQmlScopeObject(NoThrowEngine *ctx);
    static ReturnedValue getQmlSingleton(NoThrowEngine *ctx, int nameIndex);
    static ReturnedValue getQmlAttachedProperty(ExecutionEngine *engine, int attachedPropertiesId, int propertyIndex);
    static ReturnedValue getQmlQObjectProperty(ExecutionEngine *engine, const ValueRef object, int propertyIndex, bool captureRequired);
    static ReturnedValue getQmlSingletonQObjectProperty(ExecutionEngine *engine, const ValueRef object, int propertyIndex, bool captureRequired);
    static void setQmlQObjectProperty(ExecutionEngine *engine, const ValueRef object, int propertyIndex, const ValueRef value);
};

struct Q_QML_PRIVATE_EXPORT RuntimeHelpers {
    static ReturnedValue objectDefaultValue(Object *object, int typeHint);
    static ReturnedValue toPrimitive(const ValueRef value, int typeHint);

    static double stringToNumber(const QString &s);
    static Heap::String *stringFromNumber(ExecutionEngine *engine, double number);
    static double toNumber(const ValueRef value);
    static void numberToString(QString *result, double num, int radix = 10);

    static ReturnedValue toString(ExecutionEngine *engine, const ValueRef value);
    static Heap::String *convertToString(ExecutionEngine *engine, const ValueRef value);

    static ReturnedValue toObject(ExecutionEngine *engine, const ValueRef value);
    static Heap::Object *convertToObject(ExecutionEngine *engine, const ValueRef value);

    static Bool equalHelper(const ValueRef x, const ValueRef y);
    static Bool strictEqual(const ValueRef x, const ValueRef y);

    static ReturnedValue addHelper(ExecutionEngine *engine, const ValueRef left, const ValueRef right);
};


// type conversion and testing
#ifndef V4_BOOTSTRAP
inline ReturnedValue RuntimeHelpers::toPrimitive(const ValueRef value, int typeHint)
{
    Object *o = value->asObject();
    if (!o)
        return value.asReturnedValue();
    return RuntimeHelpers::objectDefaultValue(o, typeHint);
}
#endif

inline double RuntimeHelpers::toNumber(const ValueRef value)
{
    return value->toNumber();
}

inline ReturnedValue Runtime::uPlus(const ValueRef value)
{
    TRACE1(value);

    if (value->isNumber())
        return value.asReturnedValue();
    if (value->integerCompatible())
        return Encode(value->int_32);

    double n = value->toNumberImpl();
    return Encode(n);
}

inline ReturnedValue Runtime::uMinus(const ValueRef value)
{
    TRACE1(value);

    // +0 != -0, so we need to convert to double when negating 0
    if (value->isInteger() && value->integerValue())
        return Encode(-value->integerValue());
    else {
        double n = RuntimeHelpers::toNumber(value);
        return Encode(-n);
    }
}

inline ReturnedValue Runtime::complement(const ValueRef value)
{
    TRACE1(value);

    int n = value->toInt32();
    return Encode((int)~n);
}

inline ReturnedValue Runtime::uNot(const ValueRef value)
{
    TRACE1(value);

    bool b = value->toBoolean();
    return Encode(!b);
}

// binary operators
inline ReturnedValue Runtime::bitOr(const ValueRef left, const ValueRef right)
{
    TRACE2(left, right);

    int lval = left->toInt32();
    int rval = right->toInt32();
    return Encode(lval | rval);
}

inline ReturnedValue Runtime::bitXor(const ValueRef left, const ValueRef right)
{
    TRACE2(left, right);

    int lval = left->toInt32();
    int rval = right->toInt32();
    return Encode(lval ^ rval);
}

inline ReturnedValue Runtime::bitAnd(const ValueRef left, const ValueRef right)
{
    TRACE2(left, right);

    int lval = left->toInt32();
    int rval = right->toInt32();
    return Encode(lval & rval);
}

#ifndef V4_BOOTSTRAP
inline ReturnedValue Runtime::add(ExecutionEngine *engine, const ValueRef left, const ValueRef right)
{
    TRACE2(left, right);

    if (Q_LIKELY(left->isInteger() && right->isInteger()))
        return add_int32(left->integerValue(), right->integerValue());
    if (left->isNumber() && right->isNumber())
        return Primitive::fromDouble(left->asDouble() + right->asDouble()).asReturnedValue();

    return RuntimeHelpers::addHelper(engine, left, right);
}
#endif // V4_BOOTSTRAP

inline ReturnedValue Runtime::sub(const ValueRef left, const ValueRef right)
{
    TRACE2(left, right);

    if (Q_LIKELY(left->isInteger() && right->isInteger()))
        return sub_int32(left->integerValue(), right->integerValue());

    double lval = left->isNumber() ? left->asDouble() : left->toNumberImpl();
    double rval = right->isNumber() ? right->asDouble() : right->toNumberImpl();

    return Primitive::fromDouble(lval - rval).asReturnedValue();
}

inline ReturnedValue Runtime::mul(const ValueRef left, const ValueRef right)
{
    TRACE2(left, right);

    if (Q_LIKELY(left->isInteger() && right->isInteger()))
        return mul_int32(left->integerValue(), right->integerValue());

    double lval = left->isNumber() ? left->asDouble() : left->toNumberImpl();
    double rval = right->isNumber() ? right->asDouble() : right->toNumberImpl();

    return Primitive::fromDouble(lval * rval).asReturnedValue();
}

inline ReturnedValue Runtime::div(const ValueRef left, const ValueRef right)
{
    TRACE2(left, right);

    double lval = left->toNumber();
    double rval = right->toNumber();
    return Primitive::fromDouble(lval / rval).asReturnedValue();
}

inline ReturnedValue Runtime::mod(const ValueRef left, const ValueRef right)
{
    TRACE2(left, right);

    if (Value::integerCompatible(*left, *right) && right->integerValue() != 0) {
        int intRes = left->integerValue() % right->integerValue();
        if (intRes != 0 || left->integerValue() >= 0)
            return Encode(intRes);
    }

    double lval = RuntimeHelpers::toNumber(left);
    double rval = RuntimeHelpers::toNumber(right);
    return Primitive::fromDouble(std::fmod(lval, rval)).asReturnedValue();
}

inline ReturnedValue Runtime::shl(const ValueRef left, const ValueRef right)
{
    TRACE2(left, right);

    int lval = left->toInt32();
    int rval = right->toInt32() & 0x1f;
    return Encode((int)(lval << rval));
}

inline ReturnedValue Runtime::shr(const ValueRef left, const ValueRef right)
{
    TRACE2(left, right);

    int lval = left->toInt32();
    unsigned rval = right->toUInt32() & 0x1f;
    return Encode((int)(lval >> rval));
}

inline ReturnedValue Runtime::ushr(const ValueRef left, const ValueRef right)
{
    TRACE2(left, right);

    unsigned lval = left->toUInt32();
    unsigned rval = right->toUInt32() & 0x1f;
    uint res = lval >> rval;

    return Encode(res);
}

inline ReturnedValue Runtime::greaterThan(const ValueRef left, const ValueRef right)
{
    TRACE2(left, right);

    bool r = Runtime::compareGreaterThan(left, right);
    return Encode(r);
}

inline ReturnedValue Runtime::lessThan(const ValueRef left, const ValueRef right)
{
    TRACE2(left, right);

    bool r = Runtime::compareLessThan(left, right);
    return Encode(r);
}

inline ReturnedValue Runtime::greaterEqual(const ValueRef left, const ValueRef right)
{
    TRACE2(left, right);

    bool r = Runtime::compareGreaterEqual(left, right);
    return Encode(r);
}

inline ReturnedValue Runtime::lessEqual(const ValueRef left, const ValueRef right)
{
    TRACE2(left, right);

    bool r = Runtime::compareLessEqual(left, right);
    return Encode(r);
}

inline Bool Runtime::compareEqual(const ValueRef left, const ValueRef right)
{
    TRACE2(left, right);

    if (left->rawValue() == right->rawValue())
        // NaN != NaN
        return !left->isNaN();

    if (left->type() == right->type()) {
        if (!left->isManaged())
            return false;
        if (left->isString() == right->isString())
            return left->cast<Managed>()->isEqualTo(right->cast<Managed>());
    }

    return RuntimeHelpers::equalHelper(left, right);
}

inline ReturnedValue Runtime::equal(const ValueRef left, const ValueRef right)
{
    TRACE2(left, right);

    bool r = Runtime::compareEqual(left, right);
    return Encode(r);
}

inline ReturnedValue Runtime::notEqual(const ValueRef left, const ValueRef right)
{
    TRACE2(left, right);

    bool r = !Runtime::compareEqual(left, right);
    return Encode(r);
}

inline ReturnedValue Runtime::strictEqual(const ValueRef left, const ValueRef right)
{
    TRACE2(left, right);

    bool r = RuntimeHelpers::strictEqual(left, right);
    return Encode(r);
}

inline ReturnedValue Runtime::strictNotEqual(const ValueRef left, const ValueRef right)
{
    TRACE2(left, right);

    bool r = ! RuntimeHelpers::strictEqual(left, right);
    return Encode(r);
}

inline Bool Runtime::compareNotEqual(const ValueRef left, const ValueRef right)
{
    TRACE2(left, right);

    return !Runtime::compareEqual(left, right);
}

inline Bool Runtime::compareStrictEqual(const ValueRef left, const ValueRef right)
{
    TRACE2(left, right);

    return RuntimeHelpers::strictEqual(left, right);
}

inline Bool Runtime::compareStrictNotEqual(const ValueRef left, const ValueRef right)
{
    TRACE2(left, right);

    return ! RuntimeHelpers::strictEqual(left, right);
}

inline Bool Runtime::toBoolean(const ValueRef value)
{
    return value->toBoolean();
}

} // namespace QV4

QT_END_NAMESPACE

#endif // QMLJS_RUNTIME_H
