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

#include "qv4runtime_p.h"
#include "qv4context_p.h"
#include "qv4engine_p.h"
#include <stdio.h>
#include <setjmp.h>

using namespace QV4;

extern "C" {

Value __qmljs_llvm_return(ExecutionContext */*ctx*/, Value *result)
{
    return *result;
}

Value *__qmljs_llvm_get_argument(ExecutionContext *ctx, int index)
{
    assert(ctx->type == ExecutionContext::Type_CallContext);
    return &static_cast<CallContext *>(ctx)->arguments[index];
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
    *result = Value::fromString(ctx->engine->newString(QString::fromUtf8(str)));
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
    __qmljs_init_closure(ctx, result, clos);
}

bool __qmljs_llvm_to_boolean(ExecutionContext *ctx, const Value *value)
{
    return __qmljs_to_boolean(*value);
}

void __qmljs_llvm_bit_and(ExecutionContext *ctx, Value *result, Value *left, Value *right)
{
    __qmljs_bit_and(ctx, result, *left, *right);
}

void __qmljs_llvm_bit_or(ExecutionContext *ctx, Value *result, Value *left, Value *right)
{
    __qmljs_bit_or(ctx, result, *left, *right);
}

void __qmljs_llvm_bit_xor(ExecutionContext *ctx, Value *result, Value *left, Value *right)
{
    __qmljs_bit_xor(ctx, result, *left, *right);
}

void __qmljs_llvm_add(ExecutionContext *ctx, Value *result, Value *left, Value *right)
{
    __qmljs_add(ctx, result, *left, *right);
}

void __qmljs_llvm_sub(ExecutionContext *ctx, Value *result, Value *left, Value *right)
{
    __qmljs_sub(ctx, result, *left, *right);
}

void __qmljs_llvm_mul(ExecutionContext *ctx, Value *result, Value *left, Value *right)
{
    __qmljs_mul(ctx, result, *left, *right);
}

void __qmljs_llvm_div(ExecutionContext *ctx, Value *result, Value *left, Value *right)
{
    __qmljs_div(ctx, result, *left, *right);
}

void __qmljs_llvm_mod(ExecutionContext *ctx, Value *result, Value *left, Value *right)
{
    __qmljs_mod(ctx, result, *left, *right);
}

void __qmljs_llvm_shl(ExecutionContext *ctx, Value *result, Value *left, Value *right)
{
    __qmljs_shl(ctx, result, *left, *right);
}

void __qmljs_llvm_shr(ExecutionContext *ctx, Value *result, Value *left, Value *right)
{
    __qmljs_shr(ctx, result, *left, *right);
}

void __qmljs_llvm_ushr(ExecutionContext *ctx, Value *result, Value *left, Value *right)
{
    __qmljs_ushr(ctx, result, *left, *right);
}

void __qmljs_llvm_gt(ExecutionContext *ctx, Value *result, Value *left, Value *right)
{
    __qmljs_gt(ctx, result, *left, *right);
}

void __qmljs_llvm_lt(ExecutionContext *ctx, Value *result, Value *left, Value *right)
{
    __qmljs_lt(ctx, result, *left, *right);
}

void __qmljs_llvm_ge(ExecutionContext *ctx, Value *result, Value *left, Value *right)
{
    __qmljs_ge(ctx, result, *left, *right);
}

void __qmljs_llvm_le(ExecutionContext *ctx, Value *result, Value *left, Value *right)
{
    __qmljs_le(ctx, result, *left, *right);
}

void __qmljs_llvm_eq(ExecutionContext *ctx, Value *result, Value *left, Value *right)
{
    __qmljs_eq(ctx, result, *left, *right);
}

void __qmljs_llvm_ne(ExecutionContext *ctx, Value *result, Value *left, Value *right)
{
    __qmljs_ne(ctx, result, *left, *right);
}

void __qmljs_llvm_se(ExecutionContext *ctx, Value *result, Value *left, Value *right)
{
    __qmljs_se(ctx, result, *left, *right);
}

void __qmljs_llvm_sne(ExecutionContext *ctx, Value *result, Value *left, Value *right)
{
    __qmljs_sne(ctx, result, *left, *right);
}

void __qmljs_llvm_instanceof(ExecutionContext *ctx, Value *result, Value *left, Value *right)
{
    __qmljs_instanceof(ctx, result, *left, *right);
}

void __qmljs_llvm_in(ExecutionContext *ctx, Value *result, Value *left, Value *right)
{
    __qmljs_in(ctx, result, *left, *right);
}

void __qmljs_llvm_uplus(ExecutionContext *ctx, Value *result, const Value *value)
{
    __qmljs_uplus(result, *value);
}

void __qmljs_llvm_uminus(ExecutionContext *ctx, Value *result, const Value *value)
{
    __qmljs_uminus(result, *value);
}

void __qmljs_llvm_compl(ExecutionContext *ctx, Value *result, const Value *value)
{
    __qmljs_compl(result, *value);
}

void __qmljs_llvm_not(ExecutionContext *ctx, Value *result, const Value *value)
{
    __qmljs_not(result, *value);
}

void __qmljs_llvm_inplace_bit_and_name(ExecutionContext *ctx, String *dest, Value *src)
{
    __qmljs_inplace_bit_and_name(ctx, dest, *src);
}

void __qmljs_llvm_inplace_bit_or_name(ExecutionContext *ctx, String *dest, Value *src)
{
    __qmljs_inplace_bit_or_name(ctx, dest, *src);
}

void __qmljs_llvm_inplace_bit_xor_name(ExecutionContext *ctx, String *dest, Value *src)
{
    __qmljs_inplace_bit_xor_name(ctx, dest, *src);
}

void __qmljs_llvm_inplace_add_name(ExecutionContext *ctx, String *dest, Value *src)
{
    __qmljs_inplace_add_name(ctx, dest, *src);
}

void __qmljs_llvm_inplace_sub_name(ExecutionContext *ctx, String *dest, Value *src)
{
    __qmljs_inplace_sub_name(ctx, dest, *src);
}

void __qmljs_llvm_inplace_mul_name(ExecutionContext *ctx, String *dest, Value *src)
{
    __qmljs_inplace_mul_name(ctx, dest, *src);
}

void __qmljs_llvm_inplace_div_name(ExecutionContext *ctx, String *dest, Value *src)
{
    __qmljs_inplace_div_name(ctx, dest, *src);
}

void __qmljs_llvm_inplace_mod_name(ExecutionContext *ctx, String *dest, Value *src)
{
    __qmljs_inplace_mod_name(ctx, dest, *src);
}

void __qmljs_llvm_inplace_shl_name(ExecutionContext *ctx, String *dest, Value *src)
{
    __qmljs_inplace_shl_name(ctx, dest, *src);
}

void __qmljs_llvm_inplace_shr_name(ExecutionContext *ctx, String *dest, Value *src)
{
    __qmljs_inplace_shr_name(ctx, dest, *src);
}

void __qmljs_llvm_inplace_ushr_name(ExecutionContext *ctx, String *dest, Value *src)
{
    __qmljs_inplace_ushr_name(ctx, dest, *src);
}

void __qmljs_llvm_inplace_bit_and_element(ExecutionContext *ctx, Value *base, Value *index, Value *value)
{
    __qmljs_inplace_bit_and_element(ctx, *base, *index, *value);
}

void __qmljs_llvm_inplace_bit_or_element(ExecutionContext *ctx, Value *base, Value *index, Value *value)
{
    __qmljs_inplace_bit_or_element(ctx, *base, *index, *value);
}

void __qmljs_llvm_inplace_bit_xor_element(ExecutionContext *ctx, Value *base, Value *index, Value *value)
{
    __qmljs_inplace_bit_xor_element(ctx, *base, *index, *value);
}

void __qmljs_llvm_inplace_add_element(ExecutionContext *ctx, Value *base, Value *index, Value *value)
{
    __qmljs_inplace_add_element(ctx, *base, *index, *value);
}

void __qmljs_llvm_inplace_sub_element(ExecutionContext *ctx, Value *base, Value *index, Value *value)
{
    __qmljs_inplace_sub_element(ctx, *base, *index, *value);
}

void __qmljs_llvm_inplace_mul_element(ExecutionContext *ctx, Value *base, Value *index, Value *value)
{
    __qmljs_inplace_mul_element(ctx, *base, *index, *value);
}

void __qmljs_llvm_inplace_div_element(ExecutionContext *ctx, Value *base, Value *index, Value *value)
{
    __qmljs_inplace_div_element(ctx, *base, *index, *value);
}

void __qmljs_llvm_inplace_mod_element(ExecutionContext *ctx, Value *base, Value *index, Value *value)
{
    __qmljs_inplace_mod_element(ctx, *base, *index, *value);
}

void __qmljs_llvm_inplace_shl_element(ExecutionContext *ctx, Value *base, Value *index, Value *value)
{
    __qmljs_inplace_shl_element(ctx, *base, *index, *value);
}

void __qmljs_llvm_inplace_shr_element(ExecutionContext *ctx, Value *base, Value *index, Value *value)
{
    __qmljs_inplace_shr_element(ctx, *base, *index, *value);
}

void __qmljs_llvm_inplace_ushr_element(ExecutionContext *ctx, Value *base, Value *index, Value *value)
{
    __qmljs_inplace_ushr_element(ctx, *base, *index, *value);
}

void __qmljs_llvm_inplace_bit_and_member(ExecutionContext *ctx, Value *value, Value *base, String *member)
{
    __qmljs_inplace_bit_and_member(ctx, *base, member, *value);
}

void __qmljs_llvm_inplace_bit_or_member(ExecutionContext *ctx, Value *value, Value *base, String *member)
{
    __qmljs_inplace_bit_or_member(ctx, *base, member, *value);
}

void __qmljs_llvm_inplace_bit_xor_member(ExecutionContext *ctx, Value *value, Value *base, String *member)
{
    __qmljs_inplace_bit_xor_member(ctx, *base, member, *value);
}

void __qmljs_llvm_inplace_add_member(ExecutionContext *ctx, Value *value, Value *base, String *member)
{
    __qmljs_inplace_add_member(ctx, *base, member, *value);
}

void __qmljs_llvm_inplace_sub_member(ExecutionContext *ctx, Value *value, Value *base, String *member)
{
    __qmljs_inplace_sub_member(ctx, *base, member, *value);
}

void __qmljs_llvm_inplace_mul_member(ExecutionContext *ctx, Value *value, Value *base, String *member)
{
    __qmljs_inplace_mul_member(ctx, *base, member, *value);
}

void __qmljs_llvm_inplace_div_member(ExecutionContext *ctx, Value *value, Value *base, String *member)
{
    __qmljs_inplace_div_member(ctx, *base, member, *value);
}

void __qmljs_llvm_inplace_mod_member(ExecutionContext *ctx, Value *value, Value *base, String *member)
{
    __qmljs_inplace_mod_member(ctx, *base, member, *value);
}

void __qmljs_llvm_inplace_shl_member(ExecutionContext *ctx, Value *value, Value *base, String *member)
{
    __qmljs_inplace_shl_member(ctx, *base, member, *value);
}

void __qmljs_llvm_inplace_shr_member(ExecutionContext *ctx, Value *value, Value *base, String *member)
{
    __qmljs_inplace_shr_member(ctx, *base, member, *value);
}

void __qmljs_llvm_inplace_ushr_member(ExecutionContext *ctx, Value *value, Value *base, String *member)
{
    __qmljs_inplace_ushr_member(ctx, *base, member, *value);
}

String *__qmljs_llvm_identifier_from_utf8(ExecutionContext *ctx, const char *str)
{
    return ctx->engine->newIdentifier(QString::fromUtf8(str));
}

void __qmljs_llvm_call_activation_property(ExecutionContext *context, Value *result, String *name, Value *args, int argc)
{
    __qmljs_call_activation_property(context, result, name, args, argc);
}

void __qmljs_llvm_call_value(ExecutionContext *context, Value *result, const Value *thisObject, const Value *func, Value *args, int argc)
{
    __qmljs_call_value(context, result, thisObject, *func, args, argc);
}

void __qmljs_llvm_construct_activation_property(ExecutionContext *context, Value *result, String *name, Value *args, int argc)
{
    __qmljs_construct_activation_property(context, result, name, args, argc);
}

void __qmljs_llvm_construct_value(ExecutionContext *context, Value *result, const Value *func, Value *args, int argc)
{
    __qmljs_construct_value(context, result, *func, args, argc);
}

void __qmljs_llvm_get_activation_property(ExecutionContext *ctx, Value *result, String *name)
{
    __qmljs_get_activation_property(ctx, result, name);
}

void __qmljs_llvm_set_activation_property(ExecutionContext *ctx, String *name, Value *value)
{
    __qmljs_set_activation_property(ctx, name, *value);
}

void __qmljs_llvm_get_property(ExecutionContext *ctx, Value *result, Value *object, String *name)
{
    __qmljs_get_property(ctx, result, *object, name);
}

void __qmljs_llvm_call_property(ExecutionContext *context, Value *result, const Value *base, String *name, Value *args, int argc)
{
    __qmljs_call_property(context, result, *base, name, args, argc);
}

void __qmljs_llvm_construct_property(ExecutionContext *context, Value *result, const Value *base, String *name, Value *args, int argc)
{
    __qmljs_construct_property(context, result, *base, name, args, argc);
}

void __qmljs_llvm_get_element(ExecutionContext *ctx, Value *result, Value *object, Value *index)
{
    __qmljs_get_element(ctx, result, *object, *index);
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
    __qmljs_builtin_typeof(ctx, result, *value);
}

void __qmljs_llvm_throw(ExecutionContext *context, Value *value)
{
    __qmljs_throw(context, *value);
}

void __qmljs_llvm_delete_exception_handler(ExecutionContext *context)
{
    // ### FIXME.
}

void __qmljs_llvm_foreach_iterator_object(ExecutionContext *context, Value *result, Value *in)
{
    __qmljs_foreach_iterator_object(context, result, *in);
}

void __qmljs_llvm_foreach_next_property_name(Value *result, Value *it)
{
    __qmljs_foreach_next_property_name(result, *it);
}

void __qmljs_llvm_get_this_object(ExecutionContext *ctx, Value *result)
{
    *result = ctx->thisObject;
}

void __qmljs_llvm_delete_subscript(ExecutionContext *ctx, Value *result, Value *base, Value *index)
{
    __qmljs_delete_subscript(ctx, result, *base, *index);
}

void __qmljs_llvm_delete_member(ExecutionContext *ctx, Value *result, Value *base, String *name)
{
    __qmljs_delete_member(ctx, result, *base, name);
}

void __qmljs_llvm_delete_name(ExecutionContext *ctx, Value *result, String *name)
{
    __qmljs_delete_name(ctx, result, name);
}

} // extern "C"
