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
#include <stdio.h>

using namespace QQmlJS::VM;

extern "C" {

void __qmljs_llvm_return(Context *ctx, Value *result)
{
    ctx->result = *result;
}

Value *__qmljs_llvm_get_argument(Context *ctx, int index)
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

void __qmljs_llvm_init_string(Context *ctx, Value *result, const char *str)
{
    *result = Value::fromString(__qmljs_string_from_utf8(ctx, str));
}

void __qmljs_llvm_init_native_function(Context *ctx, Value *result, void (*code)(Context *))
{
    *result = __qmljs_init_native_function(code, ctx);
}

bool __qmljs_llvm_to_boolean(Context *ctx, const Value *value)
{
    return __qmljs_to_boolean(*value, ctx);
}

void __qmljs_llvm_bit_and(Context *ctx, Value *result, Value *left, Value *right)
{
    *result = __qmljs_bit_and(*left, *right, ctx);
}

void __qmljs_llvm_bit_or(Context *ctx, Value *result, Value *left, Value *right)
{
    *result = __qmljs_bit_or(*left, *right, ctx);
}

void __qmljs_llvm_bit_xor(Context *ctx, Value *result, Value *left, Value *right)
{
    *result = __qmljs_bit_xor(*left, *right, ctx);
}

void __qmljs_llvm_add(Context *ctx, Value *result, Value *left, Value *right)
{
    *result = __qmljs_add(*left, *right, ctx);
}

void __qmljs_llvm_sub(Context *ctx, Value *result, Value *left, Value *right)
{
    *result = __qmljs_sub(*left, *right, ctx);
}

void __qmljs_llvm_mul(Context *ctx, Value *result, Value *left, Value *right)
{
    *result = __qmljs_mul(*left, *right, ctx);
}

void __qmljs_llvm_div(Context *ctx, Value *result, Value *left, Value *right)
{
    *result = __qmljs_div(*left, *right, ctx);
}

void __qmljs_llvm_mod(Context *ctx, Value *result, Value *left, Value *right)
{
    *result = __qmljs_mod(*left, *right, ctx);
}

void __qmljs_llvm_shl(Context *ctx, Value *result, Value *left, Value *right)
{
    *result = __qmljs_shl(*left, *right, ctx);
}

void __qmljs_llvm_shr(Context *ctx, Value *result, Value *left, Value *right)
{
    *result = __qmljs_shr(*left, *right, ctx);
}

void __qmljs_llvm_ushr(Context *ctx, Value *result, Value *left, Value *right)
{
    *result = __qmljs_ushr(*left, *right, ctx);
}

void __qmljs_llvm_gt(Context *ctx, Value *result, Value *left, Value *right)
{
    *result = __qmljs_gt(*left, *right, ctx);
}

void __qmljs_llvm_lt(Context *ctx, Value *result, Value *left, Value *right)
{
    *result = __qmljs_lt(*left, *right, ctx);
}

void __qmljs_llvm_ge(Context *ctx, Value *result, Value *left, Value *right)
{
    *result = __qmljs_ge(*left, *right, ctx);
}

void __qmljs_llvm_le(Context *ctx, Value *result, Value *left, Value *right)
{
    *result = __qmljs_le(*left, *right, ctx);
}

void __qmljs_llvm_eq(Context *ctx, Value *result, Value *left, Value *right)
{
    *result = __qmljs_eq(*left, *right, ctx);
}

void __qmljs_llvm_ne(Context *ctx, Value *result, Value *left, Value *right)
{
    *result = __qmljs_ne(*left, *right, ctx);
}

void __qmljs_llvm_se(Context *ctx, Value *result, Value *left, Value *right)
{
    *result = __qmljs_se(*left, *right, ctx);
}

void __qmljs_llvm_sne(Context *ctx, Value *result, Value *left, Value *right)
{
    *result = __qmljs_sne(*left, *right, ctx);
}

void __qmljs_llvm_instanceof(Context *ctx, Value *result, Value *left, Value *right)
{
    *result = __qmljs_instanceof(*left, *right, ctx);
}

void __qmljs_llvm_in(Context *ctx, Value *result, Value *left, Value *right)
{
    *result = __qmljs_in(*left, *right, ctx);
}

void __qmljs_llvm_uplus(Context *ctx, Value *result, const Value *value)
{
    *result = __qmljs_uplus(*value, ctx);
}

void __qmljs_llvm_uminus(Context *ctx, Value *result, const Value *value)
{
    *result = __qmljs_uminus(*value, ctx);
}

void __qmljs_llvm_compl(Context *ctx, Value *result, const Value *value)
{
    *result = __qmljs_compl(*value, ctx);
}

void __qmljs_llvm_not(Context *ctx, Value *result, const Value *value)
{
    *result = __qmljs_not(*value, ctx);
}

String *__qmljs_llvm_identifier_from_utf8(Context *ctx, const char *str)
{
    return __qmljs_identifier_from_utf8(ctx, str); // ### make it unique
}

void __qmljs_llvm_call_activation_property(Context *context, Value *result, String *name, Value *args, int argc)
{
    *result = __qmljs_call_activation_property(context, name, args, argc);
}

void __qmljs_llvm_call_value(Context *context, Value *result, const Value *thisObject, const Value *func, Value *args, int argc)
{
    Value that = thisObject ? *thisObject : Value::undefinedValue();
    *result = __qmljs_call_value(context, that, func, args, argc);
}

void __qmljs_llvm_construct_activation_property(Context *context, Value *result, String *name, Value *args, int argc)
{
    *result = __qmljs_construct_activation_property(context, name, args, argc);
}

void __qmljs_llvm_construct_value(Context *context, Value *result, const Value *func, Value *args, int argc)
{
    *result = __qmljs_construct_value(context, *func, args, argc);
}

void __qmljs_llvm_get_activation_property(Context *ctx, Value *result, String *name)
{
    *result = __qmljs_get_activation_property(ctx, name);
}

void __qmljs_llvm_set_activation_property(Context *ctx, String *name, Value *value)
{
    __qmljs_set_activation_property(ctx, name, *value);
}

void __qmljs_llvm_get_property(Context *ctx, Value *result, Value *object, String *name)
{
    *result = __qmljs_get_property(ctx, *object, name);
}

void __qmljs_llvm_call_property(Context *context, Value *result, const Value *base, String *name, Value *args, int argc)
{
    *result = __qmljs_call_property(context, *base, name, args, argc);
}

void __qmljs_llvm_construct_property(Context *context, Value *result, const Value *base, String *name, Value *args, int argc)
{
    *result = __qmljs_construct_property(context, *base, name, args, argc);
}

void __qmljs_llvm_get_element(Context *ctx, Value *result, Value *object, Value *index)
{
    *result = __qmljs_get_element(ctx, *object, *index);
}

void __qmljs_llvm_set_element(Context *ctx, Value *object, Value *index, Value *value)
{
    __qmljs_set_element(ctx, *object, *index, *value);
}

void __qmljs_llvm_set_property(Context *ctx, Value *object, String *name, Value *value)
{
    __qmljs_set_property(ctx, *object, name, *value);
}

void __qmljs_llvm_typeof(Context *ctx, Value *result, const Value *value)
{
    *result = __qmljs_typeof(ctx, *value);
}

void __qmljs_llvm_throw(Context *context, Value *value)
{
    __qmljs_throw(context, *value);
}

void __qmljs_llvm_rethrow(Context *context, Value *result)
{
    *result = __qmljs_rethrow(context);
}

void __qmljs_llvm_get_this_object(Context *ctx, Value *result)
{
    *result = __qmljs_get_thisObject(ctx);
}

void __qmljs_llvm_delete_subscript(Context *ctx, Value *result, Value *base, Value *index)
{
    *result = __qmljs_delete_subscript(ctx, *base, *index);
}

void __qmljs_llvm_delete_member(Context *ctx, Value *result, Value *base, String *name)
{
    *result = __qmljs_delete_member(ctx, *base, name);
}

void __qmljs_llvm_delete_property(Context *ctx, Value *result, String *name)
{
    *result = __qmljs_delete_property(ctx, name);
}

void __qmljs_llvm_delete_value(Context *ctx, Value *result, Value *value)
{
    *result = __qmljs_delete_value(ctx, *value);
}

void __qmljs_llvm_init_this_object(Context *ctx)
{
    if (ctx->calledAsConstructor)
        ctx->thisObject = __qmljs_new_object(ctx);
}

} // extern "C"
