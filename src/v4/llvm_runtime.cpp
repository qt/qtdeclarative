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

#include "qmljs_runtime.h"
#include "qmljs_environment.h"
#include <stdio.h>
#include <setjmp.h>

using namespace QQmlJS::VM;

extern "C" {

Value __qmljs_llvm_return(ExecutionContext */*ctx*/, Value *result)
{
    return *result;
}

Value *__qmljs_llvm_get_argument(ExecutionContext *ctx, int index)
{
    return &ctx->arguments[index];
}

void __qmljs_llvm_init_undefined(Value *result)
{
    *result = Value::undefinedValue();
}

void __qmljs_llvm_init_null(Value *result)
{
    *result = Value::nullValue();
}

void __qmljs_llvm_init_boolean(Value *result, bool value)
{
    *result = Value::fromBoolean(value);
}

void __qmljs_llvm_init_number(Value *result, double value)
{
    *result = Value::fromDouble(value);
}

void __qmljs_llvm_init_string(ExecutionContext *ctx, Value *result, const char *str)
{
    *result = Value::fromString(__qmljs_string_from_utf8(ctx, str));
}

void __qmljs_llvm_init_closure(ExecutionContext *ctx, Value *result,
                               String *name, bool hasDirectEval,
                               bool usesArgumentsObject, bool isStrict,
                               bool hasNestedFunctions,
                               String **formals, unsigned formalCount,
                               String **locals, unsigned localCount)
{
    Function *clos = __qmljs_register_function(ctx, name, hasDirectEval,
                                               usesArgumentsObject, isStrict,
                                               hasNestedFunctions,
                                               formals, formalCount,
                                               locals, localCount);
    *result = __qmljs_init_closure(clos, ctx);
}

bool __qmljs_llvm_to_boolean(ExecutionContext *ctx, const Value *value)
{
    return __qmljs_to_boolean(*value, ctx);
}

void __qmljs_llvm_bit_and(ExecutionContext *ctx, Value *result, Value *left, Value *right)
{
    *result = __qmljs_bit_and(*left, *right, ctx);
}

void __qmljs_llvm_bit_or(ExecutionContext *ctx, Value *result, Value *left, Value *right)
{
    *result = __qmljs_bit_or(*left, *right, ctx);
}

void __qmljs_llvm_bit_xor(ExecutionContext *ctx, Value *result, Value *left, Value *right)
{
    *result = __qmljs_bit_xor(*left, *right, ctx);
}

void __qmljs_llvm_add(ExecutionContext *ctx, Value *result, Value *left, Value *right)
{
    *result = __qmljs_add(*left, *right, ctx);
}

void __qmljs_llvm_sub(ExecutionContext *ctx, Value *result, Value *left, Value *right)
{
    *result = __qmljs_sub(*left, *right, ctx);
}

void __qmljs_llvm_mul(ExecutionContext *ctx, Value *result, Value *left, Value *right)
{
    *result = __qmljs_mul(*left, *right, ctx);
}

void __qmljs_llvm_div(ExecutionContext *ctx, Value *result, Value *left, Value *right)
{
    *result = __qmljs_div(*left, *right, ctx);
}

void __qmljs_llvm_mod(ExecutionContext *ctx, Value *result, Value *left, Value *right)
{
    *result = __qmljs_mod(*left, *right, ctx);
}

void __qmljs_llvm_shl(ExecutionContext *ctx, Value *result, Value *left, Value *right)
{
    *result = __qmljs_shl(*left, *right, ctx);
}

void __qmljs_llvm_shr(ExecutionContext *ctx, Value *result, Value *left, Value *right)
{
    *result = __qmljs_shr(*left, *right, ctx);
}

void __qmljs_llvm_ushr(ExecutionContext *ctx, Value *result, Value *left, Value *right)
{
    *result = __qmljs_ushr(*left, *right, ctx);
}

void __qmljs_llvm_gt(ExecutionContext *ctx, Value *result, Value *left, Value *right)
{
    *result = __qmljs_gt(*left, *right, ctx);
}

void __qmljs_llvm_lt(ExecutionContext *ctx, Value *result, Value *left, Value *right)
{
    *result = __qmljs_lt(*left, *right, ctx);
}

void __qmljs_llvm_ge(ExecutionContext *ctx, Value *result, Value *left, Value *right)
{
    *result = __qmljs_ge(*left, *right, ctx);
}

void __qmljs_llvm_le(ExecutionContext *ctx, Value *result, Value *left, Value *right)
{
    *result = __qmljs_le(*left, *right, ctx);
}

void __qmljs_llvm_eq(ExecutionContext *ctx, Value *result, Value *left, Value *right)
{
    *result = __qmljs_eq(*left, *right, ctx);
}

void __qmljs_llvm_ne(ExecutionContext *ctx, Value *result, Value *left, Value *right)
{
    *result = __qmljs_ne(*left, *right, ctx);
}

void __qmljs_llvm_se(ExecutionContext *ctx, Value *result, Value *left, Value *right)
{
    *result = __qmljs_se(*left, *right, ctx);
}

void __qmljs_llvm_sne(ExecutionContext *ctx, Value *result, Value *left, Value *right)
{
    *result = __qmljs_sne(*left, *right, ctx);
}

void __qmljs_llvm_instanceof(ExecutionContext *ctx, Value *result, Value *left, Value *right)
{
    *result = __qmljs_instanceof(*left, *right, ctx);
}

void __qmljs_llvm_in(ExecutionContext *ctx, Value *result, Value *left, Value *right)
{
    *result = __qmljs_in(*left, *right, ctx);
}

void __qmljs_llvm_uplus(ExecutionContext *ctx, Value *result, const Value *value)
{
    *result = __qmljs_uplus(*value, ctx);
}

void __qmljs_llvm_uminus(ExecutionContext *ctx, Value *result, const Value *value)
{
    *result = __qmljs_uminus(*value, ctx);
}

void __qmljs_llvm_compl(ExecutionContext *ctx, Value *result, const Value *value)
{
    *result = __qmljs_compl(*value, ctx);
}

void __qmljs_llvm_not(ExecutionContext *ctx, Value *result, const Value *value)
{
    *result = __qmljs_not(*value, ctx);
}

void __qmljs_llvm_inplace_bit_and_name(ExecutionContext *ctx, String *dest, Value *src)
{
    __qmljs_inplace_bit_and_name(*src, dest, ctx);
}

void __qmljs_llvm_inplace_bit_or_name(ExecutionContext *ctx, String *dest, Value *src)
{
    __qmljs_inplace_bit_or_name(*src, dest, ctx);
}

void __qmljs_llvm_inplace_bit_xor_name(ExecutionContext *ctx, String *dest, Value *src)
{
    __qmljs_inplace_bit_xor_name(*src, dest, ctx);
}

void __qmljs_llvm_inplace_add_name(ExecutionContext *ctx, String *dest, Value *src)
{
    __qmljs_inplace_add_name(*src, dest, ctx);
}

void __qmljs_llvm_inplace_sub_name(ExecutionContext *ctx, String *dest, Value *src)
{
    __qmljs_inplace_sub_name(*src, dest, ctx);
}

void __qmljs_llvm_inplace_mul_name(ExecutionContext *ctx, String *dest, Value *src)
{
    __qmljs_inplace_mul_name(*src, dest, ctx);
}

void __qmljs_llvm_inplace_div_name(ExecutionContext *ctx, String *dest, Value *src)
{
    __qmljs_inplace_div_name(*src, dest, ctx);
}

void __qmljs_llvm_inplace_mod_name(ExecutionContext *ctx, String *dest, Value *src)
{
    __qmljs_inplace_mod_name(*src, dest, ctx);
}

void __qmljs_llvm_inplace_shl_name(ExecutionContext *ctx, String *dest, Value *src)
{
    __qmljs_inplace_shl_name(*src, dest, ctx);
}

void __qmljs_llvm_inplace_shr_name(ExecutionContext *ctx, String *dest, Value *src)
{
    __qmljs_inplace_shr_name(*src, dest, ctx);
}

void __qmljs_llvm_inplace_ushr_name(ExecutionContext *ctx, String *dest, Value *src)
{
    __qmljs_inplace_ushr_name(*src, dest, ctx);
}

void __qmljs_llvm_inplace_bit_and_element(ExecutionContext *ctx, Value *base, Value *index, Value *value)
{
    __qmljs_inplace_bit_and_element(*base, *index, *value, ctx);
}

void __qmljs_llvm_inplace_bit_or_element(ExecutionContext *ctx, Value *base, Value *index, Value *value)
{
    __qmljs_inplace_bit_or_element(*base, *index, *value, ctx);
}

void __qmljs_llvm_inplace_bit_xor_element(ExecutionContext *ctx, Value *base, Value *index, Value *value)
{
    __qmljs_inplace_bit_xor_element(*base, *index, *value, ctx);
}

void __qmljs_llvm_inplace_add_element(ExecutionContext *ctx, Value *base, Value *index, Value *value)
{
    __qmljs_inplace_add_element(*base, *index, *value, ctx);
}

void __qmljs_llvm_inplace_sub_element(ExecutionContext *ctx, Value *base, Value *index, Value *value)
{
    __qmljs_inplace_sub_element(*base, *index, *value, ctx);
}

void __qmljs_llvm_inplace_mul_element(ExecutionContext *ctx, Value *base, Value *index, Value *value)
{
    __qmljs_inplace_mul_element(*base, *index, *value, ctx);
}

void __qmljs_llvm_inplace_div_element(ExecutionContext *ctx, Value *base, Value *index, Value *value)
{
    __qmljs_inplace_div_element(*base, *index, *value, ctx);
}

void __qmljs_llvm_inplace_mod_element(ExecutionContext *ctx, Value *base, Value *index, Value *value)
{
    __qmljs_inplace_mod_element(*base, *index, *value, ctx);
}

void __qmljs_llvm_inplace_shl_element(ExecutionContext *ctx, Value *base, Value *index, Value *value)
{
    __qmljs_inplace_shl_element(*base, *index, *value, ctx);
}

void __qmljs_llvm_inplace_shr_element(ExecutionContext *ctx, Value *base, Value *index, Value *value)
{
    __qmljs_inplace_shr_element(*base, *index, *value, ctx);
}

void __qmljs_llvm_inplace_ushr_element(ExecutionContext *ctx, Value *base, Value *index, Value *value)
{
    __qmljs_inplace_ushr_element(*base, *index, *value, ctx);
}

void __qmljs_llvm_inplace_bit_and_member(ExecutionContext *ctx, Value *value, Value *base, String *member)
{
    __qmljs_inplace_bit_and_member(*value, *base, member, ctx);
}

void __qmljs_llvm_inplace_bit_or_member(ExecutionContext *ctx, Value *value, Value *base, String *member)
{
    __qmljs_inplace_bit_or_member(*value, *base, member, ctx);
}

void __qmljs_llvm_inplace_bit_xor_member(ExecutionContext *ctx, Value *value, Value *base, String *member)
{
    __qmljs_inplace_bit_xor_member(*value, *base, member, ctx);
}

void __qmljs_llvm_inplace_add_member(ExecutionContext *ctx, Value *value, Value *base, String *member)
{
    __qmljs_inplace_add_member(*value, *base, member, ctx);
}

void __qmljs_llvm_inplace_sub_member(ExecutionContext *ctx, Value *value, Value *base, String *member)
{
    __qmljs_inplace_sub_member(*value, *base, member, ctx);
}

void __qmljs_llvm_inplace_mul_member(ExecutionContext *ctx, Value *value, Value *base, String *member)
{
    __qmljs_inplace_mul_member(*value, *base, member, ctx);
}

void __qmljs_llvm_inplace_div_member(ExecutionContext *ctx, Value *value, Value *base, String *member)
{
    __qmljs_inplace_div_member(*value, *base, member, ctx);
}

void __qmljs_llvm_inplace_mod_member(ExecutionContext *ctx, Value *value, Value *base, String *member)
{
    __qmljs_inplace_mod_member(*value, *base, member, ctx);
}

void __qmljs_llvm_inplace_shl_member(ExecutionContext *ctx, Value *value, Value *base, String *member)
{
    __qmljs_inplace_shl_member(*value, *base, member, ctx);
}

void __qmljs_llvm_inplace_shr_member(ExecutionContext *ctx, Value *value, Value *base, String *member)
{
    __qmljs_inplace_shr_member(*value, *base, member, ctx);
}

void __qmljs_llvm_inplace_ushr_member(ExecutionContext *ctx, Value *value, Value *base, String *member)
{
    __qmljs_inplace_ushr_member(*value, *base, member, ctx);
}

String *__qmljs_llvm_identifier_from_utf8(ExecutionContext *ctx, const char *str)
{
    return __qmljs_identifier_from_utf8(ctx, str); // ### make it unique
}

void __qmljs_llvm_call_activation_property(ExecutionContext *context, Value *result, String *name, Value *args, int argc)
{
    __qmljs_call_activation_property(context, result, name, args, argc);
}

void __qmljs_llvm_call_value(ExecutionContext *context, Value *result, const Value *thisObject, const Value *func, Value *args, int argc)
{
    Value that = thisObject ? *thisObject : Value::undefinedValue();
    *result = __qmljs_call_value(context, that, *func, args, argc);
}

void __qmljs_llvm_construct_activation_property(ExecutionContext *context, Value *result, String *name, Value *args, int argc)
{
    __qmljs_construct_activation_property(context, result, name, args, argc);
}

void __qmljs_llvm_construct_value(ExecutionContext *context, Value *result, const Value *func, Value *args, int argc)
{
    *result = __qmljs_construct_value(context, *func, args, argc);
}

void __qmljs_llvm_get_activation_property(ExecutionContext *ctx, Value *result, String *name)
{
    __qmljs_get_activation_property(ctx, result, name);
}

void __qmljs_llvm_set_activation_property(ExecutionContext *ctx, String *name, Value *value)
{
    __qmljs_set_activation_property(ctx, name, value);
}

void __qmljs_llvm_get_property(ExecutionContext *ctx, Value *result, Value *object, String *name)
{
    *result = __qmljs_get_property(ctx, *object, name);
}

void __qmljs_llvm_call_property(ExecutionContext *context, Value *result, const Value *base, String *name, Value *args, int argc)
{
    *result = __qmljs_call_property(context, *base, name, args, argc);
}

void __qmljs_llvm_construct_property(ExecutionContext *context, Value *result, const Value *base, String *name, Value *args, int argc)
{
    *result = __qmljs_construct_property(context, *base, name, args, argc);
}

void __qmljs_llvm_get_element(ExecutionContext *ctx, Value *result, Value *object, Value *index)
{
    *result = __qmljs_get_element(ctx, *object, *index);
}

void __qmljs_llvm_set_element(ExecutionContext *ctx, Value *object, Value *index, Value *value)
{
    __qmljs_set_element(ctx, *object, *index, *value);
}

void __qmljs_llvm_set_property(ExecutionContext *ctx, Value *object, String *name, Value *value)
{
    __qmljs_set_property(ctx, *object, name, *value);
}

void __qmljs_llvm_builtin_declare_var(ExecutionContext *ctx, bool deletable, String *name)
{
    __qmljs_builtin_declare_var(ctx, deletable, name);
}

void __qmljs_llvm_typeof(ExecutionContext *ctx, Value *result, const Value *value)
{
    *result = __qmljs_builtin_typeof(*value, ctx);
}

void __qmljs_llvm_throw(ExecutionContext *context, Value *value)
{
    __qmljs_throw(*value, context);
}

void __qmljs_llvm_create_exception_handler(ExecutionContext *context, Value *result)
{
    void *buf = __qmljs_create_exception_handler(context);
    *result = Value::fromInt32(setjmp(* static_cast<jmp_buf *>(buf)));
}

void __qmljs_llvm_delete_exception_handler(ExecutionContext *context)
{
    __qmljs_delete_exception_handler(context);
}

void __qmljs_llvm_get_exception(ExecutionContext *context, Value *result)
{
    *result = __qmljs_get_exception(context);
}

void __qmljs_llvm_foreach_iterator_object(ExecutionContext *context, Value *result, Value *in)
{
    *result = __qmljs_foreach_iterator_object(*in, context);
}

void __qmljs_llvm_foreach_next_property_name(Value *result, Value *it)
{
    *result = __qmljs_foreach_next_property_name(*it);
}

void __qmljs_llvm_get_this_object(ExecutionContext *ctx, Value *result)
{
    *result = __qmljs_get_thisObject(ctx);
}

void __qmljs_llvm_delete_subscript(ExecutionContext *ctx, Value *result, Value *base, Value *index)
{
    *result = __qmljs_delete_subscript(ctx, *base, *index);
}

void __qmljs_llvm_delete_member(ExecutionContext *ctx, Value *result, Value *base, String *name)
{
    *result = __qmljs_delete_member(ctx, *base, name);
}

void __qmljs_llvm_delete_name(ExecutionContext *ctx, Value *result, String *name)
{
    *result = __qmljs_delete_name(ctx, name);
}

} // extern "C"
