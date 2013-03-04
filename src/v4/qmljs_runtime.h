/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the V4VM module of the Qt Toolkit.
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

#include "qv4global.h"
#include "qmljs_value.h"
#include "qmljs_math.h"


#include <QtCore/QString>
#include <QtCore/qnumeric.h>
#include <QtCore/QDebug>

#include <cmath>
#include <cassert>

#include <wtf/MathExtras.h>

#ifdef DO_TRACE_INSTR
#  define TRACE1(x) fprintf(stderr, "    %s\n", __FUNCTION__);
#  define TRACE2(x, y) fprintf(stderr, "    %s\n", __FUNCTION__);
#else
#  define TRACE1(x)
#  define TRACE2(x, y)
#endif // TRACE1

QT_BEGIN_NAMESPACE

namespace QQmlJS {
namespace VM {

enum TypeHint {
    PREFERREDTYPE_HINT,
    NUMBER_HINT,
    STRING_HINT
};

struct Function;
struct Object;
struct String;
struct PropertyDescriptor;
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

struct Q_V4_EXPORT Exception {
    explicit Exception(ExecutionContext *throwingContext);
    ~Exception();

    void accept(ExecutionContext *catchingContext);

    void partiallyUnwindContext(ExecutionContext *catchingContext);
private:
    ExecutionContext *throwingContext;
    bool accepted;
};

extern "C" {

// context
void __qmljs_call_activation_property(ExecutionContext *, Value *result, String *name, Value *args, int argc);
void __qmljs_call_property(ExecutionContext *context, Value *result, const Value &that, String *name, Value *args, int argc);
void __qmljs_call_property_lookup(ExecutionContext *context, Value *result, const Value &thisObject, uint index, Value *args, int argc);
void __qmljs_call_element(ExecutionContext *context, Value *result, const Value &that, const Value &index, Value *args, int argc);
void __qmljs_call_value(ExecutionContext *context, Value *result, const Value *thisObject, const Value &func, Value *args, int argc);

void __qmljs_construct_activation_property(ExecutionContext *, Value *result, String *name, Value *args, int argc);
void __qmljs_construct_property(ExecutionContext *context, Value *result, const Value &base, String *name, Value *args, int argc);
void __qmljs_construct_value(ExecutionContext *context, Value *result, const Value &func, Value *args, int argc);

void __qmljs_builtin_typeof(ExecutionContext *ctx, Value *result, const Value &val);
void __qmljs_builtin_typeof_name(ExecutionContext *context, Value* result, String *name);
void __qmljs_builtin_typeof_member(ExecutionContext* context, Value* result, const Value &base, String *name);
void __qmljs_builtin_typeof_element(ExecutionContext* context, Value *result, const Value &base, const Value &index);

void __qmljs_builtin_post_increment(ExecutionContext *ctx, Value *result, Value *val);
void __qmljs_builtin_post_increment_name(ExecutionContext *context, Value *result, String *name);
void __qmljs_builtin_post_increment_member(ExecutionContext *context, Value *result, const Value &base, String *name);
void __qmljs_builtin_post_increment_element(ExecutionContext *context, Value *result, const Value &base, const Value *index);

void __qmljs_builtin_post_decrement(ExecutionContext *ctx, Value *result, Value *val);
void __qmljs_builtin_post_decrement_name(ExecutionContext *context, Value *result, String *name);
void __qmljs_builtin_post_decrement_member(ExecutionContext *context, Value *result, const Value &base, String *name);
void __qmljs_builtin_post_decrement_element(ExecutionContext *context, Value *result, const Value &base, const Value &index);

void Q_NORETURN __qmljs_builtin_throw(ExecutionContext *context, const Value &val);
void Q_NORETURN __qmljs_builtin_rethrow(ExecutionContext *context);
ExecutionContext *__qmljs_builtin_push_with_scope(const Value &o, ExecutionContext *ctx);
ExecutionContext *__qmljs_builtin_push_catch_scope(String *exceptionVarName, ExecutionContext *ctx);
ExecutionContext *__qmljs_builtin_pop_scope(ExecutionContext *ctx);
void __qmljs_builtin_declare_var(ExecutionContext *ctx, bool deletable, String *name);
void __qmljs_builtin_define_property(ExecutionContext *ctx, const Value &object, String *name, Value *val);
void __qmljs_builtin_define_array(ExecutionContext *ctx, Value *array, QQmlJS::VM::Value *values, uint length);
void __qmljs_builtin_define_getter_setter(ExecutionContext *ctx, const Value &object, String *name, const Value *getter, const Value *setter);

// constructors
void __qmljs_init_closure(ExecutionContext *ctx, Value *result, VM::Function *clos);
VM::Function *__qmljs_register_function(ExecutionContext *ctx, String *name,
                                        bool hasDirectEval,
                                        bool usesArgumentsObject, bool isStrict,
                                        bool hasNestedFunctions,
                                        String **formals, unsigned formalCount,
                                        String **locals, unsigned localCount);


// strings
double __qmljs_string_to_number(const String *string);
Value __qmljs_string_from_number(ExecutionContext *ctx, double number);
Bool __qmljs_string_compare(ExecutionContext *ctx, String *left, String *right);
Bool __qmljs_string_equal(String *left, String *right);
String *__qmljs_string_concat(ExecutionContext *ctx, String *first, String *second);

// objects
Value __qmljs_object_default_value(ExecutionContext *ctx, Value object, int typeHint);
void __qmljs_set_activation_property(ExecutionContext *ctx, String *name, const Value& value);
void __qmljs_set_property(ExecutionContext *ctx, const Value &object, String *name, const Value &value);
void __qmljs_get_property(ExecutionContext *ctx, Value *result, const Value &object, String *name);
void __qmljs_get_activation_property(ExecutionContext *ctx, Value *result, String *name);

void __qmljs_get_property_lookup(ExecutionContext *ctx, Value *result, const Value &object, int lookupIndex);
void __qmljs_set_property_lookup(ExecutionContext *ctx, const Value &object, int lookupIndex, const Value &value);


void __qmljs_get_element(ExecutionContext *ctx, Value *retval, const Value &object, const Value &index);
void __qmljs_set_element(ExecutionContext *ctx, const Value &object, const Value &index, const Value &value);

// For each
void __qmljs_foreach_iterator_object(ExecutionContext *ctx, Value *result, const Value &in);
void __qmljs_foreach_next_property_name(Value *result, const Value &foreach_iterator);

// context
void __qmljs_get_thisObject(ExecutionContext *ctx, Value *result);

// type conversion and testing
Value __qmljs_to_primitive(const Value &value, ExecutionContext *ctx, int typeHint);
Bool __qmljs_to_boolean(const Value &value);
double __qmljs_to_number(const Value &value, ExecutionContext *ctx);
Value __qmljs_to_string(const Value &value, ExecutionContext *ctx);
Q_V4_EXPORT String *__qmljs_convert_to_string(ExecutionContext *ctx, const Value &value);
Value __qmljs_to_object(ExecutionContext *ctx, const Value &value);
Object *__qmljs_convert_to_object(ExecutionContext *ctx, const Value &value);
Value __qmljs_default_value(const Value &value, ExecutionContext *ctx, int typeHint);

Bool __qmljs_equal(const Value &x, const Value &y, ExecutionContext *ctx);
Bool __qmljs_strict_equal(const Value &x, const Value &y);

// unary operators
typedef void (*UnaryOpName)(ExecutionContext *, Value *, const Value &);
void __qmljs_uplus(ExecutionContext *ctx, Value *result, const Value &value);
void __qmljs_uminus(ExecutionContext *ctx, Value *result, const Value &value);
void __qmljs_compl(ExecutionContext *ctx, Value *result, const Value &value);
void __qmljs_not(ExecutionContext *ctx, Value *result, const Value &value);
void __qmljs_increment(ExecutionContext *ctx, Value *result, const Value &value);
void __qmljs_decrement(ExecutionContext *ctx, Value *result, const Value &value);

void __qmljs_delete_subscript(ExecutionContext *ctx, Value *result, const Value &base, const Value &index);
void __qmljs_delete_member(ExecutionContext *ctx, Value *result, const Value &base, String *name);
void __qmljs_delete_name(ExecutionContext *ctx, Value *result, String *name);

void Q_NORETURN __qmljs_throw(ExecutionContext*, const Value &value);
// actually returns a jmp_buf *
Q_V4_EXPORT void __qmljs_create_exception_handler(ExecutionContext *context);
void __qmljs_get_exception(ExecutionContext *context, Value *result);

// binary operators
typedef void (*BinOp)(ExecutionContext *ctx, Value *result, const Value &left, const Value &right);

void __qmljs_instanceof(ExecutionContext *ctx, Value *result, const Value &left, const Value &right);
void __qmljs_in(ExecutionContext *ctx, Value *result, const Value &left, const Value &right);
void __qmljs_bit_or(ExecutionContext *ctx, Value *result, const Value &left, const Value &right);
void __qmljs_bit_xor(ExecutionContext *ctx, Value *result, const Value &left, const Value &right);
void __qmljs_bit_and(ExecutionContext *ctx, Value *result, const Value &left, const Value &right);
void __qmljs_add(ExecutionContext *ctx, Value *result, const Value &left, const Value &right);
void __qmljs_sub(ExecutionContext *ctx, Value *result, const Value &left, const Value &right);
void __qmljs_mul(ExecutionContext *ctx, Value *result, const Value &left, const Value &right);
void __qmljs_div(ExecutionContext *ctx, Value *result, const Value &left, const Value &right);
void __qmljs_mod(ExecutionContext *ctx, Value *result, const Value &left, const Value &right);
void __qmljs_shl(ExecutionContext *ctx, Value *result, const Value &left, const Value &right);
void __qmljs_shr(ExecutionContext *ctx, Value *result, const Value &left, const Value &right);
void __qmljs_ushr(ExecutionContext *ctx, Value *result, const Value &left, const Value &right);
void __qmljs_gt(ExecutionContext *ctx, Value *result, const Value &left, const Value &right);
void __qmljs_lt(ExecutionContext *ctx, Value *result, const Value &left, const Value &right);
void __qmljs_ge(ExecutionContext *ctx, Value *result, const Value &left, const Value &right);
void __qmljs_le(ExecutionContext *ctx, Value *result, const Value &left, const Value &right);
void __qmljs_eq(ExecutionContext *ctx, Value *result, const Value &left, const Value &right);
void __qmljs_ne(ExecutionContext *ctx, Value *result, const Value &left, const Value &right);
void __qmljs_se(ExecutionContext *ctx, Value *result, const Value &left, const Value &right);
void __qmljs_sne(ExecutionContext *ctx, Value *result, const Value &left, const Value &right);

void __qmljs_add_helper(ExecutionContext *ctx, Value *result, const Value &left, const Value &right);


typedef void (*InplaceBinOpName)(ExecutionContext *ctx, String *name, const Value &value);
void __qmljs_inplace_bit_and_name(ExecutionContext *ctx, String *name, const Value &value);
void __qmljs_inplace_bit_or_name(ExecutionContext *ctx, String *name, const Value &value);
void __qmljs_inplace_bit_xor_name(ExecutionContext *ctx, String *name, const Value &value);
void __qmljs_inplace_add_name(ExecutionContext *ctx, String *name, const Value &value);
void __qmljs_inplace_sub_name(ExecutionContext *ctx, String *name, const Value &value);
void __qmljs_inplace_mul_name(ExecutionContext *ctx, String *name, const Value &value);
void __qmljs_inplace_div_name(ExecutionContext *ctx, String *name, const Value &value);
void __qmljs_inplace_mod_name(ExecutionContext *ctx, String *name, const Value &value);
void __qmljs_inplace_shl_name(ExecutionContext *ctx, String *name, const Value &value);
void __qmljs_inplace_shr_name(ExecutionContext *ctx, String *name, const Value &value);
void __qmljs_inplace_ushr_name(ExecutionContext *ctx, String *name, const Value &value);

typedef void (*InplaceBinOpElement)(ExecutionContext *ctx, const Value &base, const Value &index, const Value &rhs);
void __qmljs_inplace_bit_and_element(ExecutionContext *ctx, const Value &base, const Value &index, const Value &rhs);
void __qmljs_inplace_bit_or_element(ExecutionContext *ctx, const Value &base, const Value &index, const Value &rhs);
void __qmljs_inplace_bit_xor_element(ExecutionContext *ctx, const Value &base, const Value &index, const Value &rhs);
void __qmljs_inplace_add_element(ExecutionContext *ctx, const Value &base, const Value &index, const Value &rhs);
void __qmljs_inplace_sub_element(ExecutionContext *ctx, const Value &base, const Value &index, const Value &rhs);
void __qmljs_inplace_mul_element(ExecutionContext *ctx, const Value &base, const Value &index, const Value &rhs);
void __qmljs_inplace_div_element(ExecutionContext *ctx, const Value &base, const Value &index, const Value &rhs);
void __qmljs_inplace_mod_element(ExecutionContext *ctx, const Value &base, const Value &index, const Value &rhs);
void __qmljs_inplace_shl_element(ExecutionContext *ctx, const Value &base, const Value &index, const Value &rhs);
void __qmljs_inplace_shr_element(ExecutionContext *ctx, const Value &base, const Value &index, const Value &rhs);
void __qmljs_inplace_ushr_element(ExecutionContext *ctx, const Value &base, const Value &index, const Value &rhs);

typedef void (*InplaceBinOpMember)(ExecutionContext *ctx, const Value &base, String *name, const Value &rhs);
void __qmljs_inplace_bit_and_member(ExecutionContext *ctx, const Value &base, String *name, const Value &rhs);
void __qmljs_inplace_bit_or_member(ExecutionContext *ctx, const Value &base, String *name, const Value &rhs);
void __qmljs_inplace_bit_xor_member(ExecutionContext *ctx, const Value &base, String *name, const Value &rhs);
void __qmljs_inplace_add_member(ExecutionContext *ctx, const Value &base, String *name, const Value &rhs);
void __qmljs_inplace_sub_member(ExecutionContext *ctx, const Value &base, String *name, const Value &rhs);
void __qmljs_inplace_mul_member(ExecutionContext *ctx, const Value &base, String *name, const Value &rhs);
void __qmljs_inplace_div_member(ExecutionContext *ctx, const Value &base, String *name, const Value &rhs);
void __qmljs_inplace_mod_member(ExecutionContext *ctx, const Value &base, String *name, const Value &rhs);
void __qmljs_inplace_shl_member(ExecutionContext *ctx, const Value &base, String *name, const Value &rhs);
void __qmljs_inplace_shr_member(ExecutionContext *ctx, const Value &base, String *name, const Value &rhs);
void __qmljs_inplace_ushr_member(ExecutionContext *ctx, const Value &base, String *name, const Value &rhs);

typedef Bool (*CmpOp)(ExecutionContext *ctx, const Value &left, const Value &right);
Bool __qmljs_cmp_gt(ExecutionContext *ctx, const Value &left, const Value &right);
Bool __qmljs_cmp_lt(ExecutionContext *ctx, const Value &left, const Value &right);
Bool __qmljs_cmp_ge(ExecutionContext *ctx, const Value &left, const Value &right);
Bool __qmljs_cmp_le(ExecutionContext *ctx, const Value &left, const Value &right);
Bool __qmljs_cmp_eq(ExecutionContext *ctx, const Value &left, const Value &right);
Bool __qmljs_cmp_ne(ExecutionContext *ctx, const Value &left, const Value &right);
Bool __qmljs_cmp_se(ExecutionContext *ctx, const Value &left, const Value &right);
Bool __qmljs_cmp_sne(ExecutionContext *ctx, const Value &left, const Value &right);
Bool __qmljs_cmp_instanceof(ExecutionContext *ctx, const Value &left, const Value &right);
Bool __qmljs_cmp_in(ExecutionContext *ctx, const Value &left, const Value &right);

// type conversion and testing
inline Value __qmljs_to_primitive(const Value &value, ExecutionContext *ctx, int typeHint)
{
    if (!value.isObject())
        return value;
    return __qmljs_default_value(value, ctx, typeHint);
}

inline double __qmljs_to_number(const Value &value, ExecutionContext *ctx)
{
    switch (value.type()) {
    case Value::Undefined_Type:
        return std::numeric_limits<double>::quiet_NaN();
    case Value::Null_Type:
        return 0;
    case Value::Boolean_Type:
        return (value.booleanValue() ? 1. : 0.);
    case Value::Integer_Type:
        return value.int_32;
    case Value::String_Type:
        return __qmljs_string_to_number(value.stringValue());
    case Value::Object_Type: {
        Value prim = __qmljs_to_primitive(value, ctx, NUMBER_HINT);
        return __qmljs_to_number(prim, ctx);
    }
    default: // double
        return value.doubleValue();
    }
}

inline Value __qmljs_to_string(const Value &value, ExecutionContext *ctx)
{
    if (value.isString())
        return value;
    return Value::fromString(__qmljs_convert_to_string(ctx, value));
}

inline Value __qmljs_to_object(ExecutionContext *ctx, const Value &value)
{
    if (value.isObject())
        return value;
    return Value::fromObject(__qmljs_convert_to_object(ctx, value));
}


inline Value __qmljs_default_value(const Value &value, ExecutionContext *ctx, int typeHint)
{
    if (value.isObject())
        return __qmljs_object_default_value(ctx, value, typeHint);
    return Value::undefinedValue();
}


inline void __qmljs_uplus(ExecutionContext *ctx, Value *result, const Value &value)
{
    TRACE1(value);

    *result = value;
    if (result->tryIntegerConversion())
        return;

    double n = __qmljs_to_number(value, ctx);
    *result = Value::fromDouble(n);
}

inline void __qmljs_uminus(ExecutionContext *ctx, Value *result, const Value &value)
{
    TRACE1(value);

    // +0 != -0, so we need to convert to double when negating 0
    if (value.isInteger() && value.integerValue())
        *result = Value::fromInt32(-value.integerValue());
    else {
        double n = __qmljs_to_number(value, ctx);
        *result = Value::fromDouble(-n);
    }
}

inline void __qmljs_compl(ExecutionContext *ctx, Value *result, const Value &value)
{
    TRACE1(value);

    int n;
    if (value.isConvertibleToInt())
        n = value.int_32;
    else
        n = Value::toInt32(__qmljs_to_number(value, ctx));

    *result = Value::fromInt32(~n);
}

inline void __qmljs_not(ExecutionContext *, Value *result, const Value &value)
{
    TRACE1(value);

    bool b = value.toBoolean();
    *result = Value::fromBoolean(!b);
}

// binary operators
inline void __qmljs_bit_or(ExecutionContext *ctx, Value *result, const Value &left, const Value &right)
{
    TRACE2(left, right);

    if (Value::integerCompatible(left, right)) {
        *result = Value::fromInt32(left.integerValue() | right.integerValue());
        return;
    }

    int lval = Value::toInt32(__qmljs_to_number(left, ctx));
    int rval = Value::toInt32(__qmljs_to_number(right, ctx));
    *result = Value::fromInt32(lval | rval);
}

inline void __qmljs_bit_xor(ExecutionContext *ctx, Value *result, const Value &left, const Value &right)
{
    TRACE2(left, right);

    if (Value::integerCompatible(left, right)) {
        *result = Value::fromInt32(left.integerValue() ^ right.integerValue());
        return;
    }

    int lval = Value::toInt32(__qmljs_to_number(left, ctx));
    int rval = Value::toInt32(__qmljs_to_number(right, ctx));
    *result = Value::fromInt32(lval ^ rval);
}

inline void __qmljs_bit_and(ExecutionContext *ctx, Value *result, const Value &left, const Value &right)
{
    TRACE2(left, right);

    if (Value::integerCompatible(left, right)) {
        *result = Value::fromInt32(left.integerValue() & right.integerValue());
        return;
    }

    int lval = Value::toInt32(__qmljs_to_number(left, ctx));
    int rval = Value::toInt32(__qmljs_to_number(right, ctx));
    *result = Value::fromInt32(lval & rval);
}

inline void __qmljs_add(ExecutionContext *ctx, Value *result, const Value &left, const Value &right)
{
    TRACE2(left, right);

#ifdef QMLJS_INLINE_MATH
    if (Value::integerCompatible(left, right)) {
        *result = add_int32(left.integerValue(), right.integerValue());
        return;
    }
#endif

    if (Value::bothDouble(left, right)) {
        *result = Value::fromDouble(left.doubleValue() + right.doubleValue());
        return;
    }

    __qmljs_add_helper(ctx, result, left, right);
}

inline void __qmljs_sub(ExecutionContext *ctx, Value *result, const Value &left, const Value &right)
{
    TRACE2(left, right);

#ifdef QMLJS_INLINE_MATH
    if (Value::integerCompatible(left, right)) {
        *result = sub_int32(left.integerValue(), right.integerValue());
        return;
    }
#endif

    double lval = __qmljs_to_number(left, ctx);
    double rval = __qmljs_to_number(right, ctx);
    *result = Value::fromDouble(lval - rval);
}

inline void __qmljs_mul(ExecutionContext *ctx, Value *result, const Value &left, const Value &right)
{
    TRACE2(left, right);

#ifdef QMLJS_INLINE_MATH
    if (Value::integerCompatible(left, right)) {
        *result = mul_int32(left.integerValue(), right.integerValue());
        return;
    }
#endif

    double lval = __qmljs_to_number(left, ctx);
    double rval = __qmljs_to_number(right, ctx);
    *result = Value::fromDouble(lval * rval);
}

inline void __qmljs_div(ExecutionContext *ctx, Value *result, const Value &left, const Value &right)
{
    TRACE2(left, right);

    double lval = __qmljs_to_number(left, ctx);
    double rval = __qmljs_to_number(right, ctx);
    *result = Value::fromDouble(lval / rval);
}

inline void __qmljs_mod(ExecutionContext *ctx, Value *result, const Value &left, const Value &right)
{
    TRACE2(left, right);

    if (Value::integerCompatible(left, right) && right.integerValue() != 0) {
        *result = Value::fromInt32(left.integerValue() % right.integerValue());
        return;
    }

    double lval = __qmljs_to_number(left, ctx);
    double rval = __qmljs_to_number(right, ctx);
    *result = Value::fromDouble(fmod(lval, rval));
}

inline void __qmljs_shl(ExecutionContext *ctx, Value *result, const Value &left, const Value &right)
{
    TRACE2(left, right);

    if (Value::integerCompatible(left, right)) {
        *result = Value::fromInt32(left.integerValue() << ((uint(right.integerValue()) & 0x1f)));
        return;
    }

    int lval = Value::toInt32(__qmljs_to_number(left, ctx));
    unsigned rval = Value::toUInt32(__qmljs_to_number(right, ctx)) & 0x1f;
    *result = Value::fromInt32(lval << rval);
}

inline void __qmljs_shr(ExecutionContext *ctx, Value *result, const Value &left, const Value &right)
{
    TRACE2(left, right);

    if (Value::integerCompatible(left, right)) {
        *result = Value::fromInt32(left.integerValue() >> ((uint(right.integerValue()) & 0x1f)));
        return;
    }

    int lval = Value::toInt32(__qmljs_to_number(left, ctx));
    unsigned rval = Value::toUInt32(__qmljs_to_number(right, ctx)) & 0x1f;
    *result = Value::fromInt32(lval >> rval);
}

inline void __qmljs_ushr(ExecutionContext *ctx, Value *result, const Value &left, const Value &right)
{
    TRACE2(left, right);

    uint res;
    if (Value::integerCompatible(left, right)) {
        res = uint(left.integerValue()) >> (uint(right.integerValue()) & 0x1f);
    } else {
        unsigned lval = Value::toUInt32(__qmljs_to_number(left, ctx));
        unsigned rval = Value::toUInt32(__qmljs_to_number(right, ctx)) & 0x1f;
        res = lval >> rval;
    }

    if (res > INT_MAX)
        *result = Value::fromDouble(res);
    else
        *result = Value::fromInt32(res);
}

inline void __qmljs_gt(ExecutionContext *ctx, Value *result, const Value &left, const Value &right)
{
    TRACE2(left, right);

    *result = Value::fromBoolean(__qmljs_cmp_gt(ctx, left, right));
}

inline void __qmljs_lt(ExecutionContext *ctx, Value *result, const Value &left, const Value &right)
{
    TRACE2(left, right);

    *result = Value::fromBoolean(__qmljs_cmp_lt(ctx, left, right));
}

inline void __qmljs_ge(ExecutionContext *ctx, Value *result, const Value &left, const Value &right)
{
    TRACE2(left, right);

    *result = Value::fromBoolean(__qmljs_cmp_ge(ctx, left, right));
}

inline void __qmljs_le(ExecutionContext *ctx, Value *result, const Value &left, const Value &right)
{
    TRACE2(left, right);

    *result = Value::fromBoolean(__qmljs_cmp_le(ctx, left, right));
}

inline void __qmljs_eq(ExecutionContext *ctx, Value *result, const Value &left, const Value &right)
{
    TRACE2(left, right);

    *result = Value::fromBoolean(__qmljs_cmp_eq(ctx, left, right));
}

inline void __qmljs_ne(ExecutionContext *ctx, Value *result, const Value &left, const Value &right)
{
    TRACE2(left, right);

    *result = Value::fromBoolean(!__qmljs_cmp_eq(ctx, left, right));
}

inline void __qmljs_se(ExecutionContext *, Value *result, const Value &left, const Value &right)
{
    TRACE2(left, right);

    bool r = __qmljs_strict_equal(left, right);
    *result = Value::fromBoolean(r);
}

inline void __qmljs_sne(ExecutionContext *, Value *result, const Value &left, const Value &right)
{
    TRACE2(left, right);

    bool r = ! __qmljs_strict_equal(left, right);
    *result = Value::fromBoolean(r);
}

inline Bool __qmljs_cmp_gt(ExecutionContext *ctx, const Value &left, const Value &right)
{
    TRACE2(left, right);
    if (Value::integerCompatible(left, right))
        return left.integerValue() > right.integerValue();

    Value l = __qmljs_to_primitive(left, ctx, NUMBER_HINT);
    Value r = __qmljs_to_primitive(right, ctx, NUMBER_HINT);

    if (Value::bothDouble(l, r)) {
        return l.doubleValue() > r.doubleValue();
    } else if (l.isString() && r.isString()) {
        return __qmljs_string_compare(ctx, r.stringValue(), l.stringValue());
    } else {
        double dl = __qmljs_to_number(l, ctx);
        double dr = __qmljs_to_number(r, ctx);
        return dl > dr;
    }
}

inline Bool __qmljs_cmp_lt(ExecutionContext *ctx, const Value &left, const Value &right)
{
    TRACE2(left, right);
    if (Value::integerCompatible(left, right))
        return left.integerValue() < right.integerValue();

    Value l = __qmljs_to_primitive(left, ctx, NUMBER_HINT);
    Value r = __qmljs_to_primitive(right, ctx, NUMBER_HINT);

    if (Value::bothDouble(l, r)) {
        return l.doubleValue() < r.doubleValue();
    } else if (l.isString() && r.isString()) {
        return __qmljs_string_compare(ctx, l.stringValue(), r.stringValue());
    } else {
        double dl = __qmljs_to_number(l, ctx);
        double dr = __qmljs_to_number(r, ctx);
        return dl < dr;
    }
}

inline Bool __qmljs_cmp_ge(ExecutionContext *ctx, const Value &left, const Value &right)
{
    TRACE2(left, right);
    if (Value::integerCompatible(left, right))
        return left.integerValue() >= right.integerValue();

    Value l = __qmljs_to_primitive(left, ctx, NUMBER_HINT);
    Value r = __qmljs_to_primitive(right, ctx, NUMBER_HINT);

    if (Value::bothDouble(l, r)) {
        return l.doubleValue() >= r.doubleValue();
    } else if (l.isString() && r.isString()) {
        return !__qmljs_string_compare(ctx, l.stringValue(), r.stringValue());
    } else {
        double dl = __qmljs_to_number(l, ctx);
        double dr = __qmljs_to_number(r, ctx);
        return dl >= dr;
    }
}

inline Bool __qmljs_cmp_le(ExecutionContext *ctx, const Value &left, const Value &right)
{
    TRACE2(left, right);
    if (Value::integerCompatible(left, right))
        return left.integerValue() <= right.integerValue();

    Value l = __qmljs_to_primitive(left, ctx, NUMBER_HINT);
    Value r = __qmljs_to_primitive(right, ctx, NUMBER_HINT);

    if (Value::bothDouble(l, r)) {
        return l.doubleValue() <= r.doubleValue();
    } else if (l.isString() && r.isString()) {
        return !__qmljs_string_compare(ctx, r.stringValue(), l.stringValue());
    } else {
        double dl = __qmljs_to_number(l, ctx);
        double dr = __qmljs_to_number(r, ctx);
        return dl <= dr;
    }
}

inline Bool __qmljs_cmp_eq(ExecutionContext *ctx, const Value &left, const Value &right)
{
    TRACE2(left, right);

    // need to test for doubles first as NaN != NaN
    if (Value::bothDouble(left, right))
        return left.doubleValue() == right.doubleValue();
    if (left.val == right.val)
        return true;
    if (left.isString() && right.isString())
        return __qmljs_string_equal(left.stringValue(), right.stringValue());

    return __qmljs_equal(left, right, ctx);
}

inline Bool __qmljs_cmp_ne(ExecutionContext *ctx, const Value &left, const Value &right)
{
    TRACE2(left, right);

    return !__qmljs_cmp_eq(ctx, left, right);
}

inline Bool __qmljs_cmp_se(ExecutionContext *, const Value &left, const Value &right)
{
    TRACE2(left, right);

    return __qmljs_strict_equal(left, right);
}

inline Bool __qmljs_cmp_sne(ExecutionContext *, const Value &left, const Value &right)
{
    TRACE2(left, right);

    return ! __qmljs_strict_equal(left, right);
}

inline Bool __qmljs_cmp_instanceof(ExecutionContext *ctx, const Value &left, const Value &right)
{
    TRACE2(left, right);

    Value v;
    __qmljs_instanceof(ctx, &v, left, right);
    return v.booleanValue();
}

inline uint __qmljs_cmp_in(ExecutionContext *ctx, const Value &left, const Value &right)
{
    TRACE2(left, right);

    Value v;
    __qmljs_in(ctx, &v, left, right);
    return v.booleanValue();
}

inline Bool __qmljs_strict_equal(const Value &x, const Value &y)
{
    TRACE2(x, y);

    if (x.isDouble() || y.isDouble())
        return x.asDouble() == y.asDouble();
    if (x.rawValue() == y.rawValue())
        return true;
    if (x.isString() && y.isString())
        return __qmljs_string_equal(x.stringValue(), y.stringValue());
    return false;
}

} // extern "C"

} // namespace VM
} // namespace QQmlJS

QT_END_NAMESPACE

#endif // QMLJS_RUNTIME_H
