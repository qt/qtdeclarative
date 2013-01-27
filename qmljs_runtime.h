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

#include <qmljs_value.h>
#include <qmljs_math.h>

#include <QtCore/QString>
#include <QtCore/qnumeric.h>
#include <QtCore/QDebug>

#include <cmath>
#include <cassert>

#ifdef DO_TRACE_INSTR
#  define TRACE1(x) fprintf(stderr, "    %s\n", __FUNCTION__);
#  define TRACE2(x, y) fprintf(stderr, "    %s\n", __FUNCTION__);
#else
#  define TRACE1(x)
#  define TRACE2(x, y)
#endif // TRACE1

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

extern "C" {

// context
Value __qmljs_call_activation_property(ExecutionContext *, String *name, Value *args, int argc);
Value __qmljs_call_property(ExecutionContext *context, Value that, String *name, Value *args, int argc);
Value __qmljs_call_element(ExecutionContext *context, Value that, Value index, Value *args, int argc);
Value __qmljs_call_value(ExecutionContext *context, Value thisObject, Value func, Value *args, int argc);

Value __qmljs_construct_activation_property(ExecutionContext *, String *name, Value *args, int argc);
Value __qmljs_construct_property(ExecutionContext *context, Value base, String *name, Value *args, int argc);
Value __qmljs_construct_value(ExecutionContext *context, Value func, Value *args, int argc);

Value __qmljs_builtin_typeof(Value val, ExecutionContext *ctx);
Value __qmljs_builtin_typeof_name(String *name, ExecutionContext *context);
Value __qmljs_builtin_typeof_member(Value base, String *name, ExecutionContext *context);
Value __qmljs_builtin_typeof_element(Value base, Value index, ExecutionContext *context);

Value __qmljs_builtin_post_increment(Value *val, ExecutionContext *ctx);
Value __qmljs_builtin_post_increment_name(String *name, ExecutionContext *context);
Value __qmljs_builtin_post_increment_member(Value base, String *name, ExecutionContext *context);
Value __qmljs_builtin_post_increment_element(Value base, Value index, ExecutionContext *context);

Value __qmljs_builtin_post_decrement(Value *val, ExecutionContext *ctx);
Value __qmljs_builtin_post_decrement_name(String *name, ExecutionContext *context);
Value __qmljs_builtin_post_decrement_member(Value base, String *name, ExecutionContext *context);
Value __qmljs_builtin_post_decrement_element(Value base, Value index, ExecutionContext *context);

void __qmljs_builtin_throw(Value val, ExecutionContext *context);
void __qmljs_builtin_rethrow(ExecutionContext *context);
ExecutionContext *__qmljs_builtin_push_with_scope(Value o, ExecutionContext *ctx);
ExecutionContext *__qmljs_builtin_pop_scope(ExecutionContext *ctx);
void __qmljs_builtin_declare_var(ExecutionContext *ctx, bool deletable, String *name);
void __qmljs_builtin_define_property(Value object, String *name, Value val, ExecutionContext *ctx);
void __qmljs_builtin_define_array_property(Value object, int index, Value val, ExecutionContext *ctx);
void __qmljs_builtin_define_getter_setter(Value object, String *name, Value getter, Value setter, ExecutionContext *ctx);

// constructors
Value __qmljs_init_closure(VM::Function *clos, ExecutionContext *ctx);
VM::Function *__qmljs_register_function(ExecutionContext *ctx, String *name,
                                        bool hasDirectEval,
                                        bool usesArgumentsObject, bool isStrict,
                                        bool hasNestedFunctions,
                                        String **formals, unsigned formalCount,
                                        String **locals, unsigned localCount);

Bool __qmljs_is_function(Value value);

// string literals
Value __qmljs_string_literal_undefined(ExecutionContext *ctx);
Value __qmljs_string_literal_null(ExecutionContext *ctx);
Value __qmljs_string_literal_true(ExecutionContext *ctx);
Value __qmljs_string_literal_false(ExecutionContext *ctx);
Value __qmljs_string_literal_object(ExecutionContext *ctx);
Value __qmljs_string_literal_boolean(ExecutionContext *ctx);
Value __qmljs_string_literal_number(ExecutionContext *ctx);
Value __qmljs_string_literal_string(ExecutionContext *ctx);
Value __qmljs_string_literal_function(ExecutionContext *ctx);

// strings
String *__qmljs_string_from_utf8(ExecutionContext *ctx, const char *s);
int __qmljs_string_length(ExecutionContext *ctx, String *string);
double __qmljs_string_to_number(const String *string);
Value __qmljs_string_from_number(ExecutionContext *ctx, double number);
Bool __qmljs_string_compare(ExecutionContext *ctx, String *left, String *right);
Bool __qmljs_string_equal(String *left, String *right);
String *__qmljs_string_concat(ExecutionContext *ctx, String *first, String *second);
String *__qmljs_identifier_from_utf8(ExecutionContext *ctx, const char *s);

// objects
Value __qmljs_object_default_value(ExecutionContext *ctx, Value object, int typeHint);
Value __qmljs_throw_type_error(ExecutionContext *ctx);
Value __qmljs_new_object(ExecutionContext *ctx);
Value __qmljs_new_boolean_object(ExecutionContext *ctx, bool boolean);
Value __qmljs_new_number_object(ExecutionContext *ctx, double n);
Value __qmljs_new_string_object(ExecutionContext *ctx, String *string);
void __qmljs_set_activation_property(ExecutionContext *ctx, String *name, Value value);
void __qmljs_set_property(ExecutionContext *ctx, Value object, String *name, Value value);
Value __qmljs_get_property(ExecutionContext *ctx, Value object, String *name);
Value __qmljs_get_activation_property(ExecutionContext *ctx, String *name);

Value __qmljs_get_element(ExecutionContext *ctx, Value object, Value index);
void __qmljs_set_element(ExecutionContext *ctx, Value object, Value index, Value value);

// For each
Value __qmljs_foreach_iterator_object(Value in, ExecutionContext *ctx);
Value __qmljs_foreach_next_property_name(Value foreach_iterator);

// context
Value __qmljs_get_thisObject(ExecutionContext *ctx);

// type conversion and testing
Value __qmljs_to_primitive(Value value, ExecutionContext *ctx, int typeHint);
Bool __qmljs_to_boolean(Value value, ExecutionContext *ctx);
double __qmljs_to_number(Value value, ExecutionContext *ctx);
double __qmljs_to_integer(Value value, ExecutionContext *ctx);
int __qmljs_to_int32(Value value, ExecutionContext *ctx);
unsigned short __qmljs_to_uint16(Value value, ExecutionContext *ctx);
Value __qmljs_to_string(Value value, ExecutionContext *ctx);
Value __qmljs_to_object(Value value, ExecutionContext *ctx);
//uint __qmljs_check_object_coercible(Context *ctx, Value *result, Value *value);
Bool __qmljs_is_callable(Value value, ExecutionContext *ctx);
Value __qmljs_default_value(Value value, ExecutionContext *ctx, int typeHint);

Bool __qmljs_equal(Value x, Value y, ExecutionContext *ctx);
Bool __qmljs_strict_equal(Value x, Value y);

// unary operators
Value __qmljs_uplus(Value value, ExecutionContext *ctx);
Value __qmljs_uminus(Value value, ExecutionContext *ctx);
Value __qmljs_compl(Value value, ExecutionContext *ctx);
Value __qmljs_not(Value value, ExecutionContext *ctx);
Value __qmljs_increment(Value value, ExecutionContext *ctx);
Value __qmljs_decrement(Value value, ExecutionContext *ctx);

Value __qmljs_delete_subscript(ExecutionContext *ctx, Value base, Value index);
Value __qmljs_delete_member(ExecutionContext *ctx, Value base, String *name);
Value __qmljs_delete_name(ExecutionContext *ctx, String *name);

void __qmljs_throw(Value value, ExecutionContext *context);
// actually returns a jmp_buf *
void *__qmljs_create_exception_handler(ExecutionContext *context);
void __qmljs_delete_exception_handler(ExecutionContext *context);
Value __qmljs_get_exception(ExecutionContext *context);

// binary operators
typedef Value (*BinOp)(Value left, Value right, ExecutionContext *ctx);

Value __qmljs_instanceof(Value left, Value right, ExecutionContext *ctx);
Value __qmljs_in(Value left, Value right, ExecutionContext *ctx);
Value __qmljs_bit_or(Value left, Value right, ExecutionContext *ctx);
Value __qmljs_bit_xor(Value left, Value right, ExecutionContext *ctx);
Value __qmljs_bit_and(Value left, Value right, ExecutionContext *ctx);
Value __qmljs_add(Value left, Value right, ExecutionContext *ctx);
Value __qmljs_sub(Value left, Value right, ExecutionContext *ctx);
Value __qmljs_mul(Value left, Value right, ExecutionContext *ctx);
Value __qmljs_div(Value left, Value right, ExecutionContext *ctx);
Value __qmljs_mod(Value left, Value right, ExecutionContext *ctx);
Value __qmljs_shl(Value left, Value right, ExecutionContext *ctx);
Value __qmljs_shr(Value left, Value right, ExecutionContext *ctx);
Value __qmljs_ushr(Value left, Value right, ExecutionContext *ctx);
Value __qmljs_gt(Value left, Value right, ExecutionContext *ctx);
Value __qmljs_lt(Value left, Value right, ExecutionContext *ctx);
Value __qmljs_ge(Value left, Value right, ExecutionContext *ctx);
Value __qmljs_le(Value left, Value right, ExecutionContext *ctx);
Value __qmljs_eq(Value left, Value right, ExecutionContext *ctx);
Value __qmljs_ne(Value left, Value right, ExecutionContext *ctx);
Value __qmljs_se(Value left, Value right, ExecutionContext *ctx);
Value __qmljs_sne(Value left, Value right, ExecutionContext *ctx);

Value __qmljs_add_helper(Value left, Value right, ExecutionContext *ctx);

void __qmljs_inplace_bit_and_name(Value value, String *name, ExecutionContext *ctx);
void __qmljs_inplace_bit_or_name(Value value, String *name, ExecutionContext *ctx);
void __qmljs_inplace_bit_xor_name(Value value, String *name, ExecutionContext *ctx);
void __qmljs_inplace_add_name(Value value, String *name, ExecutionContext *ctx);
void __qmljs_inplace_sub_name(Value value, String *name, ExecutionContext *ctx);
void __qmljs_inplace_mul_name(Value value, String *name, ExecutionContext *ctx);
void __qmljs_inplace_div_name(Value value, String *name, ExecutionContext *ctx);
void __qmljs_inplace_mod_name(Value value, String *name, ExecutionContext *ctx);
void __qmljs_inplace_shl_name(Value value, String *name, ExecutionContext *ctx);
void __qmljs_inplace_shr_name(Value value, String *name, ExecutionContext *ctx);
void __qmljs_inplace_ushr_name(Value value, String *name, ExecutionContext *ctx);

void __qmljs_inplace_bit_and_element(Value base, Value index, Value value, ExecutionContext *ctx);
void __qmljs_inplace_bit_or_element(Value base, Value index, Value value, ExecutionContext *ctx);
void __qmljs_inplace_bit_xor_element(Value base, Value index, Value value, ExecutionContext *ctx);
void __qmljs_inplace_add_element(Value base, Value index, Value value, ExecutionContext *ctx);
void __qmljs_inplace_sub_element(Value base, Value index, Value value, ExecutionContext *ctx);
void __qmljs_inplace_mul_element(Value base, Value index, Value value, ExecutionContext *ctx);
void __qmljs_inplace_div_element(Value base, Value index, Value value, ExecutionContext *ctx);
void __qmljs_inplace_mod_element(Value base, Value index, Value value, ExecutionContext *ctx);
void __qmljs_inplace_shl_element(Value base, Value index, Value value, ExecutionContext *ctx);
void __qmljs_inplace_shr_element(Value base, Value index, Value value, ExecutionContext *ctx);
void __qmljs_inplace_ushr_element(Value base, Value index, Value value, ExecutionContext *ctx);

void __qmljs_inplace_bit_and_member(Value value, Value base, String *name, ExecutionContext *ctx);
void __qmljs_inplace_bit_or_member(Value value, Value base, String *name, ExecutionContext *ctx);
void __qmljs_inplace_bit_xor_member(Value value, Value base, String *name, ExecutionContext *ctx);
void __qmljs_inplace_add_member(Value value, Value base, String *name, ExecutionContext *ctx);
void __qmljs_inplace_sub_member(Value value, Value base, String *name, ExecutionContext *ctx);
void __qmljs_inplace_mul_member(Value value, Value base, String *name, ExecutionContext *ctx);
void __qmljs_inplace_div_member(Value value, Value base, String *name, ExecutionContext *ctx);
void __qmljs_inplace_mod_member(Value value, Value base, String *name, ExecutionContext *ctx);
void __qmljs_inplace_shl_member(Value value, Value base, String *name, ExecutionContext *ctx);
void __qmljs_inplace_shr_member(Value value, Value base, String *name, ExecutionContext *ctx);
void __qmljs_inplace_ushr_member(Value value, Value base, String *name, ExecutionContext *ctx);

Bool __qmljs_cmp_gt(Value left, Value right, ExecutionContext *ctx);
Bool __qmljs_cmp_lt(Value left, Value right, ExecutionContext *ctx);
Bool __qmljs_cmp_ge(Value left, Value right, ExecutionContext *ctx);
Bool __qmljs_cmp_le(Value left, Value right, ExecutionContext *ctx);
Bool __qmljs_cmp_eq(Value left, Value right, ExecutionContext *ctx);
Bool __qmljs_cmp_ne(Value left, Value right, ExecutionContext *ctx);
Bool __qmljs_cmp_se(Value left, Value right, ExecutionContext *ctx);
Bool __qmljs_cmp_sne(Value left, Value right, ExecutionContext *ctx);
Bool __qmljs_cmp_instanceof(Value left, Value right, ExecutionContext *ctx);
Bool __qmljs_cmp_in(Value left, Value right, ExecutionContext *ctx);

// type conversion and testing
inline Value __qmljs_to_primitive(Value value, ExecutionContext *ctx, int typeHint)
{
    if (!value.isObject())
        return value;
    return __qmljs_default_value(value, ctx, typeHint);
}

inline Bool __qmljs_to_boolean(Value value, ExecutionContext *ctx)
{
    switch (value.type()) {
    case Value::Undefined_Type:
    case Value::Null_Type:
        return false;
    case Value::Boolean_Type:
    case Value::Integer_Type:
        return (bool)value.int_32;
    case Value::String_Type:
        return __qmljs_string_length(ctx, value.stringValue()) > 0;
    case Value::Object_Type:
        return true;
    default: // double
        if (! value.doubleValue() || std::isnan(value.doubleValue()))
            return false;
        return true;
    }
}

inline double __qmljs_to_number(Value value, ExecutionContext *ctx)
{
    switch (value.type()) {
    case Value::Undefined_Type:
        return nan("");
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

inline double __qmljs_to_integer(Value value, ExecutionContext *ctx)
{
    if (value.isConvertibleToInt())
        return value.int_32;

    return Value::toInteger(__qmljs_to_number(value, ctx));
}

inline int __qmljs_to_int32(Value value, ExecutionContext *ctx)
{
    if (value.isConvertibleToInt())
        return value.int_32;

    return Value::toInt32(__qmljs_to_number(value, ctx));
}

inline unsigned short __qmljs_to_uint16(Value value, ExecutionContext *ctx)
{
    if (value.isConvertibleToInt())
        return (ushort)(uint)value.integerValue();

    double number = __qmljs_to_number(value, ctx);

    double D16 = 65536.0;
    if ((number >= 0 && number < D16))
        return static_cast<ushort>(number);

    if (!std::isfinite(number))
        return +0;

    double d = ::floor(::fabs(number));
    if (std::signbit(number))
        d = -d;

    number = ::fmod(d , D16);

    if (number < 0)
        number += D16;

    return (unsigned short)number;
}

inline Value __qmljs_to_string(Value value, ExecutionContext *ctx)
{
    switch (value.type()) {
    case Value::Undefined_Type:
        return __qmljs_string_literal_undefined(ctx);
        break;
    case Value::Null_Type:
        return __qmljs_string_literal_null(ctx);
        break;
    case Value::Boolean_Type:
        if (value.booleanValue())
            return __qmljs_string_literal_true(ctx);
        else
            return __qmljs_string_literal_false(ctx);
        break;
    case Value::String_Type:
        return value;
        break;
    case Value::Object_Type: {
        Value prim = __qmljs_to_primitive(value, ctx, STRING_HINT);
        if (prim.isPrimitive())
            return __qmljs_to_string(prim, ctx);
        else
            return __qmljs_throw_type_error(ctx);
        break;
    }
    case Value::Integer_Type:
        return __qmljs_string_from_number(ctx, value.int_32);
        break;
    default: // double
        return __qmljs_string_from_number(ctx, value.doubleValue());
        break;

    } // switch
}

inline Value __qmljs_to_object(Value value, ExecutionContext *ctx)
{
    switch (value.type()) {
    case Value::Undefined_Type:
    case Value::Null_Type:
        return __qmljs_throw_type_error(ctx);
        break;
    case Value::Boolean_Type:
        return __qmljs_new_boolean_object(ctx, value.booleanValue());
        break;
    case Value::String_Type:
        return __qmljs_new_string_object(ctx, value.stringValue());
        break;
    case Value::Object_Type:
        return value;
        break;
    case Value::Integer_Type:
        return __qmljs_new_number_object(ctx, value.int_32);
        break;
    default: // double
        return __qmljs_new_number_object(ctx, value.doubleValue());
        break;
    }
}

/*
inline uint __qmljs_check_object_coercible(Context *ctx, Value *result, Value *value)
{
    switch (value->type()) {
    case Value::Undefined_Type:
    case Value::Null_Type:
        *result = __qmljs_throw_type_error(ctx);
        return false;
    default:
        return true;
    }
}
*/

inline Bool __qmljs_is_callable(Value value, ExecutionContext * /*ctx*/)
{
    if (value.isObject())
        return __qmljs_is_function(value);
    else
        return false;
}

inline Value __qmljs_default_value(Value value, ExecutionContext *ctx, int typeHint)
{
    if (value.isObject())
        return __qmljs_object_default_value(ctx, value, typeHint);
    return Value::undefinedValue();
}


inline Value __qmljs_uplus(Value value, ExecutionContext *ctx)
{
    TRACE1(value);

    if (value.tryIntegerConversion())
        return value;

    double n = __qmljs_to_number(value, ctx);
    return Value::fromDouble(n);
}

inline Value __qmljs_uminus(Value value, ExecutionContext *ctx)
{
    TRACE1(value);

    // +0 != -0, so we need to convert to double when negating 0
    if (value.isInteger() && value.integerValue())
        return Value::fromInt32(-value.integerValue());
    double n = __qmljs_to_number(value, ctx);
    return Value::fromDouble(-n);
}

inline Value __qmljs_compl(Value value, ExecutionContext *ctx)
{
    TRACE1(value);

    int n;
    if (value.isConvertibleToInt())
        n = value.int_32;
    else
        n = Value::toInt32(__qmljs_to_number(value, ctx));

    return Value::fromInt32(~n);
}

inline Value __qmljs_not(Value value, ExecutionContext *ctx)
{
    TRACE1(value);

    bool b = __qmljs_to_boolean(value, ctx);
    return Value::fromBoolean(!b);
}

// binary operators
inline Value __qmljs_bit_or(Value left, Value right, ExecutionContext *ctx)
{
    TRACE2(left, right);

    if (Value::integerCompatible(left, right))
        return Value::fromInt32(left.integerValue() | right.integerValue());

    int lval = Value::toInt32(__qmljs_to_number(left, ctx));
    int rval = Value::toInt32(__qmljs_to_number(right, ctx));
    return Value::fromInt32(lval | rval);
}

inline Value __qmljs_bit_xor(Value left, Value right, ExecutionContext *ctx)
{
    TRACE2(left, right);

    if (Value::integerCompatible(left, right))
        return Value::fromInt32(left.integerValue() ^ right.integerValue());

    int lval = Value::toInt32(__qmljs_to_number(left, ctx));
    int rval = Value::toInt32(__qmljs_to_number(right, ctx));
    return Value::fromInt32(lval ^ rval);
}

inline Value __qmljs_bit_and(Value left, Value right, ExecutionContext *ctx)
{
    TRACE2(left, right);

    if (Value::integerCompatible(left, right))
        return Value::fromInt32(left.integerValue() & right.integerValue());

    int lval = Value::toInt32(__qmljs_to_number(left, ctx));
    int rval = Value::toInt32(__qmljs_to_number(right, ctx));
    return Value::fromInt32(lval & rval);
}

inline Value __qmljs_add(Value left, Value right, ExecutionContext *ctx)
{
    TRACE2(left, right);

#ifndef QMLJS_LLVM_RUNTIME
    if (Value::integerCompatible(left, right))
        return add_int32(left.integerValue(), right.integerValue());
#endif // QMLJS_LLVM_RUNTIME

    if (Value::bothDouble(left, right))
        return Value::fromDouble(left.doubleValue() + right.doubleValue());

    return __qmljs_add_helper(left, right, ctx);
}

inline Value __qmljs_sub(Value left, Value right, ExecutionContext *ctx)
{
    TRACE2(left, right);

#ifndef QMLJS_LLVM_RUNTIME
    if (Value::integerCompatible(left, right))
        return sub_int32(left.integerValue(), right.integerValue());
#endif // QMLJS_LLVM_RUNTIME

    double lval = __qmljs_to_number(left, ctx);
    double rval = __qmljs_to_number(right, ctx);
    return Value::fromDouble(lval - rval);
}

inline Value __qmljs_mul(Value left, Value right, ExecutionContext *ctx)
{
    TRACE2(left, right);

#ifndef QMLJS_LLVM_RUNTIME
    if (Value::integerCompatible(left, right))
        return mul_int32(left.integerValue(), right.integerValue());
#endif // QMLJS_LLVM_RUNTIME

    double lval = __qmljs_to_number(left, ctx);
    double rval = __qmljs_to_number(right, ctx);
    return Value::fromDouble(lval * rval);
}

inline Value __qmljs_div(Value left, Value right, ExecutionContext *ctx)
{
    TRACE2(left, right);

    double lval = __qmljs_to_number(left, ctx);
    double rval = __qmljs_to_number(right, ctx);
    return Value::fromDouble(lval / rval);
}

inline Value __qmljs_mod(Value left, Value right, ExecutionContext *ctx)
{
    TRACE2(left, right);

    if (Value::integerCompatible(left, right) && right.integerValue() != 0)
        return Value::fromInt32(left.integerValue() % right.integerValue());

    double lval = __qmljs_to_number(left, ctx);
    double rval = __qmljs_to_number(right, ctx);
    return Value::fromDouble(fmod(lval, rval));
}

inline Value __qmljs_shl(Value left, Value right, ExecutionContext *ctx)
{
    TRACE2(left, right);

    if (Value::integerCompatible(left, right))
        return Value::fromInt32(left.integerValue() << ((uint(right.integerValue()) & 0x1f)));

    int lval = Value::toInt32(__qmljs_to_number(left, ctx));
    unsigned rval = Value::toUInt32(__qmljs_to_number(right, ctx)) & 0x1f;
    return Value::fromInt32(lval << rval);
}

inline Value __qmljs_shr(Value left, Value right, ExecutionContext *ctx)
{
    TRACE2(left, right);

    if (Value::integerCompatible(left, right))
        return Value::fromInt32(left.integerValue() >> ((uint(right.integerValue()) & 0x1f)));

    int lval = Value::toInt32(__qmljs_to_number(left, ctx));
    unsigned rval = Value::toUInt32(__qmljs_to_number(right, ctx)) & 0x1f;
    return Value::fromInt32(lval >> rval);
}

inline Value __qmljs_ushr(Value left, Value right, ExecutionContext *ctx)
{
    TRACE2(left, right);

    uint result;
    if (Value::integerCompatible(left, right)) {
        result = uint(left.integerValue()) >> (uint(right.integerValue()) & 0x1f);
    } else {
        unsigned lval = Value::toUInt32(__qmljs_to_number(left, ctx));
        unsigned rval = Value::toUInt32(__qmljs_to_number(right, ctx)) & 0x1f;
        result = lval >> rval;
    }

    if (result > INT_MAX)
        return Value::fromDouble(result);
    return Value::fromInt32(result);
}

inline Value __qmljs_gt(Value left, Value right, ExecutionContext *ctx)
{
    TRACE2(left, right);

    return Value::fromBoolean(__qmljs_cmp_gt(left, right, ctx));
}

inline Value __qmljs_lt(Value left, Value right, ExecutionContext *ctx)
{
    TRACE2(left, right);

    return Value::fromBoolean(__qmljs_cmp_lt(left, right, ctx));
}

inline Value __qmljs_ge(Value left, Value right, ExecutionContext *ctx)
{
    TRACE2(left, right);

    return Value::fromBoolean(__qmljs_cmp_ge(left, right, ctx));
}

inline Value __qmljs_le(Value left, Value right, ExecutionContext *ctx)
{
    TRACE2(left, right);

    return Value::fromBoolean(__qmljs_cmp_le(left, right, ctx));
}

inline Value __qmljs_eq(Value left, Value right, ExecutionContext *ctx)
{
    TRACE2(left, right);

    return Value::fromBoolean(__qmljs_cmp_eq(left, right, ctx));
}

inline Value __qmljs_ne(Value left, Value right, ExecutionContext *ctx)
{
    TRACE2(left, right);

    return Value::fromBoolean(!__qmljs_cmp_eq(left, right, ctx));
}

inline Value __qmljs_se(Value left, Value right, ExecutionContext *)
{
    TRACE2(left, right);

    bool r = __qmljs_strict_equal(left, right);
    return Value::fromBoolean(r);
}

inline Value __qmljs_sne(Value left, Value right, ExecutionContext *)
{
    TRACE2(left, right);

    bool r = ! __qmljs_strict_equal(left, right);
    return Value::fromBoolean(r);
}

inline Bool __qmljs_cmp_gt(Value left, Value right, ExecutionContext *ctx)
{
    TRACE2(left, right);

    left = __qmljs_to_primitive(left, ctx, NUMBER_HINT);
    right = __qmljs_to_primitive(right, ctx, NUMBER_HINT);

    if (Value::integerCompatible(left, right))
        return left.integerValue() > right.integerValue();
    if (Value::bothDouble(left, right)) {
        return left.doubleValue() > right.doubleValue();
    } else if (left.isString() && right.isString()) {
        return __qmljs_string_compare(ctx, right.stringValue(), left.stringValue());
    } else {
        double l = __qmljs_to_number(left, ctx);
        double r = __qmljs_to_number(right, ctx);
        return l > r;
    }
}

inline Bool __qmljs_cmp_lt(Value left, Value right, ExecutionContext *ctx)
{
    TRACE2(left, right);

    left = __qmljs_to_primitive(left, ctx, NUMBER_HINT);
    right = __qmljs_to_primitive(right, ctx, NUMBER_HINT);

    if (Value::integerCompatible(left, right))
        return left.integerValue() < right.integerValue();
    if (Value::bothDouble(left, right)) {
        return left.doubleValue() < right.doubleValue();
    } else if (left.isString() && right.isString()) {
        return __qmljs_string_compare(ctx, left.stringValue(), right.stringValue());
    } else {
        double l = __qmljs_to_number(left, ctx);
        double r = __qmljs_to_number(right, ctx);
        return l < r;
    }
}

inline Bool __qmljs_cmp_ge(Value left, Value right, ExecutionContext *ctx)
{
    TRACE2(left, right);

    left = __qmljs_to_primitive(left, ctx, NUMBER_HINT);
    right = __qmljs_to_primitive(right, ctx, NUMBER_HINT);

    if (Value::integerCompatible(left, right))
        return left.integerValue() >= right.integerValue();
    if (Value::bothDouble(left, right)) {
        return left.doubleValue() >= right.doubleValue();
    } else if (left.isString() && right.isString()) {
        return !__qmljs_string_compare(ctx, left.stringValue(), right.stringValue());
    } else {
        double l = __qmljs_to_number(left, ctx);
        double r = __qmljs_to_number(right, ctx);
        return l >= r;
    }
}

inline Bool __qmljs_cmp_le(Value left, Value right, ExecutionContext *ctx)
{
    TRACE2(left, right);

    left = __qmljs_to_primitive(left, ctx, NUMBER_HINT);
    right = __qmljs_to_primitive(right, ctx, NUMBER_HINT);

    if (Value::integerCompatible(left, right))
        return left.integerValue() <= right.integerValue();
    if (Value::bothDouble(left, right)) {
        return left.doubleValue() <= right.doubleValue();
    } else if (left.isString() && right.isString()) {
        return !__qmljs_string_compare(ctx, right.stringValue(), left.stringValue());
    } else {
        double l = __qmljs_to_number(left, ctx);
        double r = __qmljs_to_number(right, ctx);
        return l <= r;
    }
}

inline Bool __qmljs_cmp_eq(Value left, Value right, ExecutionContext *ctx)
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

inline Bool __qmljs_cmp_ne(Value left, Value right, ExecutionContext *ctx)
{
    TRACE2(left, right);

    return !__qmljs_cmp_eq(left, right, ctx);
}

inline Bool __qmljs_cmp_se(Value left, Value right, ExecutionContext *)
{
    TRACE2(left, right);

    return __qmljs_strict_equal(left, right);
}

inline Bool __qmljs_cmp_sne(Value left, Value right, ExecutionContext *)
{
    TRACE2(left, right);

    return ! __qmljs_strict_equal(left, right);
}

inline Bool __qmljs_cmp_instanceof(Value left, Value right, ExecutionContext *ctx)
{
    TRACE2(left, right);

    Value v = __qmljs_instanceof(left, right, ctx);
    return v.booleanValue();
}

inline uint __qmljs_cmp_in(Value left, Value right, ExecutionContext *ctx)
{
    TRACE2(left, right);

    Value v = __qmljs_in(left, right, ctx);
    return v.booleanValue();
}

inline Bool __qmljs_strict_equal(Value x, Value y)
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

#endif // QMLJS_RUNTIME_H
