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


#include "qv4ecmaobjects_p.h"
#include "qv4mm.h"
#include <QtCore/qnumeric.h>
#include <QtCore/qmath.h>
#include <QtCore/QDateTime>
#include <QtCore/QStringList>
#include <QtCore/QDebug>
#include <cassert>

#include <private/qqmljsengine_p.h>
#include <private/qqmljslexer_p.h>
#include <private/qqmljsparser_p.h>
#include <private/qqmljsast_p.h>
#include <qv4ir_p.h>
#include <qv4codegen_p.h>
#include <qv4isel_masm_p.h>

#ifndef Q_WS_WIN
#  include <time.h>
#  ifndef Q_OS_VXWORKS
#    include <sys/time.h>
#  else
#    include "qplatformdefs.h"
#  endif
#else
#  include <windows.h>
#endif

using namespace QQmlJS::VM;


//
// Object
//
ObjectCtor::ObjectCtor(ExecutionContext *scope)
    : FunctionObject(scope)
{
}

Value ObjectCtor::construct(ExecutionContext *ctx)
{
    if (!ctx->argumentCount || ctx->argument(0).isUndefined() || ctx->argument(0).isNull())
        return ctx->thisObject;
    return __qmljs_to_object(ctx->argument(0), ctx);
}

Value ObjectCtor::call(ExecutionContext *ctx)
{
    if (!ctx->argumentCount || ctx->argument(0).isUndefined() || ctx->argument(0).isNull())
        return Value::fromObject(ctx->engine->newObject());
    return __qmljs_to_object(ctx->argument(0), ctx);
}

void ObjectPrototype::init(ExecutionContext *ctx, const Value &ctor)
{
    ctor.objectValue()->defineReadonlyProperty(ctx->engine->id_prototype, Value::fromObject(this));
    ctor.objectValue()->defineReadonlyProperty(ctx->engine->id_length, Value::fromInt32(1));
    ctor.objectValue()->defineDefaultProperty(ctx, QStringLiteral("getPrototypeOf"), method_getPrototypeOf, 1);
    ctor.objectValue()->defineDefaultProperty(ctx, QStringLiteral("getOwnPropertyDescriptor"), method_getOwnPropertyDescriptor, 2);
    ctor.objectValue()->defineDefaultProperty(ctx, QStringLiteral("getOwnPropertyNames"), method_getOwnPropertyNames, 1);
    ctor.objectValue()->defineDefaultProperty(ctx, QStringLiteral("create"), method_create, 2);
    ctor.objectValue()->defineDefaultProperty(ctx, QStringLiteral("defineProperty"), method_defineProperty, 3);
    ctor.objectValue()->defineDefaultProperty(ctx, QStringLiteral("defineProperties"), method_defineProperties, 2);
    ctor.objectValue()->defineDefaultProperty(ctx, QStringLiteral("seal"), method_seal, 1);
    ctor.objectValue()->defineDefaultProperty(ctx, QStringLiteral("freeze"), method_freeze, 1);
    ctor.objectValue()->defineDefaultProperty(ctx, QStringLiteral("preventExtensions"), method_preventExtensions, 1);
    ctor.objectValue()->defineDefaultProperty(ctx, QStringLiteral("isSealed"), method_isSealed, 1);
    ctor.objectValue()->defineDefaultProperty(ctx, QStringLiteral("isFrozen"), method_isFrozen, 1);
    ctor.objectValue()->defineDefaultProperty(ctx, QStringLiteral("isExtensible"), method_isExtensible, 1);
    ctor.objectValue()->defineDefaultProperty(ctx, QStringLiteral("keys"), method_keys, 1);

    defineDefaultProperty(ctx, QStringLiteral("constructor"), ctor);
    defineDefaultProperty(ctx, QStringLiteral("toString"), method_toString, 0);
    defineDefaultProperty(ctx, QStringLiteral("toLocaleString"), method_toLocaleString, 0);
    defineDefaultProperty(ctx, QStringLiteral("valueOf"), method_valueOf, 0);
    defineDefaultProperty(ctx, QStringLiteral("hasOwnProperty"), method_hasOwnProperty, 1);
    defineDefaultProperty(ctx, QStringLiteral("isPrototypeOf"), method_isPrototypeOf, 1);
    defineDefaultProperty(ctx, QStringLiteral("propertyIsEnumerable"), method_propertyIsEnumerable, 1);
    defineDefaultProperty(ctx, QStringLiteral("__defineGetter__"), method_defineGetter, 0);
    defineDefaultProperty(ctx, QStringLiteral("__defineSetter__"), method_defineSetter, 0);
}

Value ObjectPrototype::method_getPrototypeOf(ExecutionContext *ctx)
{
    Value o = ctx->argument(0);
    if (! o.isObject())
        ctx->throwTypeError();

    Object *p = o.objectValue()->prototype;
    return p ? Value::fromObject(p) : Value::nullValue();
}

Value ObjectPrototype::method_getOwnPropertyDescriptor(ExecutionContext *ctx)
{
    Value O = ctx->argument(0);
    if (!O.isObject())
        ctx->throwTypeError();

    String *name = ctx->argument(1).toString(ctx);
    PropertyDescriptor *desc = O.objectValue()->__getOwnProperty__(ctx, name);
    return fromPropertyDescriptor(ctx, desc);
}

Value ObjectPrototype::method_getOwnPropertyNames(ExecutionContext *ctx)
{
    Object *O = ctx->argument(0).asObject();
    if (!O)
        ctx->throwTypeError();

    ArrayObject *array = ctx->engine->newArrayObject(ctx)->asArrayObject();
    Array &a = array->array;
    ObjectIterator it(ctx, O, ObjectIterator::NoFlags);
    while (1) {
        Value v = it.nextPropertyNameAsString();
        if (v.isNull())
            break;
        a.push_back(v);
    }
    return Value::fromObject(array);
}

Value ObjectPrototype::method_create(ExecutionContext *ctx)
{
    Value O = ctx->argument(0);
    if (!O.isObject() && !O.isNull())
        ctx->throwTypeError();

    Object *newObject = ctx->engine->newObject();
    newObject->prototype = O.objectValue();

    Value objValue = Value::fromObject(newObject);
    if (ctx->argumentCount > 1 && !ctx->argument(1).isUndefined()) {
        ctx->arguments[0] = objValue;
        method_defineProperties(ctx);
    }

    return objValue;
}

Value ObjectPrototype::method_defineProperty(ExecutionContext *ctx)
{
    Value O = ctx->argument(0);
    if (!O.isObject())
        ctx->throwTypeError();

    String *name = ctx->argument(1).toString(ctx);

    Value attributes = ctx->argument(2);
    PropertyDescriptor pd;
    toPropertyDescriptor(ctx, attributes, &pd);

    if (!O.objectValue()->__defineOwnProperty__(ctx, name, &pd))
        __qmljs_throw_type_error(ctx);

    return O;
}

Value ObjectPrototype::method_defineProperties(ExecutionContext *ctx)
{
    Value O = ctx->argument(0);
    if (!O.isObject())
        ctx->throwTypeError();

    Object *o = ctx->argument(1).toObject(ctx).objectValue();

    ObjectIterator it(ctx, o, ObjectIterator::EnumberableOnly);
    while (1) {
        uint index;
        String *name;
        PropertyDescriptor *pd = it.next(&name, &index);
        if (!pd)
            break;
        PropertyDescriptor n;
        toPropertyDescriptor(ctx, o->getValue(ctx, pd), &n);
        bool ok;
        if (name)
            ok = O.objectValue()->__defineOwnProperty__(ctx, name, &n);
        else
            ok = O.objectValue()->__defineOwnProperty__(ctx, index, &n);
        if (!ok)
            __qmljs_throw_type_error(ctx);
    }

    return O;
}

Value ObjectPrototype::method_seal(ExecutionContext *ctx)
{
    if (!ctx->argument(0).isObject())
        __qmljs_throw_type_error(ctx);

    Object *o = ctx->argument(0).objectValue();
    o->extensible = false;

    ObjectIterator it(ctx, o, ObjectIterator::NoFlags);
    while (1) {
        uint index;
        String *name;
        PropertyDescriptor *pd = it.next(&name, &index);
        if (!pd)
            break;
        pd->configurable = PropertyDescriptor::Disabled;
    }
    return ctx->argument(0);
}

Value ObjectPrototype::method_freeze(ExecutionContext *ctx)
{
    if (!ctx->argument(0).isObject())
        __qmljs_throw_type_error(ctx);

    Object *o = ctx->argument(0).objectValue();
    o->extensible = false;

    ObjectIterator it(ctx, o, ObjectIterator::NoFlags);
    while (1) {
        uint index;
        String *name;
        PropertyDescriptor *pd = it.next(&name, &index);
        if (!pd)
            break;
        if (pd->type == PropertyDescriptor::Data)
            pd->writable = PropertyDescriptor::Disabled;
        pd->configurable = PropertyDescriptor::Disabled;
    }
    return ctx->argument(0);
}

Value ObjectPrototype::method_preventExtensions(ExecutionContext *ctx)
{
    if (!ctx->argument(0).isObject())
        __qmljs_throw_type_error(ctx);

    Object *o = ctx->argument(0).objectValue();
    o->extensible = false;
    return ctx->argument(0);
}

Value ObjectPrototype::method_isSealed(ExecutionContext *ctx)
{
    if (!ctx->argument(0).isObject())
        __qmljs_throw_type_error(ctx);

    Object *o = ctx->argument(0).objectValue();
    if (o->extensible)
        return Value::fromBoolean(false);

    ObjectIterator it(ctx, o, ObjectIterator::NoFlags);
    while (1) {
        uint index;
        String *name;
        PropertyDescriptor *pd = it.next(&name, &index);
        if (!pd)
            break;
        if (pd->configurable != PropertyDescriptor::Disabled)
            return Value::fromBoolean(false);
    }
    return Value::fromBoolean(true);
}

Value ObjectPrototype::method_isFrozen(ExecutionContext *ctx)
{
    if (!ctx->argument(0).isObject())
        __qmljs_throw_type_error(ctx);

    Object *o = ctx->argument(0).objectValue();
    if (o->extensible)
        return Value::fromBoolean(false);

    ObjectIterator it(ctx, o, ObjectIterator::NoFlags);
    while (1) {
        uint index;
        String *name;
        PropertyDescriptor *pd = it.next(&name, &index);
        if (!pd)
            break;
            if (pd->isWritable() || pd->isConfigurable())
            return Value::fromBoolean(false);
    }
    return Value::fromBoolean(true);
}

Value ObjectPrototype::method_isExtensible(ExecutionContext *ctx)
{
    if (!ctx->argument(0).isObject())
        __qmljs_throw_type_error(ctx);

    Object *o = ctx->argument(0).objectValue();
    return Value::fromBoolean(o->extensible);
}

Value ObjectPrototype::method_keys(ExecutionContext *ctx)
{
    if (!ctx->argument(0).isObject())
        __qmljs_throw_type_error(ctx);

    Object *o = ctx->argument(0).objectValue();

    ArrayObject *a = ctx->engine->newArrayObject(ctx);

    ObjectIterator it(ctx, o, ObjectIterator::EnumberableOnly);
    while (1) {
        uint index;
        String *name;
        PropertyDescriptor *pd = it.next(&name, &index);
        if (!pd)
            break;
        Value key;
        if (name) {
            key = Value::fromString(name);
        } else {
            key = Value::fromDouble(index);
            key = __qmljs_to_string(key, ctx);
        }
        a->array.push_back(key);
    }

    return Value::fromObject(a);
}

Value ObjectPrototype::method_toString(ExecutionContext *ctx)
{
    if (ctx->thisObject.isUndefined()) {
        return Value::fromString(ctx, QStringLiteral("[object Undefined]"));
    } else if (ctx->thisObject.isNull()) {
        return Value::fromString(ctx, QStringLiteral("[object Null]"));
    } else {
        Value obj = __qmljs_to_object(ctx->thisObject, ctx);
        QString className = obj.objectValue()->className();
        return Value::fromString(ctx, QString::fromUtf8("[object %1]").arg(className));
    }
}

Value ObjectPrototype::method_toLocaleString(ExecutionContext *ctx)
{
    Object *o = __qmljs_to_object(ctx->thisObject, ctx).objectValue();
    Value ts = o->__get__(ctx, ctx->engine->identifier(QStringLiteral("toString")));
    FunctionObject *f = ts.asFunctionObject();
    if (!f)
        __qmljs_throw_type_error(ctx);
    return f->call(ctx, Value::fromObject(o), 0, 0);
}

Value ObjectPrototype::method_valueOf(ExecutionContext *ctx)
{
    return ctx->thisObject.toObject(ctx);
}

Value ObjectPrototype::method_hasOwnProperty(ExecutionContext *ctx)
{
    String *P = ctx->argument(0).toString(ctx);
    Value O = ctx->thisObject.toObject(ctx);
    bool r = O.objectValue()->__getOwnProperty__(ctx, P) != 0;
    return Value::fromBoolean(r);
}

Value ObjectPrototype::method_isPrototypeOf(ExecutionContext *ctx)
{
    Value V = ctx->argument(0);
    if (! V.isObject())
        return Value::fromBoolean(false);

    Object *O = ctx->thisObject.toObject(ctx).objectValue();
    Object *proto = V.objectValue()->prototype;
    while (proto) {
        if (O == proto)
            return Value::fromBoolean(true);
        proto = proto->prototype;
    }
    return Value::fromBoolean(false);
}

Value ObjectPrototype::method_propertyIsEnumerable(ExecutionContext *ctx)
{
    String *p = ctx->argument(0).toString(ctx);

    Object *o = ctx->thisObject.toObject(ctx).objectValue();
    PropertyDescriptor *pd = o->__getOwnProperty__(ctx, p);
    return Value::fromBoolean(pd && pd->isEnumerable());
}

Value ObjectPrototype::method_defineGetter(ExecutionContext *ctx)
{
    if (ctx->argumentCount < 2)
        __qmljs_throw_type_error(ctx);
    String *prop = ctx->argument(0).toString(ctx);

    FunctionObject *f = ctx->argument(1).asFunctionObject();
    if (!f)
        __qmljs_throw_type_error(ctx);

    Object *o = ctx->thisObject.toObject(ctx).objectValue();

    PropertyDescriptor pd = PropertyDescriptor::fromAccessor(f, 0);
    pd.configurable = PropertyDescriptor::Enabled;
    pd.enumberable = PropertyDescriptor::Enabled;
    o->__defineOwnProperty__(ctx, prop, &pd);
    return Value::undefinedValue();
}

Value ObjectPrototype::method_defineSetter(ExecutionContext *ctx)
{
    if (ctx->argumentCount < 2)
        __qmljs_throw_type_error(ctx);
    String *prop = ctx->argument(0).toString(ctx);

    FunctionObject *f = ctx->argument(1).asFunctionObject();
    if (!f)
        __qmljs_throw_type_error(ctx);

    Object *o = ctx->thisObject.toObject(ctx).objectValue();

    PropertyDescriptor pd = PropertyDescriptor::fromAccessor(0, f);
    pd.configurable = PropertyDescriptor::Enabled;
    pd.enumberable = PropertyDescriptor::Enabled;
    o->__defineOwnProperty__(ctx, prop, &pd);
    return Value::undefinedValue();
}

void ObjectPrototype::toPropertyDescriptor(ExecutionContext *ctx, Value v, PropertyDescriptor *desc)
{
    if (!v.isObject())
        __qmljs_throw_type_error(ctx);

    Object *o = v.objectValue();

    desc->type = PropertyDescriptor::Generic;

    desc->enumberable = PropertyDescriptor::Undefined;
    if (o->__hasProperty__(ctx, ctx->engine->id_enumerable))
        desc->enumberable = __qmljs_to_boolean(o->__get__(ctx, ctx->engine->id_enumerable), ctx) ? PropertyDescriptor::Enabled : PropertyDescriptor::Disabled;

    desc->configurable = PropertyDescriptor::Undefined;
    if (o->__hasProperty__(ctx, ctx->engine->id_configurable))
        desc->configurable = __qmljs_to_boolean(o->__get__(ctx, ctx->engine->id_configurable), ctx) ? PropertyDescriptor::Enabled : PropertyDescriptor::Disabled;

    desc->get = 0;
    if (o->__hasProperty__(ctx, ctx->engine->id_get)) {
        Value get = o->__get__(ctx, ctx->engine->id_get);
        FunctionObject *f = get.asFunctionObject();
        if (f) {
            desc->get = f;
        } else if (get.isUndefined()) {
            desc->get = (FunctionObject *)0x1;
        } else {
            __qmljs_throw_type_error(ctx);
        }
        desc->type = PropertyDescriptor::Accessor;
    }

    desc->set = 0;
    if (o->__hasProperty__(ctx, ctx->engine->id_set)) {
        Value set = o->__get__(ctx, ctx->engine->id_set);
        FunctionObject *f = set.asFunctionObject();
        if (f) {
            desc->set = f;
        } else if (set.isUndefined()) {
            desc->set = (FunctionObject *)0x1;
        } else {
            __qmljs_throw_type_error(ctx);
        }
        desc->type = PropertyDescriptor::Accessor;
    }

    desc->writable = PropertyDescriptor::Undefined;
    if (o->__hasProperty__(ctx, ctx->engine->id_writable)) {
        if (desc->isAccessor())
            __qmljs_throw_type_error(ctx);
        desc->writable = __qmljs_to_boolean(o->__get__(ctx, ctx->engine->id_writable), ctx) ? PropertyDescriptor::Enabled : PropertyDescriptor::Disabled;
        // writable forces it to be a data descriptor
        desc->value = Value::undefinedValue();
    }

    if (o->__hasProperty__(ctx, ctx->engine->id_value)) {
        if (desc->isAccessor())
            __qmljs_throw_type_error(ctx);
        desc->value = o->__get__(ctx, ctx->engine->id_value);
        desc->type = PropertyDescriptor::Data;
    }

}


Value ObjectPrototype::fromPropertyDescriptor(ExecutionContext *ctx, const PropertyDescriptor *desc)
{
    if (!desc)
        return Value::undefinedValue();

    ExecutionEngine *engine = ctx->engine;
//    Let obj be the result of creating a new object as if by the expression new Object() where Object is the standard built-in constructor with that name.
    Object *o = engine->newObject();

    PropertyDescriptor pd;
    pd.type = PropertyDescriptor::Data;
    pd.writable = PropertyDescriptor::Enabled;
    pd.enumberable = PropertyDescriptor::Enabled;
    pd.configurable = PropertyDescriptor::Enabled;

    if (desc->isData()) {
        pd.value = desc->value;
        o->__defineOwnProperty__(ctx, engine->identifier(QStringLiteral("value")), &pd);
        pd.value = Value::fromBoolean(desc->writable == PropertyDescriptor::Enabled ? true : false);
        o->__defineOwnProperty__(ctx, engine->identifier(QStringLiteral("writable")), &pd);
    } else {
        pd.value = desc->get ? Value::fromObject(desc->get) : Value::undefinedValue();
        o->__defineOwnProperty__(ctx, engine->identifier(QStringLiteral("get")), &pd);
        pd.value = desc->set ? Value::fromObject(desc->set) : Value::undefinedValue();
        o->__defineOwnProperty__(ctx, engine->identifier(QStringLiteral("set")), &pd);
    }
    pd.value = Value::fromBoolean(desc->enumberable == PropertyDescriptor::Enabled ? true : false);
    o->__defineOwnProperty__(ctx, engine->identifier(QStringLiteral("enumerable")), &pd);
    pd.value = Value::fromBoolean(desc->configurable == PropertyDescriptor::Enabled ? true : false);
    o->__defineOwnProperty__(ctx, engine->identifier(QStringLiteral("configurable")), &pd);

    return Value::fromObject(o);
}


//
// Number object
//
NumberCtor::NumberCtor(ExecutionContext *scope)
    : FunctionObject(scope)
{
}

Value NumberCtor::construct(ExecutionContext *ctx)
{
    double d = ctx->argumentCount ? ctx->argument(0).toNumber(ctx) : 0;
    return Value::fromObject(ctx->engine->newNumberObject(Value::fromDouble(d)));
}

Value NumberCtor::call(ExecutionContext *ctx)
{
    double d = ctx->argumentCount ? ctx->argument(0).toNumber(ctx) : 0;
    return Value::fromDouble(d);
}

void NumberPrototype::init(ExecutionContext *ctx, const Value &ctor)
{
    ctor.objectValue()->defineReadonlyProperty(ctx->engine->id_prototype, Value::fromObject(this));
    ctor.objectValue()->defineReadonlyProperty(ctx->engine->id_length, Value::fromInt32(1));

    ctor.objectValue()->defineReadonlyProperty(ctx->engine, QStringLiteral("NaN"), Value::fromDouble(qSNaN()));
    ctor.objectValue()->defineReadonlyProperty(ctx->engine, QStringLiteral("NEGATIVE_INFINITY"), Value::fromDouble(-qInf()));
    ctor.objectValue()->defineReadonlyProperty(ctx->engine, QStringLiteral("POSITIVE_INFINITY"), Value::fromDouble(qInf()));
    ctor.objectValue()->defineReadonlyProperty(ctx->engine, QStringLiteral("MAX_VALUE"), Value::fromDouble(1.7976931348623158e+308));

#ifdef __INTEL_COMPILER
# pragma warning( push )
# pragma warning(disable: 239)
#endif
    ctor.objectValue()->defineReadonlyProperty(ctx->engine, QStringLiteral("MIN_VALUE"), Value::fromDouble(5e-324));
#ifdef __INTEL_COMPILER
# pragma warning( pop )
#endif

    defineDefaultProperty(ctx, QStringLiteral("constructor"), ctor);
    defineDefaultProperty(ctx, QStringLiteral("toString"), method_toString);
    defineDefaultProperty(ctx, QStringLiteral("toLocalString"), method_toLocaleString);
    defineDefaultProperty(ctx, QStringLiteral("valueOf"), method_valueOf);
    defineDefaultProperty(ctx, QStringLiteral("toFixed"), method_toFixed, 1);
    defineDefaultProperty(ctx, QStringLiteral("toExponential"), method_toExponential);
    defineDefaultProperty(ctx, QStringLiteral("toPrecision"), method_toPrecision);
}

Value NumberPrototype::method_toString(ExecutionContext *ctx)
{
    NumberObject *thisObject = ctx->thisObject.asNumberObject();
    if (!thisObject)
        ctx->throwTypeError();

    Value arg = ctx->argument(0);
    if (!arg.isUndefined()) {
        int radix = arg.toInt32(ctx);
        if (radix < 2 || radix > 36) {
            ctx->throwError(QString::fromLatin1("Number.prototype.toString: %0 is not a valid radix")
                            .arg(radix));
            return Value::undefinedValue();
        }

        double num = thisObject->value.asDouble();
        if (std::isnan(num)) {
            return Value::fromString(ctx, QStringLiteral("NaN"));
        } else if (qIsInf(num)) {
            return Value::fromString(ctx, QLatin1String(num < 0 ? "-Infinity" : "Infinity"));
        }

        if (radix != 10) {
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
            return Value::fromString(ctx, str);
        }
    }

    Value internalValue = thisObject->value;
    String *str = internalValue.toString(ctx);
    return Value::fromString(str);
}

Value NumberPrototype::method_toLocaleString(ExecutionContext *ctx)
{
    NumberObject *thisObject = ctx->thisObject.asNumberObject();
    if (!thisObject)
        ctx->throwTypeError();

    String *str = thisObject->value.toString(ctx);
    return Value::fromString(str);
}

Value NumberPrototype::method_valueOf(ExecutionContext *ctx)
{
    NumberObject *thisObject = ctx->thisObject.asNumberObject();
    if (!thisObject)
        ctx->throwTypeError();

    return thisObject->value;
}

Value NumberPrototype::method_toFixed(ExecutionContext *ctx)
{
    NumberObject *thisObject = ctx->thisObject.asNumberObject();
    if (!thisObject)
        ctx->throwTypeError();

    double fdigits = 0;

    if (ctx->argumentCount > 0)
        fdigits = ctx->argument(0).toInteger(ctx);

    if (std::isnan(fdigits))
        fdigits = 0;

    double v = thisObject->value.asDouble();
    QString str;
    if (std::isnan(v))
        str = QString::fromLatin1("NaN");
    else if (qIsInf(v))
        str = QString::fromLatin1(v < 0 ? "-Infinity" : "Infinity");
    else
        str = QString::number(v, 'f', int (fdigits));
    return Value::fromString(ctx, str);
}

Value NumberPrototype::method_toExponential(ExecutionContext *ctx)
{
    NumberObject *thisObject = ctx->thisObject.asNumberObject();
    if (!thisObject)
        ctx->throwTypeError();

    double fdigits = 0;

    if (ctx->argumentCount > 0)
        fdigits = ctx->argument(0).toInteger(ctx);

    QString z = QString::number(thisObject->value.asDouble(), 'e', int (fdigits));
    return Value::fromString(ctx, z);
}

Value NumberPrototype::method_toPrecision(ExecutionContext *ctx)
{
    NumberObject *thisObject = ctx->thisObject.asNumberObject();
    if (!thisObject)
        ctx->throwTypeError();

    double fdigits = 0;

    if (ctx->argumentCount > 0)
        fdigits = ctx->argument(0).toInteger(ctx);

    return Value::fromString(ctx, QString::number(thisObject->value.asDouble(), 'g', int (fdigits)));
}

//
// Boolean object
//
BooleanCtor::BooleanCtor(ExecutionContext *scope)
    : FunctionObject(scope)
{
}

Value BooleanCtor::construct(ExecutionContext *ctx)
{
    const double n = ctx->argument(0).toBoolean(ctx);
    return Value::fromObject(ctx->engine->newBooleanObject(Value::fromBoolean(n)));
}

Value BooleanCtor::call(ExecutionContext *ctx)
{
    bool value = ctx->argumentCount ? ctx->argument(0).toBoolean(ctx) : 0;
    return Value::fromBoolean(value);
}

void BooleanPrototype::init(ExecutionContext *ctx, const Value &ctor)
{
    ctor.objectValue()->defineReadonlyProperty(ctx->engine->id_prototype, Value::fromObject(this));
    ctor.objectValue()->defineReadonlyProperty(ctx->engine->id_length, Value::fromInt32(1));
    defineDefaultProperty(ctx, QStringLiteral("constructor"), ctor);
    defineDefaultProperty(ctx, QStringLiteral("toString"), method_toString);
    defineDefaultProperty(ctx, QStringLiteral("valueOf"), method_valueOf);
}

Value BooleanPrototype::method_toString(ExecutionContext *ctx)
{
    BooleanObject *thisObject = ctx->thisObject.asBooleanObject();
    if (!thisObject)
        ctx->throwTypeError();

    return Value::fromString(ctx, QLatin1String(thisObject->value.booleanValue() ? "true" : "false"));
}

Value BooleanPrototype::method_valueOf(ExecutionContext *ctx)
{
    BooleanObject *thisObject = ctx->thisObject.asBooleanObject();
    if (!thisObject)
        ctx->throwTypeError();

    return thisObject->value;
}
