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

#include "debugging.h"
#include "qmljs_runtime.h"
#include "qmljs_objects.h"
#include "qv4ir_p.h"
#include "qv4ecmaobjects_p.h"

#include <QtCore/qmath.h>
#include <QtCore/qnumeric.h>
#include <QtCore/QDebug>
#include <cstdio>
#include <cassert>
#include <typeinfo>
#include <stdlib.h>
#include <xlocale.h>

namespace QQmlJS {
namespace VM {

static inline Value callFunction(ExecutionContext *context, Value thisObject, FunctionObject *func, Value *args, int argc)
{
    if (func) {
        return func->call(context, thisObject, args, argc);
    } else {
        context->throwTypeError();
        return Value::undefinedValue();
    }
}

QString numberToString(double num, int radix = 10)
{
    if (std::isnan(num)) {
        return QStringLiteral("NaN");
    } else if (qIsInf(num)) {
        return QLatin1String(num < 0 ? "-Infinity" : "Infinity");
    }

    if (radix == 10)
        return QString::number(num, 'g', 16);

    QString str;
    bool negative = false;

    if (num < 0) {
        negative = true;
        num = -num;
    }

    double frac = num - ::floor(num);
    num = Value::toInteger(num);

    do {
        char c = (char)::fmod(num, radix);
        c = (c < 10) ? (c + '0') : (c - 10 + 'a');
        str.prepend(QLatin1Char(c));
        num = ::floor(num / radix);
    } while (num != 0);

    if (frac != 0) {
        str.append(QLatin1Char('.'));
        do {
            frac = frac * radix;
            char c = (char)::floor(frac);
            c = (c < 10) ? (c + '0') : (c - 10 + 'a');
            str.append(QLatin1Char(c));
            frac = frac - ::floor(frac);
        } while (frac != 0);
    }

    if (negative)
        str.prepend(QLatin1Char('-'));

    return str;
}

extern "C" {

Value __qmljs_init_closure(IR::Function *clos, ExecutionContext *ctx)
{
    return Value::fromObject(ctx->engine->newScriptFunction(ctx, clos));
}

Value __qmljs_init_native_function(String *name, Value (*code)(ExecutionContext *), ExecutionContext *ctx)
{
    return Value::fromObject(ctx->engine->newNativeFunction(ctx, name, code));
}

Value __qmljs_string_literal_undefined(ExecutionContext *ctx)
{
    return Value::fromString(ctx->engine->identifier(QStringLiteral("undefined")));
}

Value __qmljs_string_literal_null(ExecutionContext *ctx)
{
    return Value::fromString(ctx->engine->identifier(QStringLiteral("null")));
}

Value __qmljs_string_literal_true(ExecutionContext *ctx)
{
    return Value::fromString(ctx->engine->identifier(QStringLiteral("true")));
}

Value __qmljs_string_literal_false(ExecutionContext *ctx)
{
    return Value::fromString(ctx->engine->identifier(QStringLiteral("false")));
}

Value __qmljs_string_literal_object(ExecutionContext *ctx)
{
    return Value::fromString(ctx->engine->identifier(QStringLiteral("object")));
}

Value __qmljs_string_literal_boolean(ExecutionContext *ctx)
{
    return Value::fromString(ctx->engine->identifier(QStringLiteral("boolean")));
}

Value __qmljs_string_literal_number(ExecutionContext *ctx)
{
    return Value::fromString(ctx->engine->identifier(QStringLiteral("number")));
}

Value __qmljs_string_literal_string(ExecutionContext *ctx)
{
    return Value::fromString(ctx->engine->identifier(QStringLiteral("string")));
}

Value __qmljs_string_literal_function(ExecutionContext *ctx)
{
    return Value::fromString(ctx->engine->identifier(QStringLiteral("function")));
}

Value __qmljs_delete_subscript(ExecutionContext *ctx, Value base, Value index)
{
    if (ArrayObject *a = base.asArrayObject()) {
        int n = -1;
        if (index.isInteger())
            n = index.integerValue();
        else if (index.isDouble())
            n = index.doubleValue();
        if (n >= 0) {
            if (n < (int) a->value.size()) {
                a->value.assign(n, Value::undefinedValue());
                return Value::fromBoolean(true);
            }
        }
    }

    String *name = index.toString(ctx);
    return __qmljs_delete_member(ctx, base, name);
}

Value __qmljs_delete_member(ExecutionContext *ctx, Value base, String *name)
{
    Value obj = base.toObject(ctx);
    return Value::fromBoolean(obj.objectValue()->__delete__(ctx, name));
}

Value __qmljs_delete_name(ExecutionContext *ctx, String *name)
{
    return Value::fromBoolean(ctx->deleteProperty(name));
}

Value __qmljs_add_helper(Value left, Value right, ExecutionContext *ctx)
{
    Value pleft = __qmljs_to_primitive(left, ctx, PREFERREDTYPE_HINT);
    Value pright = __qmljs_to_primitive(right, ctx, PREFERREDTYPE_HINT);
    if (pleft.isString() || pright.isString()) {
        if (!pleft.isString())
            pleft = __qmljs_to_string(pleft, ctx);
        if (!pright.isString())
            pright = __qmljs_to_string(pright, ctx);
        String *string = __qmljs_string_concat(ctx, pleft.stringValue(), pright.stringValue());
        return Value::fromString(string);
    }
    double x = __qmljs_to_number(pleft, ctx);
    double y = __qmljs_to_number(pright, ctx);
    return Value::fromDouble(x + y);
}

Value __qmljs_instanceof(Value left, Value right, ExecutionContext *ctx)
{
    if (FunctionObject *function = right.asFunctionObject()) {
        bool r = function->hasInstance(ctx, left);
        return Value::fromBoolean(r);
    }

    return __qmljs_throw_type_error(ctx);
}

Value __qmljs_in(Value left, Value right, ExecutionContext *ctx)
{
    if (right.isObject()) {
        Value s = __qmljs_to_string(left, ctx);
        bool r = right.objectValue()->__hasProperty__(ctx, s.stringValue());
        return Value::fromBoolean(r);
    } else {
        return __qmljs_throw_type_error(ctx);
    }
}

void __qmljs_inplace_bit_and_name(Value value, String *name, ExecutionContext *ctx)
{
    ctx->inplaceBitOp(value, name, __qmljs_bit_and);
}

void __qmljs_inplace_bit_or_name(Value value, String *name, ExecutionContext *ctx)
{
    ctx->inplaceBitOp(value, name, __qmljs_bit_or);
}

void __qmljs_inplace_bit_xor_name(Value value, String *name, ExecutionContext *ctx)
{
    ctx->inplaceBitOp(value, name, __qmljs_bit_xor);
}

void __qmljs_inplace_add_name(Value value, String *name, ExecutionContext *ctx)
{
    ctx->inplaceBitOp(value, name, __qmljs_add);
}

void __qmljs_inplace_sub_name(Value value, String *name, ExecutionContext *ctx)
{
    ctx->inplaceBitOp(value, name, __qmljs_sub);
}

void __qmljs_inplace_mul_name(Value value, String *name, ExecutionContext *ctx)
{
    ctx->inplaceBitOp(value, name, __qmljs_mul);
}

void __qmljs_inplace_div_name(Value value, String *name, ExecutionContext *ctx)
{
    ctx->inplaceBitOp(value, name, __qmljs_div);
}

void __qmljs_inplace_mod_name(Value value, String *name, ExecutionContext *ctx)
{
    ctx->inplaceBitOp(value, name, __qmljs_mod);
}

void __qmljs_inplace_shl_name(Value value, String *name, ExecutionContext *ctx)
{
    ctx->inplaceBitOp(value, name, __qmljs_shl);
}

void __qmljs_inplace_shr_name(Value value, String *name, ExecutionContext *ctx)
{
    ctx->inplaceBitOp(value, name, __qmljs_shr);
}

void __qmljs_inplace_ushr_name(Value value, String *name, ExecutionContext *ctx)
{
    ctx->inplaceBitOp(value, name, __qmljs_ushr);
}

void __qmljs_inplace_bit_and_element(Value base, Value index, Value value, ExecutionContext *ctx)
{
    Object *obj = base.toObject(ctx).objectValue();
    obj->inplaceBinOp(value, index, __qmljs_bit_and, ctx);
}

void __qmljs_inplace_bit_or_element(Value base, Value index, Value value, ExecutionContext *ctx)
{
    Object *obj = base.toObject(ctx).objectValue();
    obj->inplaceBinOp(value, index, __qmljs_bit_or, ctx);
}

void __qmljs_inplace_bit_xor_element(Value base, Value index, Value value, ExecutionContext *ctx)
{
    Object *obj = base.toObject(ctx).objectValue();
    obj->inplaceBinOp(value, index, __qmljs_bit_xor, ctx);
}

void __qmljs_inplace_add_element(Value base, Value index, Value value, ExecutionContext *ctx)
{
    Object *obj = base.toObject(ctx).objectValue();
    obj->inplaceBinOp(value, index, __qmljs_add, ctx);
}

void __qmljs_inplace_sub_element(Value base, Value index, Value value, ExecutionContext *ctx)
{
    Object *obj = base.toObject(ctx).objectValue();
    obj->inplaceBinOp(value, index, __qmljs_sub, ctx);
}

void __qmljs_inplace_mul_element(Value base, Value index, Value value, ExecutionContext *ctx)
{
    Object *obj = base.toObject(ctx).objectValue();
    obj->inplaceBinOp(value, index, __qmljs_mul, ctx);
}

void __qmljs_inplace_div_element(Value base, Value index, Value value, ExecutionContext *ctx)
{
    Object *obj = base.toObject(ctx).objectValue();
    obj->inplaceBinOp(value, index, __qmljs_div, ctx);
}

void __qmljs_inplace_mod_element(Value base, Value index, Value value, ExecutionContext *ctx)
{
    Object *obj = base.toObject(ctx).objectValue();
    obj->inplaceBinOp(value, index, __qmljs_mod, ctx);
}

void __qmljs_inplace_shl_element(Value base, Value index, Value value, ExecutionContext *ctx)
{
    Object *obj = base.toObject(ctx).objectValue();
    obj->inplaceBinOp(value, index, __qmljs_shl, ctx);
}

void __qmljs_inplace_shr_element(Value base, Value index, Value value, ExecutionContext *ctx)
{
    Object *obj = base.toObject(ctx).objectValue();
    obj->inplaceBinOp(value, index, __qmljs_shr, ctx);
}

void __qmljs_inplace_ushr_element(Value base, Value index, Value value, ExecutionContext *ctx)
{
    Object *obj = base.toObject(ctx).objectValue();
    obj->inplaceBinOp(value, index, __qmljs_ushr, ctx);
}

void __qmljs_inplace_bit_and_member(Value value, Value base, String *name, ExecutionContext *ctx)
{
    Object *o = base.toObject(ctx).objectValue();
    o->inplaceBinOp(value, name, __qmljs_bit_and, ctx);
}

void __qmljs_inplace_bit_or_member(Value value, Value base, String *name, ExecutionContext *ctx)
{
    Object *o = base.toObject(ctx).objectValue();
    o->inplaceBinOp(value, name, __qmljs_bit_or, ctx);
}

void __qmljs_inplace_bit_xor_member(Value value, Value base, String *name, ExecutionContext *ctx)
{
    Object *o = base.toObject(ctx).objectValue();
    o->inplaceBinOp(value, name, __qmljs_bit_xor, ctx);
}

void __qmljs_inplace_add_member(Value value, Value base, String *name, ExecutionContext *ctx)
{
    Object *o = base.toObject(ctx).objectValue();
    o->inplaceBinOp(value, name, __qmljs_add, ctx);
}

void __qmljs_inplace_sub_member(Value value, Value base, String *name, ExecutionContext *ctx)
{
    Object *o = base.toObject(ctx).objectValue();
    o->inplaceBinOp(value, name, __qmljs_sub, ctx);
}

void __qmljs_inplace_mul_member(Value value, Value base, String *name, ExecutionContext *ctx)
{
    Object *o = base.toObject(ctx).objectValue();
    o->inplaceBinOp(value, name, __qmljs_mul, ctx);
}

void __qmljs_inplace_div_member(Value value, Value base, String *name, ExecutionContext *ctx)
{
    Object *o = base.toObject(ctx).objectValue();
    o->inplaceBinOp(value, name, __qmljs_div, ctx);
}

void __qmljs_inplace_mod_member(Value value, Value base, String *name, ExecutionContext *ctx)
{
    Object *o = base.toObject(ctx).objectValue();
    o->inplaceBinOp(value, name, __qmljs_mod, ctx);
}

void __qmljs_inplace_shl_member(Value value, Value base, String *name, ExecutionContext *ctx)
{
    Object *o = base.toObject(ctx).objectValue();
    o->inplaceBinOp(value, name, __qmljs_shl, ctx);
}

void __qmljs_inplace_shr_member(Value value, Value base, String *name, ExecutionContext *ctx)
{
    Object *o = base.toObject(ctx).objectValue();
    o->inplaceBinOp(value, name, __qmljs_shr, ctx);
}

void __qmljs_inplace_ushr_member(Value value, Value base, String *name, ExecutionContext *ctx)
{
    Object *o = base.toObject(ctx).objectValue();
    o->inplaceBinOp(value, name, __qmljs_ushr, ctx);
}

String *__qmljs_string_from_utf8(ExecutionContext *ctx, const char *s)
{
    return ctx->engine->newString(QString::fromUtf8(s));
}

String *__qmljs_identifier_from_utf8(ExecutionContext *ctx, const char *s)
{
    return ctx->engine->identifier(QString::fromUtf8(s));
}

int __qmljs_string_length(ExecutionContext *, String *string)
{
    return string->toQString().length();
}

double __qmljs_string_to_number(ExecutionContext *, String *string)
{
    const QString s = string->toQString();
    if (s.startsWith(QLatin1String("0x")) || s.startsWith(QLatin1String("0X")))
        return s.toLong(0, 16);
    locale_t c_locale = newlocale(LC_ALL_MASK, NULL, NULL);
    double d = ::strtod_l(s.toUtf8().constData(), 0, c_locale);
    freelocale(c_locale);
    return d;
}

Value __qmljs_string_from_number(ExecutionContext *ctx, double number)
{
    String *string = ctx->engine->newString(numberToString(number, 10));
    return Value::fromString(string);
}

Bool __qmljs_string_compare(ExecutionContext *, String *left, String *right)
{
    return left->toQString() < right->toQString();
}

Bool __qmljs_string_equal(String *left, String *right)
{
    return left->isEqualTo(right);
}

String *__qmljs_string_concat(ExecutionContext *ctx, String *first, String *second)
{
    return ctx->engine->newString(first->toQString() + second->toQString());
}

Bool __qmljs_is_function(Value value)
{
    return value.objectValue()->asFunctionObject() != 0;
}

Value __qmljs_object_default_value(ExecutionContext *ctx, Value object, int typeHint)
{
    if (typeHint == PREFERREDTYPE_HINT) {
        if (object.asDateObject())
            typeHint = STRING_HINT;
        else
            typeHint = NUMBER_HINT;
    }

    String *meth1 = ctx->engine->identifier("toString");
    String *meth2 = ctx->engine->identifier("valueOf");

    if (typeHint == NUMBER_HINT)
        qSwap(meth1, meth2);

    assert(object.isObject());
    Object *oo = object.objectValue();

    Value conv = oo->__get__(ctx, meth1);
    if (FunctionObject *f = conv.asFunctionObject()) {
        Value r = callFunction(ctx, object, f, 0, 0);
        if (r.isPrimitive())
            return r;
    }

    conv = oo->__get__(ctx, meth2);
    if (FunctionObject *f = conv.asFunctionObject()) {
        Value r = callFunction(ctx, object, f, 0, 0);
        if (r.isPrimitive())
            return r;
    }

    return Value::undefinedValue();
}

Value __qmljs_throw_type_error(ExecutionContext *ctx)
{
    ctx->throwTypeError();
    return Value::undefinedValue();
}

Value __qmljs_new_object(ExecutionContext *ctx)
{
    return Value::fromObject(ctx->engine->newObject());
}

Value __qmljs_new_boolean_object(ExecutionContext *ctx, bool boolean)
{
    Value value = Value::fromBoolean(boolean);
    return Value::fromObject(ctx->engine->newBooleanObject(value));
}

Value __qmljs_new_number_object(ExecutionContext *ctx, double number)
{
    Value value = Value::fromDouble(number);
    return Value::fromObject(ctx->engine->newNumberObject(value));
}

Value __qmljs_new_string_object(ExecutionContext *ctx, String *string)
{
    Value value = Value::fromString(string);
    return Value::fromObject(ctx->engine->newStringObject(value));
}

void __qmljs_set_property(ExecutionContext *ctx, Value object, String *name, Value value)
{
    object.objectValue()->__put__(ctx, name, value);
}

Value __qmljs_get_element(ExecutionContext *ctx, Value object, Value index)
{
    if (index.isNumber()) {
        if (object.isString()) {
            const QString s = object.stringValue()->toQString().mid(index.toUInt32(ctx), 1);
            if (s.isNull())
                return Value::undefinedValue();
            else
                return Value::fromString(ctx, s);
        }

        if (ArrayObject *a = object.asArrayObject())
            return a->value.at(index.toUInt32(ctx));
    }

    String *name = index.toString(ctx);

    if (! object.isObject())
        object = __qmljs_to_object(object, ctx);

    return object.objectValue()->__get__(ctx, name);
}

void __qmljs_set_element(ExecutionContext *ctx, Value object, Value index, Value value)
{
    if (index.isNumber()) {
        if (ArrayObject *a = object.asArrayObject()) {
            a->value.assign(index.toUInt32(ctx), value);
            return;
        }
    }

    String *name = index.toString(ctx);

    if (! object.isObject())
        object = __qmljs_to_object(object, ctx);

    object.objectValue()->__put__(ctx, name, value);
}

Value __qmljs_foreach_iterator_object(Value in, ExecutionContext *ctx)
{
    in = __qmljs_to_object(in, ctx);
    Object *it = ctx->engine->newForEachIteratorObject(in.objectValue());
    return Value::fromObject(it);
}

Value __qmljs_foreach_next_property_name(Value foreach_iterator)
{
    assert(foreach_iterator.isObject());

    ForEachIteratorObject *it = static_cast<ForEachIteratorObject *>(foreach_iterator.objectValue());
    assert(it->className() == QLatin1String("__ForEachIteratorObject"));

    String *s = it->nextPropertyName();
    if (!s)
        return Value::nullValue();
    return Value::fromString(s);
}


void __qmljs_set_activation_property(ExecutionContext *ctx, String *name, Value value)
{
    PropertyDescriptor tmp;
    if (PropertyDescriptor *prop = ctx->lookupPropertyDescriptor(name, &tmp))
        prop->value = value;
    else
        ctx->engine->globalObject.objectValue()->__put__(ctx, name, value);
}

Value __qmljs_get_property(ExecutionContext *ctx, Value object, String *name)
{
    if (object.isObject()) {
        return object.objectValue()->__get__(ctx, name);
    } else if (object.isString() && name->isEqualTo(ctx->engine->id_length)) {
        return Value::fromInt32(object.stringValue()->toQString().length());
    } else {
        object = __qmljs_to_object(object, ctx);

        if (object.isObject()) {
            return object.objectValue()->__get__(ctx, name);
        } else {
            ctx->throwTypeError();
            return Value::undefinedValue();
        }
    }
}

Value __qmljs_get_activation_property(ExecutionContext *ctx, String *name)
{
    PropertyDescriptor tmp;
    if (PropertyDescriptor *prop = ctx->lookupPropertyDescriptor(name, &tmp))
        return prop->value;
    ctx->throwReferenceError(Value::fromString(name));
    return Value::undefinedValue();
}

Value __qmljs_get_thisObject(ExecutionContext *ctx)
{
    if (ctx->thisObject.isObject())
        return ctx->thisObject;
    return ctx->engine->globalObject;
}

uint __qmljs_equal(Value x, Value y, ExecutionContext *ctx)
{
    if (x.type() == y.type()) {
        switch (x.type()) {
        case Value::Undefined_Type:
            return true;
        case Value::Null_Type:
            return true;
        case Value::Boolean_Type:
            return x.booleanValue() == y.booleanValue();
            break;
        case Value::Integer_Type:
            return x.integerValue() == y.integerValue();
        case Value::String_Type:
            return __qmljs_string_equal(x.stringValue(), y.stringValue());
        case Value::Object_Type:
            return x.objectValue() == y.objectValue();
        default: // double
            return x.doubleValue() == y.doubleValue();
        }
        // unreachable
    } else {
        if (x.isNumber() && y.isNumber())
            return x.asDouble() == y.asDouble();
        if (x.isNull() && y.isUndefined()) {
            return true;
        } else if (x.isUndefined() && y.isNull()) {
            return true;
        } else if (x.isNumber() && y.isString()) {
            Value ny = Value::fromDouble(__qmljs_to_number(y, ctx));
            return __qmljs_equal(x, ny, ctx);
        } else if (x.isString() && y.isNumber()) {
            Value nx = Value::fromDouble(__qmljs_to_number(x, ctx));
            return __qmljs_equal(nx, y, ctx);
        } else if (x.isBoolean()) {
            Value nx = Value::fromDouble((double) x.booleanValue());
            return __qmljs_equal(nx, y, ctx);
        } else if (y.isBoolean()) {
            Value ny = Value::fromDouble((double) y.booleanValue());
            return __qmljs_equal(x, ny, ctx);
        } else if ((x.isNumber() || x.isString()) && y.isObject()) {
            Value py = __qmljs_to_primitive(y, ctx, PREFERREDTYPE_HINT);
            return __qmljs_equal(x, py, ctx);
        } else if (x.isObject() && (y.isNumber() || y.isString())) {
            Value px = __qmljs_to_primitive(x, ctx, PREFERREDTYPE_HINT);
            return __qmljs_equal(px, y, ctx);
        }
    }

    return false;
}

// TODO: remove this function. Backends should just generate a __qmljs_get_activation_property followed by a __qmljs_call_value
Value __qmljs_call_activation_property(ExecutionContext *context, String *name, Value *args, int argc)
{
    Value func = __qmljs_get_activation_property(context, name);
    if (FunctionObject *f = func.asFunctionObject()) {
        return callFunction(context, Value::undefinedValue(), f, args, argc);
    } else {
        context->throwReferenceError(Value::fromString(name));
        return Value::undefinedValue();
    }
}

Value __qmljs_call_property(ExecutionContext *context, Value base, String *name, Value *args, int argc)
{
    Object *baseObject;
    Value thisObject;

    if (base.isUndefined()) {
        baseObject = context->activation;
        thisObject = Value::nullValue();
    } else {
        if (!base.isObject())
            base = __qmljs_to_object(base, context);
        assert(base.isObject());
        baseObject = base.objectValue();
        thisObject = base;
    }

    Value func = baseObject ? baseObject->__get__(context, name) : Value::undefinedValue();
    return callFunction(context, thisObject, func.asFunctionObject(), args, argc);
}

Value __qmljs_call_value(ExecutionContext *context, Value thisObject, Value func, Value *args, int argc)
{
    return callFunction(context, thisObject, func.asFunctionObject(), args, argc);
}

Value __qmljs_construct_activation_property(ExecutionContext *context, String *name, Value *args, int argc)
{
    PropertyDescriptor tmp;
    PropertyDescriptor *func = context->lookupPropertyDescriptor(name, &tmp);
    if (! func) {
        context->throwReferenceError(Value::fromString(name));
        return Value::undefinedValue();
    }
    return __qmljs_construct_value(context, func->value, args, argc);
}

Value __qmljs_construct_value(ExecutionContext *context, Value func, Value *args, int argc)
{
    if (FunctionObject *f = func.asFunctionObject())
        return f->construct(context, args, argc);

    context->throwTypeError();
    return Value::undefinedValue();
}

Value __qmljs_construct_property(ExecutionContext *context, Value base, String *name, Value *args, int argc)
{
    Value thisObject = base;
    if (!thisObject.isObject())
        thisObject = __qmljs_to_object(base, context);

    Value func = thisObject.objectValue()->__get__(context, name);
    if (FunctionObject *f = func.asFunctionObject())
        return f->construct(context, args, argc);

    context->throwTypeError();
    return Value::undefinedValue();
}

void __qmljs_throw(Value value, ExecutionContext *context)
{
    assert(!context->engine->unwindStack.isEmpty());

    if (context->engine->debugger)
        context->engine->debugger->aboutToThrow(&value);

    ExecutionEngine::ExceptionHandler &handler = context->engine->unwindStack.last();

    // clean up call contexts
    while (context != handler.context) {
        context->leaveCallContext();
        context = context->parent;
    }

    while (context->withObject != handler.with) {
        ExecutionContext::With *w = context->withObject;
        context->withObject = w->next;
        delete w;
    }

    context->engine->exception = value;

    longjmp(handler.stackFrame, 1);
}

void *__qmljs_create_exception_handler(ExecutionContext *context)
{
    context->engine->exception = Value::undefinedValue();
    context->engine->unwindStack.append(ExecutionEngine::ExceptionHandler());
    ExecutionEngine::ExceptionHandler &handler = context->engine->unwindStack.last();
    handler.context = context;
    handler.with = context->withObject;
    return handler.stackFrame;
}

void __qmljs_delete_exception_handler(ExecutionContext *context)
{
    assert(!context->engine->unwindStack.isEmpty());

    context->engine->unwindStack.pop_back();
}

Value __qmljs_get_exception(ExecutionContext *context)
{
    return context->engine->exception;
}

Value __qmljs_builtin_typeof(Value val, ExecutionContext *context)
{
    return __qmljs_typeof(val, context);
}

void __qmljs_builtin_throw(Value val, ExecutionContext *context)
{
    __qmljs_throw(val, context);
}

void __qmljs_builtin_push_with(Value o, ExecutionContext *ctx)
{
    Object *obj = __qmljs_to_object(o, ctx).asObject();
    ctx->pushWithObject(obj);
}

void __qmljs_builtin_pop_with(ExecutionContext *ctx)
{
    ctx->popWithObject();
}

void __qmljs_builtin_declare_var(ExecutionContext *ctx, bool deletable, String *name)
{
    ctx->createMutableBinding(ctx, name, deletable);
}

} // extern "C"


} // namespace VM
} // namespace QQmlJS
