/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/
#ifndef QMLJS_RUNTIME_H
#define QMLJS_RUNTIME_H

#include "qv4global_p.h"
#include "qv4value_p.h"
#include "qv4math_p.h"
#include "qv4scopedvalue_p.h"

#include <QtCore/QString>
#include <QtCore/qnumeric.h>
#include <QtCore/QDebug>
#include <QtCore/qurl.h>

#include <cmath>
#include <cassert>
#include <limits>

//#include <wtf/MathExtras.h>

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

struct Function;
struct Object;
struct String;
struct ExecutionContext;
struct FunctionObject;
struct BooleanObject;
struct NumberObject;
struct StringObject;
struct DateObject;
struct RegExpObject;
struct ArrayObject;
struct ErrorObject;
struct ExecutionEngine;
struct InternalClass;

// This is a trick to tell the code generators that functions taking a NoThrowContext won't
// throw exceptions and therefore don't need a check after the call.
struct NoThrowContext : public ExecutionContext
{
};

// context
QV4::ReturnedValue __qmljs_call_activation_property(QV4::ExecutionContext *, const QV4::StringRef name, CallDataRef callData);
QV4::ReturnedValue __qmljs_call_property(QV4::ExecutionContext *context, const QV4::StringRef name, CallDataRef callData);
QV4::ReturnedValue __qmljs_call_property_lookup(ExecutionContext *context, uint index, CallDataRef callData);
QV4::ReturnedValue __qmljs_call_element(ExecutionContext *context, const ValueRef index, CallDataRef callData);
QV4::ReturnedValue __qmljs_call_value(QV4::ExecutionContext *context, const QV4::ValueRef func, CallDataRef callData);

QV4::ReturnedValue __qmljs_construct_activation_property(QV4::ExecutionContext *, const QV4::StringRef name, CallDataRef callData);
QV4::ReturnedValue __qmljs_construct_property(QV4::ExecutionContext *context, const QV4::StringRef name, CallDataRef callData);
QV4::ReturnedValue __qmljs_construct_property_lookup(ExecutionContext *context, uint index, CallDataRef callData);
QV4::ReturnedValue __qmljs_construct_value(QV4::ExecutionContext *context, const QV4::ValueRef func, CallDataRef callData);

QV4::ReturnedValue __qmljs_builtin_typeof(QV4::ExecutionContext *ctx, const QV4::ValueRef val);
QV4::ReturnedValue __qmljs_builtin_typeof_name(QV4::ExecutionContext *context, const QV4::StringRef name);
QV4::ReturnedValue __qmljs_builtin_typeof_member(QV4::ExecutionContext* context, const QV4::ValueRef base, const QV4::StringRef name);
QV4::ReturnedValue __qmljs_builtin_typeof_element(QV4::ExecutionContext* context, const QV4::ValueRef base, const QV4::ValueRef index);

void __qmljs_builtin_rethrow(QV4::ExecutionContext *context);
QV4::ExecutionContext *__qmljs_builtin_push_with_scope(const QV4::ValueRef o, QV4::ExecutionContext *ctx);
QV4::ExecutionContext *__qmljs_builtin_push_catch_scope(QV4::ExecutionContext *ctx, const QV4::StringRef exceptionVarName);
QV4::ExecutionContext *__qmljs_builtin_pop_scope(QV4::ExecutionContext *ctx);
ReturnedValue __qmljs_builtin_unwind_exception(ExecutionContext *ctx);
void __qmljs_builtin_declare_var(QV4::ExecutionContext *ctx, bool deletable, const QV4::StringRef name);
void __qmljs_builtin_define_property(QV4::ExecutionContext *ctx, const QV4::ValueRef object, const QV4::StringRef name, QV4::ValueRef val);
QV4::ReturnedValue __qmljs_builtin_define_array(QV4::ExecutionContext *ctx, QV4::Value *values, uint length);
void __qmljs_builtin_define_getter_setter(QV4::ExecutionContext *ctx, const QV4::ValueRef object, const QV4::StringRef name, const QV4::ValueRef getter, const QV4::ValueRef setter);
QV4::ReturnedValue __qmljs_builtin_define_object_literal(QV4::ExecutionContext *ctx, const QV4::Value *args, int classId);
QV4::ReturnedValue __qmljs_builtin_setup_arguments_object(ExecutionContext *ctx);
void __qmljs_builtin_convert_this_to_object(ExecutionContext *ctx);

QV4::ReturnedValue __qmljs_value_from_string(QV4::String *string);
QV4::ReturnedValue __qmljs_lookup_runtime_regexp(QV4::ExecutionContext *ctx, int id);

// constructors
QV4::ReturnedValue __qmljs_init_closure(QV4::ExecutionContext *ctx, int functionId);

// strings
Q_QML_EXPORT double __qmljs_string_to_number(const QString &s);
Returned<String> *__qmljs_string_from_number(QV4::ExecutionContext *ctx, double number);

// objects
Q_QML_EXPORT ReturnedValue __qmljs_object_default_value(QV4::Object *object, int typeHint);
void __qmljs_set_activation_property(QV4::ExecutionContext *ctx, const QV4::StringRef name, const QV4::ValueRef value);
void __qmljs_set_property(QV4::ExecutionContext *ctx, const QV4::ValueRef object, const QV4::StringRef name, const QV4::ValueRef value);
QV4::ReturnedValue __qmljs_get_property(QV4::ExecutionContext *ctx, const QV4::ValueRef object, const QV4::StringRef name);
QV4::ReturnedValue __qmljs_get_activation_property(QV4::ExecutionContext *ctx, const QV4::StringRef name);

ReturnedValue __qmljs_call_global_lookup(QV4::ExecutionContext *context, uint index, CallDataRef callData);
QV4::ReturnedValue __qmljs_construct_global_lookup(QV4::ExecutionContext *context, uint index, CallDataRef callData);


QV4::ReturnedValue __qmljs_get_element(QV4::ExecutionContext *ctx, const QV4::ValueRef object, const QV4::ValueRef index);
void __qmljs_set_element(QV4::ExecutionContext *ctx, const QV4::ValueRef object, const QV4::ValueRef index, const QV4::ValueRef value);

QV4::ReturnedValue __qmljs_get_id_object(NoThrowContext *ctx, int id);
QV4::ReturnedValue __qmljs_get_imported_scripts(NoThrowContext *ctx);
QV4::ReturnedValue __qmljs_get_context_object(NoThrowContext *ctx);
QV4::ReturnedValue __qmljs_get_scope_object(NoThrowContext *ctx);
QV4::ReturnedValue __qmljs_get_qobject_property(ExecutionContext *ctx, const ValueRef object, int propertyIndex, bool captureRequired);
void __qmljs_set_qobject_property(ExecutionContext *ctx, const ValueRef object, int propertyIndex, const ValueRef value);

// For each
QV4::ReturnedValue __qmljs_foreach_iterator_object(QV4::ExecutionContext *ctx, const QV4::ValueRef in);
QV4::ReturnedValue __qmljs_foreach_next_property_name(const ValueRef foreach_iterator);

// type conversion and testing
QV4::ReturnedValue __qmljs_to_primitive(const ValueRef value, int typeHint);
Q_QML_EXPORT QV4::Bool __qmljs_to_boolean(const QV4::ValueRef value);
double __qmljs_to_number(const QV4::ValueRef value);
QV4::ReturnedValue __qmljs_to_string(QV4::ExecutionContext *ctx, const ValueRef value);
Q_QML_EXPORT Returned<String> *__qmljs_convert_to_string(QV4::ExecutionContext *ctx, const ValueRef value);
void __qmljs_numberToString(QString *result, double num, int radix = 10);
ReturnedValue __qmljs_to_object(QV4::ExecutionContext *ctx, const ValueRef value);
Returned<Object> *__qmljs_convert_to_object(QV4::ExecutionContext *ctx, const ValueRef value);

QV4::Bool __qmljs_equal_helper(const ValueRef x, const ValueRef y);
Q_QML_EXPORT QV4::Bool __qmljs_strict_equal(const ValueRef x, const ValueRef y);

// unary operators
typedef QV4::ReturnedValue (*UnaryOpName)(const QV4::ValueRef);
QV4::ReturnedValue __qmljs_uplus(const QV4::ValueRef value);
QV4::ReturnedValue __qmljs_uminus(const QV4::ValueRef value);
QV4::ReturnedValue __qmljs_compl(const QV4::ValueRef value);
QV4::ReturnedValue __qmljs_not(const QV4::ValueRef value);
QV4::ReturnedValue __qmljs_increment(const QV4::ValueRef value);
QV4::ReturnedValue __qmljs_decrement(const QV4::ValueRef value);

Q_QML_EXPORT ReturnedValue __qmljs_value_to_double(const ValueRef value);
Q_QML_EXPORT int __qmljs_value_to_int32(const ValueRef value);
Q_QML_EXPORT int __qmljs_double_to_int32(const double &d);
Q_QML_EXPORT unsigned __qmljs_value_to_uint32(const ValueRef value);
Q_QML_EXPORT unsigned __qmljs_double_to_uint32(const double &d);

QV4::ReturnedValue __qmljs_delete_subscript(QV4::ExecutionContext *ctx, const QV4::ValueRef base, const QV4::ValueRef index);
ReturnedValue __qmljs_delete_member(QV4::ExecutionContext *ctx, const QV4::ValueRef base, const QV4::StringRef name);
ReturnedValue __qmljs_delete_name(QV4::ExecutionContext *ctx, const QV4::StringRef name);

void __qmljs_throw(QV4::ExecutionContext*, const QV4::ValueRef value);

// binary operators
typedef QV4::ReturnedValue (*BinOp)(const QV4::ValueRef left, const QV4::ValueRef right);
typedef QV4::ReturnedValue (*BinOpContext)(QV4::ExecutionContext *ctx, const QV4::ValueRef left, const QV4::ValueRef right);

QV4::ReturnedValue __qmljs_instanceof(QV4::ExecutionContext *ctx, const QV4::ValueRef left, const QV4::ValueRef right);
QV4::ReturnedValue __qmljs_in(QV4::ExecutionContext *ctx, const QV4::ValueRef left, const QV4::ValueRef right);
QV4::ReturnedValue __qmljs_add(ExecutionContext *ctx, const QV4::ValueRef left, const QV4::ValueRef right);
QV4::ReturnedValue __qmljs_add_string(QV4::ExecutionContext *ctx, const QV4::ValueRef left, const QV4::ValueRef right);
QV4::ReturnedValue __qmljs_bit_or(const QV4::ValueRef left, const QV4::ValueRef right);
QV4::ReturnedValue __qmljs_bit_xor(const QV4::ValueRef left, const QV4::ValueRef right);
QV4::ReturnedValue __qmljs_bit_and(const QV4::ValueRef left, const QV4::ValueRef right);
QV4::ReturnedValue __qmljs_sub(const QV4::ValueRef left, const QV4::ValueRef right);
QV4::ReturnedValue __qmljs_mul(const QV4::ValueRef left, const QV4::ValueRef right);
QV4::ReturnedValue __qmljs_div(const QV4::ValueRef left, const QV4::ValueRef right);
QV4::ReturnedValue __qmljs_mod(const QV4::ValueRef left, const QV4::ValueRef right);
QV4::ReturnedValue __qmljs_shl(const QV4::ValueRef left, const QV4::ValueRef right);
QV4::ReturnedValue __qmljs_shr(const QV4::ValueRef left, const QV4::ValueRef right);
QV4::ReturnedValue __qmljs_ushr(const QV4::ValueRef left, const QV4::ValueRef right);
QV4::ReturnedValue __qmljs_gt(const QV4::ValueRef left, const QV4::ValueRef right);
QV4::ReturnedValue __qmljs_lt(const QV4::ValueRef left, const QV4::ValueRef right);
QV4::ReturnedValue __qmljs_ge(const QV4::ValueRef left, const QV4::ValueRef right);
QV4::ReturnedValue __qmljs_le(const QV4::ValueRef left, const QV4::ValueRef right);
QV4::ReturnedValue __qmljs_eq(const QV4::ValueRef left, const QV4::ValueRef right);
QV4::ReturnedValue __qmljs_ne(const QV4::ValueRef left, const QV4::ValueRef right);
QV4::ReturnedValue __qmljs_se(const QV4::ValueRef left, const QV4::ValueRef right);
QV4::ReturnedValue __qmljs_sne(const QV4::ValueRef left, const QV4::ValueRef right);

QV4::ReturnedValue __qmljs_add_helper(QV4::ExecutionContext *ctx, const QV4::ValueRef left, const QV4::ValueRef right);

typedef QV4::Bool (*CmpOp)(const QV4::ValueRef left, const QV4::ValueRef right);
QV4::Bool __qmljs_cmp_gt(const QV4::ValueRef l, const QV4::ValueRef r);
QV4::Bool __qmljs_cmp_lt(const QV4::ValueRef l, const QV4::ValueRef r);
QV4::Bool __qmljs_cmp_ge(const QV4::ValueRef l, const QV4::ValueRef r);
QV4::Bool __qmljs_cmp_le(const QV4::ValueRef l, const QV4::ValueRef r);
QV4::Bool __qmljs_cmp_eq(const QV4::ValueRef left, const QV4::ValueRef right);
QV4::Bool __qmljs_cmp_ne(const QV4::ValueRef left, const QV4::ValueRef right);
QV4::Bool __qmljs_cmp_se(const QV4::ValueRef left, const QV4::ValueRef right);
QV4::Bool __qmljs_cmp_sne(const QV4::ValueRef left, const QV4::ValueRef right);

typedef QV4::Bool (*CmpOpContext)(QV4::ExecutionContext *ctx, const QV4::ValueRef left, const QV4::ValueRef right);
QV4::Bool __qmljs_cmp_instanceof(QV4::ExecutionContext *ctx, const QV4::ValueRef left, const QV4::ValueRef right);
QV4::Bool __qmljs_cmp_in(QV4::ExecutionContext *ctx, const QV4::ValueRef left, const QV4::ValueRef right);

// type conversion and testing
inline ReturnedValue __qmljs_to_primitive(const QV4::ValueRef value, int typeHint)
{
    QV4::Object *o = value->asObject();
    if (!o)
        return value.asReturnedValue();
    return __qmljs_object_default_value(o, typeHint);
}

inline double __qmljs_to_number(const ValueRef value)
{
    return value->toNumber();
}

inline QV4::ReturnedValue __qmljs_uplus(const QV4::ValueRef value)
{
    TRACE1(value);

    if (value->isNumber())
        return value.asReturnedValue();
    if (value->integerCompatible())
        return Encode(value->int_32);

    double n = value->toNumberImpl();
    return Encode(n);
}

inline QV4::ReturnedValue __qmljs_uminus(const QV4::ValueRef value)
{
    TRACE1(value);

    // +0 != -0, so we need to convert to double when negating 0
    if (value->isInteger() && value->integerValue())
        return Encode(-value->integerValue());
    else {
        double n = __qmljs_to_number(value);
        return Encode(-n);
    }
}

inline QV4::ReturnedValue __qmljs_compl(const QV4::ValueRef value)
{
    TRACE1(value);

    int n;
    if (value->integerCompatible())
        n = value->int_32;
    else
        n = value->toInt32();

    return Encode((int)~n);
}

inline QV4::ReturnedValue __qmljs_not(const QV4::ValueRef value)
{
    TRACE1(value);

    bool b = value->toBoolean();
    return Encode(!b);
}

// binary operators
inline ReturnedValue __qmljs_bit_or(const QV4::ValueRef left, const QV4::ValueRef right)
{
    TRACE2(left, right);

    if (QV4::Value::integerCompatible(*left, *right))
        return Encode(left->integerValue() | right->integerValue());

    int lval = left->toInt32();
    int rval = right->toInt32();
    return Encode(lval | rval);
}

inline ReturnedValue __qmljs_bit_xor(const QV4::ValueRef left, const QV4::ValueRef right)
{
    TRACE2(left, right);

    if (QV4::Value::integerCompatible(*left, *right))
        return Encode(left->integerValue() ^ right->integerValue());

    int lval = left->toInt32();
    int rval = right->toInt32();
    return Encode(lval ^ rval);
}

inline ReturnedValue __qmljs_bit_and(const QV4::ValueRef left, const QV4::ValueRef right)
{
    TRACE2(left, right);

    if (QV4::Value::integerCompatible(*left, *right))
        return Encode(left->integerValue() & right->integerValue());

    int lval = left->toInt32();
    int rval = right->toInt32();
    return Encode(lval & rval);
}

inline QV4::ReturnedValue __qmljs_add(QV4::ExecutionContext *ctx, const QV4::ValueRef left, const QV4::ValueRef right)
{
    TRACE2(left, right);

    if (QV4::Value::integerCompatible(*left, *right))
        return add_int32(left->integerValue(), right->integerValue()).asReturnedValue();

    if (QV4::Value::bothDouble(*left, *right))
        return QV4::Primitive::fromDouble(left->doubleValue() + right->doubleValue()).asReturnedValue();

    return __qmljs_add_helper(ctx, left, right);
}

inline QV4::ReturnedValue __qmljs_sub(const QV4::ValueRef left, const QV4::ValueRef right)
{
    TRACE2(left, right);

    if (QV4::Value::integerCompatible(*left, *right))
        return sub_int32(left->integerValue(), right->integerValue()).asReturnedValue();

    double lval = __qmljs_to_number(left);
    double rval = __qmljs_to_number(right);
    return QV4::Primitive::fromDouble(lval - rval).asReturnedValue();
}

inline QV4::ReturnedValue __qmljs_mul(const QV4::ValueRef left, const QV4::ValueRef right)
{
    TRACE2(left, right);

    if (QV4::Value::integerCompatible(*left, *right))
        return mul_int32(left->integerValue(), right->integerValue()).asReturnedValue();

    double lval = __qmljs_to_number(left);
    double rval = __qmljs_to_number(right);
    return QV4::Primitive::fromDouble(lval * rval).asReturnedValue();
}

inline QV4::ReturnedValue __qmljs_div(const QV4::ValueRef left, const QV4::ValueRef right)
{
    TRACE2(left, right);

    double lval = __qmljs_to_number(left);
    double rval = __qmljs_to_number(right);
    return QV4::Primitive::fromDouble(lval / rval).asReturnedValue();
}

inline QV4::ReturnedValue __qmljs_mod(const QV4::ValueRef left, const QV4::ValueRef right)
{
    TRACE2(left, right);

    if (QV4::Value::integerCompatible(*left, *right) && right->integerValue() != 0) {
        int intRes = left->integerValue() % right->integerValue();
        if (intRes != 0 || left->integerValue() >= 0)
            return Encode(intRes);
    }

    double lval = __qmljs_to_number(left);
    double rval = __qmljs_to_number(right);
    return QV4::Primitive::fromDouble(std::fmod(lval, rval)).asReturnedValue();
}

inline QV4::ReturnedValue __qmljs_shl(const QV4::ValueRef left, const QV4::ValueRef right)
{
    TRACE2(left, right);

    if (QV4::Value::integerCompatible(*left, *right))
        return Encode((int)(left->integerValue() << ((uint(right->integerValue()) & 0x1f))));

    int lval = left->toInt32();
    unsigned rval = right->toUInt32() & 0x1f;
    return Encode((int)(lval << rval));
}

inline QV4::ReturnedValue __qmljs_shr(const QV4::ValueRef left, const QV4::ValueRef right)
{
    TRACE2(left, right);

    if (QV4::Value::integerCompatible(*left, *right))
        return Encode((int)(left->integerValue() >> ((uint(right->integerValue()) & 0x1f))));

    int lval = left->toInt32();
    unsigned rval = right->toUInt32() & 0x1f;
    return Encode((int)(lval >> rval));
}

inline QV4::ReturnedValue __qmljs_ushr(const QV4::ValueRef left, const QV4::ValueRef right)
{
    TRACE2(left, right);

    uint res;
    if (QV4::Value::integerCompatible(*left, *right)) {
        res = uint(left->integerValue()) >> (uint(right->integerValue()) & 0x1f);
    } else {
        unsigned lval = left->toUInt32();
        unsigned rval = right->toUInt32() & 0x1f;
        res = lval >> rval;
    }

    return Encode(res);
}

inline QV4::ReturnedValue __qmljs_gt(const QV4::ValueRef left, const QV4::ValueRef right)
{
    TRACE2(left, right);

    bool r = __qmljs_cmp_gt(left, right);
    return Encode(r);
}

inline QV4::ReturnedValue __qmljs_lt(const QV4::ValueRef left, const QV4::ValueRef right)
{
    TRACE2(left, right);

    bool r = __qmljs_cmp_lt(left, right);
    return Encode(r);
}

inline QV4::ReturnedValue __qmljs_ge(const QV4::ValueRef left, const QV4::ValueRef right)
{
    TRACE2(left, right);

    bool r = __qmljs_cmp_ge(left, right);
    return Encode(r);
}

inline QV4::ReturnedValue __qmljs_le(const QV4::ValueRef left, const QV4::ValueRef right)
{
    TRACE2(left, right);

    bool r = __qmljs_cmp_le(left, right);
    return Encode(r);
}

inline QV4::ReturnedValue __qmljs_eq(const QV4::ValueRef left, const QV4::ValueRef right)
{
    TRACE2(left, right);

    bool r = __qmljs_cmp_eq(left, right);
    return Encode(r);
}

inline QV4::ReturnedValue __qmljs_ne(const QV4::ValueRef left, const QV4::ValueRef right)
{
    TRACE2(left, right);

    bool r = !__qmljs_cmp_eq(left, right);
    return Encode(r);
}

inline QV4::ReturnedValue __qmljs_se(const QV4::ValueRef left, const QV4::ValueRef right)
{
    TRACE2(left, right);

    bool r = __qmljs_strict_equal(left, right);
    return Encode(r);
}

inline QV4::ReturnedValue __qmljs_sne(const QV4::ValueRef left, const QV4::ValueRef right)
{
    TRACE2(left, right);

    bool r = ! __qmljs_strict_equal(left, right);
    return Encode(r);
}

inline QV4::Bool __qmljs_cmp_eq(const QV4::ValueRef left, const QV4::ValueRef right)
{
    TRACE2(left, right);

    if (left->rawValue() == right->rawValue())
        // NaN != NaN
        return !left->isNaN();

    if (left->type() == right->type()) {
        if (!left->isManaged())
            return false;
        if (left->isString() == right->isString())
            return left->managed()->isEqualTo(right->managed());
    }

    return __qmljs_equal_helper(left, right);
}

inline QV4::Bool __qmljs_cmp_ne(const QV4::ValueRef left, const QV4::ValueRef right)
{
    TRACE2(left, right);

    return !__qmljs_cmp_eq(left, right);
}

inline QV4::Bool __qmljs_cmp_se(const QV4::ValueRef left, const QV4::ValueRef right)
{
    TRACE2(left, right);

    return __qmljs_strict_equal(left, right);
}

inline QV4::Bool __qmljs_cmp_sne(const QV4::ValueRef left, const QV4::ValueRef right)
{
    TRACE2(left, right);

    return ! __qmljs_strict_equal(left, right);
}

inline QV4::Bool __qmljs_cmp_instanceof(QV4::ExecutionContext *ctx, const QV4::ValueRef left, const QV4::ValueRef right)
{
    TRACE2(left, right);

    Scope scope(ctx);
    QV4::ScopedValue v(scope, __qmljs_instanceof(ctx, left, right));
    return v->booleanValue();
}

inline uint __qmljs_cmp_in(QV4::ExecutionContext *ctx, const QV4::ValueRef left, const QV4::ValueRef right)
{
    TRACE2(left, right);

    Scope scope(ctx);
    QV4::ScopedValue v(scope, __qmljs_in(ctx, left, right));
    return v->booleanValue();
}

} // namespace QV4

QT_END_NAMESPACE

#endif // QMLJS_RUNTIME_H
