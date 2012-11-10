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
#include "qmljs_objects.h"
#include "qv4ir_p.h"
#include "qv4ecmaobjects_p.h"

#include <QtCore/qmath.h>
#include <QtCore/qnumeric.h>
#include <QtCore/QDebug>
#include <cstdio>
#include <cassert>
#include <typeinfo>

namespace QQmlJS {
namespace VM {

static inline Value callFunction(Context *context, Value thisObject, FunctionObject *func, Value *args, int argc)
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
    if (qIsNaN(num)) {
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


Value Value::fromString(Context *ctx, const QString &s)
{
    return fromString(ctx->engine->newString(s));
}

int Value::toInt32(double number)
{
    const double D32 = 4294967296.0;
    const double D31 = D32 / 2.0;

    if ((number >= -D31 && number < D31))
        return static_cast<int>(number);


    if (!std::isfinite(number))
        return 0;

    double d = ::floor(::fabs(number));
    if (std::signbit(number))
        d = -d;

    number = ::fmod(d , D32);

    if (number < -D31)
        number += D32;
    else if (number >= D31)
        number -= D32;

    return int(number);
}

unsigned int Value::toUInt32(double number)
{
    const double D32 = 4294967296.0;
    if ((number >= 0 && number < D32))
        return static_cast<uint>(number);

    if (!std::isfinite(number))
        return +0;

    double d = ::floor(::fabs(number));
    if (std::signbit(number))
        d = -d;

    number = ::fmod(d , D32);

    if (number < 0)
        number += D32;

    return unsigned(number);
}

double Value::toInteger(double number)
{
    if (std::isnan(number))
        return +0;
    else if (! number || std::isinf(number))
        return number;
    const double v = floor(fabs(number));
    return std::signbit(number) ? -v : v;
}

Object *Value::asObject() const
{
    return isObject() ? objectValue() : 0;
}

FunctionObject *Value::asFunctionObject() const
{
    return isObject() ? objectValue()->asFunctionObject() : 0;
}

BooleanObject *Value::asBooleanObject() const
{
    return isObject() ? objectValue()->asBooleanObject() : 0;
}

NumberObject *Value::asNumberObject() const
{
    return isObject() ? objectValue()->asNumberObject() : 0;
}

StringObject *Value::asStringObject() const
{
    return isObject() ? objectValue()->asStringObject() : 0;
}

DateObject *Value::asDateObject() const
{
    return isObject() ? objectValue()->asDateObject() : 0;
}

RegExpObject *Value::asRegExpObject() const
{
    return isObject() ? objectValue()->asRegExpObject() : 0;
}

ArrayObject *Value::asArrayObject() const
{
    return isObject() ? objectValue()->asArrayObject() : 0;
}

ErrorObject *Value::asErrorObject() const
{
    return isObject() ? objectValue()->asErrorObject() : 0;
}

ActivationObject *Value::asArgumentsObject() const
{
    return isObject() ? objectValue()->asActivationObject() : 0;
}

Value Value::property(Context *ctx, String *name) const
{
    return isObject() ? objectValue()->__get__(ctx, name) : undefinedValue();
}

void Context::init(ExecutionEngine *eng)
{
    engine = eng;
    parent = 0;
    arguments = 0;
    argumentCount = 0;
    locals = 0;
    activation = Value::nullValue();
    thisObject = Value::nullValue();
    result = Value::undefinedValue();
    formals = 0;
    formalCount = 0;
    vars = 0;
    varCount = 0;
    calledAsConstructor = false;
}

PropertyDescriptor *Context::lookupPropertyDescriptor(String *name, PropertyDescriptor *tmp)
{
    for (Context *ctx = this; ctx; ctx = ctx->parent) {
        if (Object *act = ctx->activation.asObject()) {
            if (PropertyDescriptor *pd = act->__getPropertyDescriptor__(this, name, tmp))
                return pd;
        }
    }
    return 0;
}

void Context::throwError(Value value)
{
    result = value;
    __qmljs_builtin_throw(value, this);
}

void Context::throwError(const QString &message)
{
    Value v = Value::fromString(this, message);
    throwError(Value::fromObject(engine->newErrorObject(v)));
}

void Context::throwTypeError()
{
    Value v = Value::fromString(this, QStringLiteral("Type error"));
    throwError(Value::fromObject(engine->newErrorObject(v)));
}

void Context::throwUnimplemented(const QString &message)
{
    Value v = Value::fromString(this, QStringLiteral("Unimplemented ") + message);
    throwError(Value::fromObject(engine->newErrorObject(v)));
}

void Context::throwReferenceError(Value value)
{
    String *s = value.toString(this);
    QString msg = s->toQString() + QStringLiteral(" is not defined");
    throwError(Value::fromObject(engine->newErrorObject(Value::fromString(this, msg))));
}

void Context::initCallContext(Context *parent, const Value *object, FunctionObject *f, Value *args, unsigned argc)
{
    engine = parent->engine;
    this->parent = f->scope;
    assert(this->parent == f->scope);
    result = Value::undefinedValue();

    if (f->needsActivation)
        activation = Value::fromObject(engine->newActivationObject(this));
    else
        activation = Value::nullValue();

    if (object)
        thisObject = *object;
    else
        thisObject = Value::nullValue();

    formals = f->formalParameterList;
    formalCount = f->formalParameterCount;
    arguments = args;
    argumentCount = argc;
    if (f->needsActivation || argc < formalCount){
        arguments = new Value[qMax(argc, formalCount)];
        if (argc)
            std::copy(args, args + argc, arguments);
        if (argc < formalCount)
            std::fill(arguments + argc, arguments + formalCount, Value::undefinedValue());
    }
    vars = f->varList;
    varCount = f->varCount;
    locals = varCount ? new Value[varCount] : 0;
    calledAsConstructor = false;
    if (varCount)
        std::fill(locals, locals + varCount, Value::undefinedValue());
}

void Context::leaveCallContext()
{
    if (!activation.isNull()) {
        delete[] locals;
        locals = 0;
    }
}

void Context::initConstructorContext(Context *parent, Value *object, FunctionObject *f, Value *args, unsigned argc)
{
    initCallContext(parent, object, f, args, argc);
    calledAsConstructor = true;
}

void Context::leaveConstructorContext(FunctionObject *f)
{
    assert(thisObject.isObject());
    result = thisObject;

    Value proto = f->__get__(this, engine->id_prototype);
    thisObject.objectValue()->prototype = proto.objectValue();
    if (! thisObject.isObject())
        thisObject.objectValue()->prototype = engine->objectPrototype;

    leaveCallContext();
}

extern "C" {

Value __qmljs_init_closure(IR::Function *clos, Context *ctx)
{
    return Value::fromObject(ctx->engine->newScriptFunction(ctx, clos));
}

Value __qmljs_init_native_function(void (*code)(Context *), Context *ctx)
{
    return Value::fromObject(ctx->engine->newNativeFunction(ctx, code));
}

Value __qmljs_string_literal_undefined(Context *ctx)
{
    return Value::fromString(ctx->engine->identifier(QStringLiteral("undefined")));
}

Value __qmljs_string_literal_null(Context *ctx)
{
    return Value::fromString(ctx->engine->identifier(QStringLiteral("null")));
}

Value __qmljs_string_literal_true(Context *ctx)
{
    return Value::fromString(ctx->engine->identifier(QStringLiteral("true")));
}

Value __qmljs_string_literal_false(Context *ctx)
{
    return Value::fromString(ctx->engine->identifier(QStringLiteral("false")));
}

Value __qmljs_string_literal_object(Context *ctx)
{
    return Value::fromString(ctx->engine->identifier(QStringLiteral("object")));
}

Value __qmljs_string_literal_boolean(Context *ctx)
{
    return Value::fromString(ctx->engine->identifier(QStringLiteral("boolean")));
}

Value __qmljs_string_literal_number(Context *ctx)
{
    return Value::fromString(ctx->engine->identifier(QStringLiteral("number")));
}

Value __qmljs_string_literal_string(Context *ctx)
{
    return Value::fromString(ctx->engine->identifier(QStringLiteral("string")));
}

Value __qmljs_string_literal_function(Context *ctx)
{
    return Value::fromString(ctx->engine->identifier(QStringLiteral("function")));
}

Value __qmljs_delete_subscript(Context *ctx, Value base, Value index)
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

Value __qmljs_delete_member(Context *ctx, Value base, String *name)
{
    Value obj = base.toObject(ctx);
    return Value::fromBoolean(obj.objectValue()->__delete__(ctx, name, true));
}

Value __qmljs_delete_property(Context *ctx, String *name)
{
    Value obj = ctx->activation;
    if (! obj.isObject())
        obj = ctx->engine->globalObject;
    return Value::fromBoolean(obj.objectValue()->__delete__(ctx, name, true));
}

Value __qmljs_delete_value(Context *ctx, Value value)
{
    Q_UNUSED(value);
    return __qmljs_throw_type_error(ctx); // ### throw syntax error
}

Value __qmljs_add_helper(Value left, Value right, Context *ctx)
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

Value __qmljs_instanceof(Value left, Value right, Context *ctx)
{
    if (FunctionObject *function = right.asFunctionObject()) {
        bool r = function->hasInstance(ctx, left);
        return Value::fromBoolean(r);
    }

    return __qmljs_throw_type_error(ctx);
}

Value __qmljs_in(Value left, Value right, Context *ctx)
{
    if (right.isObject()) {
        Value s = __qmljs_to_string(left, ctx);
        bool r = right.objectValue()->__hasProperty__(ctx, s.stringValue());
        return Value::fromBoolean(r);
    } else {
        return __qmljs_throw_type_error(ctx);
    }
}

void __qmljs_inplace_bit_and_name(Value value, String *name, Context *ctx)
{
    PropertyDescriptor tmp;
    if (PropertyDescriptor *prop = ctx->lookupPropertyDescriptor(name, &tmp))
        prop->value = __qmljs_bit_and(prop->value, value, ctx);
    else
        ctx->throwReferenceError(Value::fromString(name));
}

void __qmljs_inplace_bit_or_name(Value value, String *name, Context *ctx)
{
    PropertyDescriptor tmp;
    if (PropertyDescriptor *prop = ctx->lookupPropertyDescriptor(name, &tmp))
        prop->value = __qmljs_bit_or(prop->value, value, ctx);
    else
        ctx->throwReferenceError(Value::fromString(name));
}

void __qmljs_inplace_bit_xor_name(Value value, String *name, Context *ctx)
{
    PropertyDescriptor tmp;
    if (PropertyDescriptor *prop = ctx->lookupPropertyDescriptor(name, &tmp))
        prop->value = __qmljs_bit_xor(prop->value, value, ctx);
    else
        ctx->throwReferenceError(Value::fromString(name));
}

void __qmljs_inplace_add_name(Value value, String *name, Context *ctx)
{
    PropertyDescriptor tmp;
    if (PropertyDescriptor *prop = ctx->lookupPropertyDescriptor(name, &tmp))
        prop->value = __qmljs_add(prop->value, value, ctx);
    else
        ctx->throwReferenceError(Value::fromString(name));
}

void __qmljs_inplace_sub_name(Value value, String *name, Context *ctx)
{
    PropertyDescriptor tmp;
    if (PropertyDescriptor *prop = ctx->lookupPropertyDescriptor(name, &tmp))
        prop->value = __qmljs_sub(prop->value, value, ctx);
    else
        ctx->throwReferenceError(Value::fromString(name));
}

void __qmljs_inplace_mul_name(Value value, String *name, Context *ctx)
{
    PropertyDescriptor tmp;
    if (PropertyDescriptor *prop = ctx->lookupPropertyDescriptor(name, &tmp))
        prop->value = __qmljs_mul(prop->value, value, ctx);
    else
        ctx->throwReferenceError(Value::fromString(name));
}

void __qmljs_inplace_div_name(Value value, String *name, Context *ctx)
{
    PropertyDescriptor tmp;
    if (PropertyDescriptor *prop = ctx->lookupPropertyDescriptor(name, &tmp))
        prop->value = __qmljs_div(prop->value, value, ctx);
    else
        ctx->throwReferenceError(Value::fromString(name));
}

void __qmljs_inplace_mod_name(Value value, String *name, Context *ctx)
{
    PropertyDescriptor tmp;
    if (PropertyDescriptor *prop = ctx->lookupPropertyDescriptor(name, &tmp))
        prop->value = __qmljs_mod(prop->value, value, ctx);
    else
        ctx->throwReferenceError(Value::fromString(name));
}

void __qmljs_inplace_shl_name(Value value, String *name, Context *ctx)
{
    PropertyDescriptor tmp;
    if (PropertyDescriptor *prop = ctx->lookupPropertyDescriptor(name, &tmp))
        prop->value = __qmljs_shl(prop->value, value, ctx);
    else
        ctx->throwReferenceError(Value::fromString(name));
}

void __qmljs_inplace_shr_name(Value value, String *name, Context *ctx)
{
    PropertyDescriptor tmp;
    if (PropertyDescriptor *prop = ctx->lookupPropertyDescriptor(name, &tmp))
        prop->value = __qmljs_shr(prop->value, value, ctx);
    else
        ctx->throwReferenceError(Value::fromString(name));
}

void __qmljs_inplace_ushr_name(Value value, String *name, Context *ctx)
{
    PropertyDescriptor tmp;
    if (PropertyDescriptor *prop = ctx->lookupPropertyDescriptor(name, &tmp))
        prop->value = __qmljs_ushr(prop->value, value, ctx);
    else
        ctx->throwReferenceError(Value::fromString(name));
}

void __qmljs_inplace_bit_and_element(Value base, Value index, Value value, Context *ctx)
{
    Object *obj = base.toObject(ctx).objectValue();
    if (ArrayObject *a = obj->asArrayObject()) {
        if (index.isNumber()) {
            const quint32 idx = index.toUInt32(ctx);
            Value v = a->value.at(idx);
            v = __qmljs_bit_and(v, value, ctx);
            a->value.assign(idx, v);
            return;
        }
    }
    String *s = index.toString(ctx);
    assert(s);
    __qmljs_inplace_bit_and_member(value, base, s, ctx);
}

void __qmljs_inplace_bit_or_element(Value base, Value index, Value value, Context *ctx)
{
    Object *obj = base.toObject(ctx).objectValue();
    if (ArrayObject *a = obj->asArrayObject()) {
        if (index.isNumber()) {
            const quint32 idx = index.toUInt32(ctx);
            Value v = a->value.at(idx);
            v = __qmljs_bit_or(v, value, ctx);
            a->value.assign(idx, v);
            return;
        }
    }
    String *s = index.toString(ctx);
    assert(s);
    __qmljs_inplace_bit_or_member(value, base, s, ctx);
}

void __qmljs_inplace_bit_xor_element(Value base, Value index, Value value, Context *ctx)
{
    Object *obj = base.toObject(ctx).objectValue();
    if (ArrayObject *a = obj->asArrayObject()) {
        if (index.isNumber()) {
            const quint32 idx = index.toUInt32(ctx);
            Value v = a->value.at(idx);
            v = __qmljs_bit_xor(v, value, ctx);
            a->value.assign(idx, v);
            return;
        }
    }
    String *s = index.toString(ctx);
    assert(s);
    __qmljs_inplace_bit_xor_member(value, base, s, ctx);
}

void __qmljs_inplace_add_element(Value base, Value index, Value value, Context *ctx)
{
    Object *obj = base.toObject(ctx).objectValue();
    if (ArrayObject *a = obj->asArrayObject()) {
        if (index.isNumber()) {
            const quint32 idx = index.toUInt32(ctx);
            Value v = a->value.at(idx);
            v = __qmljs_add(v, value, ctx);
            a->value.assign(idx, v);
            return;
        }
    }
    String *s = index.toString(ctx);
    assert(s);
    __qmljs_inplace_add_member(value, base, s, ctx);
}

void __qmljs_inplace_sub_element(Value base, Value index, Value value, Context *ctx)
{
    Object *obj = base.toObject(ctx).objectValue();
    if (ArrayObject *a = obj->asArrayObject()) {
        if (index.isNumber()) {
            const quint32 idx = index.toUInt32(ctx);
            Value v = a->value.at(idx);
            v = __qmljs_sub(v, value, ctx);
            a->value.assign(idx, v);
            return;
        }
    }
    String *s = index.toString(ctx);
    assert(s);
    __qmljs_inplace_sub_member(value, base, s, ctx);
}

void __qmljs_inplace_mul_element(Value base, Value index, Value value, Context *ctx)
{
    Object *obj = base.toObject(ctx).objectValue();
    if (ArrayObject *a = obj->asArrayObject()) {
        if (index.isNumber()) {
            const quint32 idx = index.toUInt32(ctx);
            Value v = a->value.at(idx);
            v = __qmljs_mul(v, value, ctx);
            a->value.assign(idx, v);
            return;
        }
    }
    String *s = index.toString(ctx);
    assert(s);
    __qmljs_inplace_mul_member(value, base, s, ctx);
}

void __qmljs_inplace_div_element(Value base, Value index, Value value, Context *ctx)
{
    Object *obj = base.toObject(ctx).objectValue();
    if (ArrayObject *a = obj->asArrayObject()) {
        if (index.isNumber()) {
            const quint32 idx = index.toUInt32(ctx);
            Value v = a->value.at(idx);
            v = __qmljs_div(v, value, ctx);
            a->value.assign(idx, v);
            return;
        }
    }
    String *s = index.toString(ctx);
    assert(s);
    __qmljs_inplace_div_member(value, base, s, ctx);
}

void __qmljs_inplace_mod_element(Value base, Value index, Value value, Context *ctx)
{
    Object *obj = base.toObject(ctx).objectValue();
    if (ArrayObject *a = obj->asArrayObject()) {
        if (index.isNumber()) {
            const quint32 idx = index.toUInt32(ctx);
            Value v = a->value.at(idx);
            v = __qmljs_mod(v, value, ctx);
            a->value.assign(idx, v);
            return;
        }
    }
    String *s = index.toString(ctx);
    assert(s);
    __qmljs_inplace_mod_member(value, base, s, ctx);
}

void __qmljs_inplace_shl_element(Value base, Value index, Value value, Context *ctx)
{
    Object *obj = base.toObject(ctx).objectValue();
    if (ArrayObject *a = obj->asArrayObject()) {
        if (index.isNumber()) {
            const quint32 idx = index.toUInt32(ctx);
            Value v = a->value.at(idx);
            v = __qmljs_shl(v, value, ctx);
            a->value.assign(idx, v);
            return;
        }
    }
    String *s = index.toString(ctx);
    assert(s);
    __qmljs_inplace_shl_member(value, base, s, ctx);
}

void __qmljs_inplace_shr_element(Value base, Value index, Value value, Context *ctx)
{
    Object *obj = base.toObject(ctx).objectValue();
    if (ArrayObject *a = obj->asArrayObject()) {
        if (index.isNumber()) {
            const quint32 idx = index.toUInt32(ctx);
            Value v = a->value.at(idx);
            v = __qmljs_shr(v, value, ctx);
            a->value.assign(idx, v);
            return;
        }
    }
    String *s = index.toString(ctx);
    assert(s);
    __qmljs_inplace_shr_member(value, base, s, ctx);
}

void __qmljs_inplace_ushr_element(Value base, Value index, Value value, Context *ctx)
{
    Object *obj = base.toObject(ctx).objectValue();
    if (ArrayObject *a = obj->asArrayObject()) {
        if (index.isNumber()) {
            const quint32 idx = index.toUInt32(ctx);
            Value v = a->value.at(idx);
            v = __qmljs_ushr(v, value, ctx);
            a->value.assign(idx, v);
            return;
        }
    }
    String *s = index.toString(ctx);
    assert(s);
    __qmljs_inplace_ushr_member(value, base, s, ctx);
}

void __qmljs_inplace_bit_and_member(Value value, Value base, String *name, Context *ctx)
{
    Object *o = base.objectValue();
    Value prop = o->__get__(ctx, name);
    prop = __qmljs_bit_and(prop, value, ctx);
    o->__put__(ctx, name, prop);
}

void __qmljs_inplace_bit_or_member(Value value, Value base, String *name, Context *ctx)
{
    Object *o = base.objectValue();
    Value prop = o->__get__(ctx, name);
    prop = __qmljs_bit_or(prop, value, ctx);
    o->__put__(ctx, name, prop);
}

void __qmljs_inplace_bit_xor_member(Value value, Value base, String *name, Context *ctx)
{
    Object *o = base.objectValue();
    Value prop = o->__get__(ctx, name);
    prop = __qmljs_bit_xor(prop, value, ctx);
    o->__put__(ctx, name, prop);
}

void __qmljs_inplace_add_member(Value value, Value base, String *name, Context *ctx)
{
    Object *o = base.objectValue();
    Value prop = o->__get__(ctx, name);
    prop = __qmljs_add(prop, value, ctx);
    o->__put__(ctx, name, prop);
}

void __qmljs_inplace_sub_member(Value value, Value base, String *name, Context *ctx)
{
    Object *o = base.objectValue();
    Value prop = o->__get__(ctx, name);
    prop = __qmljs_sub(prop, value, ctx);
    o->__put__(ctx, name, prop);
}

void __qmljs_inplace_mul_member(Value value, Value base, String *name, Context *ctx)
{
    Object *o = base.objectValue();
    Value prop = o->__get__(ctx, name);
    prop = __qmljs_mul(prop, value, ctx);
    o->__put__(ctx, name, prop);
}

void __qmljs_inplace_div_member(Value value, Value base, String *name, Context *ctx)
{
    Object *o = base.objectValue();
    Value prop = o->__get__(ctx, name);
    prop = __qmljs_div(prop, value, ctx);
    o->__put__(ctx, name, prop);
}

void __qmljs_inplace_mod_member(Value value, Value base, String *name, Context *ctx)
{
    Object *o = base.objectValue();
    Value prop = o->__get__(ctx, name);
    prop = __qmljs_mod(prop, value, ctx);
    o->__put__(ctx, name, prop);
}

void __qmljs_inplace_shl_member(Value value, Value base, String *name, Context *ctx)
{
    Object *o = base.objectValue();
    Value prop = o->__get__(ctx, name);
    prop = __qmljs_shl(prop, value, ctx);
    o->__put__(ctx, name, prop);
}

void __qmljs_inplace_shr_member(Value value, Value base, String *name, Context *ctx)
{
    Object *o = base.objectValue();
    Value prop = o->__get__(ctx, name);
    prop = __qmljs_shr(prop, value, ctx);
    o->__put__(ctx, name, prop);
}

void __qmljs_inplace_ushr_member(Value value, Value base, String *name, Context *ctx)
{
    Object *o = base.objectValue();
    Value prop = o->__get__(ctx, name);
    prop = __qmljs_ushr(prop, value, ctx);
    o->__put__(ctx, name, prop);
}

String *__qmljs_string_from_utf8(Context *ctx, const char *s)
{
    return ctx->engine->newString(QString::fromUtf8(s));
}

String *__qmljs_identifier_from_utf8(Context *ctx, const char *s)
{
    return ctx->engine->identifier(QString::fromUtf8(s));
}

int __qmljs_string_length(Context *, String *string)
{
    return string->toQString().length();
}

double __qmljs_string_to_number(Context *, String *string)
{
    const QString s = string->toQString();
    if (s.startsWith(QLatin1String("0x")) || s.startsWith(QLatin1String("0X")))
        return s.toLong(0, 16);
    bool ok = false;
    return string->toQString().toDouble(&ok); // ### TODO
}

Value __qmljs_string_from_number(Context *ctx, double number)
{
    String *string = ctx->engine->newString(numberToString(number, 10));
    return Value::fromString(string);
}

Bool __qmljs_string_compare(Context *, String *left, String *right)
{
    return left->toQString() < right->toQString();
}

Bool __qmljs_string_equal(String *left, String *right)
{
    return left->isEqualTo(right);
}

String *__qmljs_string_concat(Context *ctx, String *first, String *second)
{
    return ctx->engine->newString(first->toQString() + second->toQString());
}

Bool __qmljs_is_function(Value value)
{
    return value.objectValue()->asFunctionObject() != 0;
}

Value __qmljs_object_default_value(Context *ctx, Value object, int typeHint)
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

Value __qmljs_throw_type_error(Context *ctx)
{
    ctx->throwTypeError();
    return ctx->result;
}

Value __qmljs_new_object(Context *ctx)
{
    return Value::fromObject(ctx->engine->newObject());
}

Value __qmljs_new_boolean_object(Context *ctx, bool boolean)
{
    Value value = Value::fromBoolean(boolean);
    return Value::fromObject(ctx->engine->newBooleanObject(value));
}

Value __qmljs_new_number_object(Context *ctx, double number)
{
    Value value = Value::fromDouble(number);
    return Value::fromObject(ctx->engine->newNumberObject(value));
}

Value __qmljs_new_string_object(Context *ctx, String *string)
{
    Value value = Value::fromString(string);
    return Value::fromObject(ctx->engine->newStringObject(value));
}

void __qmljs_set_property(Context *ctx, Value object, String *name, Value value)
{
    object.objectValue()->__put__(ctx, name, value, /*flags*/ 0);
}

void __qmljs_set_property_boolean(Context *ctx, Value *object, String *name, bool b)
{
    Value value = Value::fromBoolean(b);
    object->objectValue()->__put__(ctx, name, value, /*flag*/ 0);
}

void __qmljs_set_property_number(Context *ctx, Value *object, String *name, double number)
{
    Q_UNUSED(ctx);
    Value value = Value::fromDouble(number);
    object->objectValue()->__put__(ctx, name, value, /*flag*/ 0);
}

void __qmljs_set_property_string(Context *ctx, Value *object, String *name, String *s)
{
    Q_UNUSED(ctx);
    Value value = Value::fromString(s);
    object->objectValue()->__put__(ctx, name, value, /*flag*/ 0);
}

void __qmljs_set_property_closure(Context *ctx, Value *object, String *name, IR::Function *function)
{
    Value value = __qmljs_init_closure(function, ctx);
    object->objectValue()->__put__(ctx, name, value, /*flag*/ 0);
}

Value __qmljs_get_element(Context *ctx, Value object, Value index)
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

void __qmljs_set_element(Context *ctx, Value object, Value index, Value value)
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

    object.objectValue()->__put__(ctx, name, value, /*flags*/ 0);
}

Value __qmljs_foreach_iterator_object(Value in, Context *ctx)
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


void __qmljs_set_activation_property(Context *ctx, String *name, Value value)
{
    PropertyDescriptor tmp;
    if (PropertyDescriptor *prop = ctx->lookupPropertyDescriptor(name, &tmp))
        prop->value = value;
    else
        ctx->engine->globalObject.objectValue()->__put__(ctx, name, value);
}

void __qmljs_set_activation_property_boolean(Context *ctx, String *name, bool b)
{
    Value value = Value::fromBoolean(b);
    __qmljs_set_activation_property(ctx, name, value);
}

void __qmljs_set_activation_property_number(Context *ctx, String *name, double number)
{
    Value value = Value::fromDouble(number);
    __qmljs_set_activation_property(ctx, name, value);
}

void __qmljs_set_activation_property_string(Context *ctx, String *name, String *string)
{
    Value value = Value::fromString(string);
    __qmljs_set_activation_property(ctx, name, value);
}

void __qmljs_set_activation_property_closure(Context *ctx, String *name, IR::Function *clos)
{
    Value value = __qmljs_init_closure(clos, ctx);
    __qmljs_set_activation_property(ctx, name, value);
}

Value __qmljs_get_property(Context *ctx, Value object, String *name)
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

Value __qmljs_get_activation_property(Context *ctx, String *name)
{
    PropertyDescriptor tmp;
    if (PropertyDescriptor *prop = ctx->lookupPropertyDescriptor(name, &tmp))
        return prop->value;
    ctx->throwReferenceError(Value::fromString(name));
    return Value::undefinedValue();
}

Value __qmljs_get_thisObject(Context *ctx)
{
    if (ctx->thisObject.isObject())
        return ctx->thisObject;
    return ctx->engine->globalObject;
}

uint __qmljs_equal(Value x, Value y, Context *ctx)
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
Value __qmljs_call_activation_property(Context *context, String *name, Value *args, int argc)
{
    Value func = __qmljs_get_activation_property(context, name);
    if (FunctionObject *f = func.asFunctionObject()) {
        return callFunction(context, Value::undefinedValue(), f, args, argc);
    } else {
        context->throwReferenceError(Value::fromString(name));
        return Value::undefinedValue();
    }
}

Value __qmljs_call_property(Context *context, Value base, String *name, Value *args, int argc)
{
    Value baseObject;
    Value thisObject;

    if (base.isUndefined()) {
        baseObject = context->activation;
        thisObject = Value::nullValue();
    } else {
        baseObject = base;
        if (!baseObject.isObject())
            baseObject = __qmljs_to_object(baseObject, context);
        assert(baseObject.isObject());
        thisObject = baseObject;
    }

    Value func = baseObject.property(context, name);
    return callFunction(context, thisObject, func.asFunctionObject(), args, argc);
}

Value __qmljs_call_value(Context *context, Value thisObject, Value func, Value *args, int argc)
{
    return callFunction(context, thisObject, func.asFunctionObject(), args, argc);
}

Value __qmljs_construct_activation_property(Context *context, String *name, Value *args, int argc)
{
    PropertyDescriptor tmp;
    PropertyDescriptor *func = context->lookupPropertyDescriptor(name, &tmp);
    if (! func) {
        context->throwReferenceError(Value::fromString(name));
        return Value::undefinedValue();
    }
    return __qmljs_construct_value(context, func->value, args, argc);
}

Value __qmljs_construct_value(Context *context, Value func, Value *args, int argc)
{
    if (FunctionObject *f = func.asFunctionObject())
        return f->construct(context, args, argc);

    context->throwTypeError();
    return Value::undefinedValue();
}

Value __qmljs_construct_property(Context *context, Value base, String *name, Value *args, int argc)
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

void __qmljs_throw(Value value, Context *context)
{
    assert(!context->engine->unwindStack.isEmpty());

    ExecutionEngine::ExceptionHandler handler = context->engine->unwindStack.last();

    // clean up call contexts
    while (context != handler.context) {
        context->leaveCallContext();
        context = context->parent;
    }

    handler.context->result = value;

    longjmp(handler.stackFrame, 1);
}

void *__qmljs_create_exception_handler(Context *context)
{
    context->engine->unwindStack.append(ExecutionEngine::ExceptionHandler());
    ExecutionEngine::ExceptionHandler *handler = &context->engine->unwindStack.last();
    handler->context = context;
    return handler->stackFrame;
}

void __qmljs_delete_exception_handler(Context *context)
{
    assert(!context->engine->unwindStack.isEmpty());

    context->engine->unwindStack.pop_back();
}

Value __qmljs_get_exception(Context *context)
{
    return context->result;
}

Value __qmljs_builtin_typeof(Value val, Context *context)
{
    return __qmljs_typeof(val, context);
}

void __qmljs_builtin_throw(Value val, Context *context)
{
    __qmljs_throw(val, context);
}


} // extern "C"


} // namespace VM
} // namespace QQmlJS
