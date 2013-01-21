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
#include <cmath>
#include <qmath.h>
#include <qnumeric.h>
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

static const double qt_PI = 2.0 * ::asin(1.0);

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
// String
//
StringCtor::StringCtor(ExecutionContext *scope)
    : FunctionObject(scope)
{
}

Value StringCtor::construct(ExecutionContext *ctx)
{
    Value value;
    if (ctx->argumentCount)
        value = Value::fromString(ctx->argument(0).toString(ctx));
    else
        value = Value::fromString(ctx, QString());
    return Value::fromObject(ctx->engine->newStringObject(ctx, value));
}

Value StringCtor::call(ExecutionContext *ctx)
{
    Value value;
    if (ctx->argumentCount)
        value = Value::fromString(ctx->argument(0).toString(ctx));
    else
        value = Value::fromString(ctx, QString());
    return value;
}

void StringPrototype::init(ExecutionContext *ctx, const Value &ctor)
{
    ctor.objectValue()->defineReadonlyProperty(ctx->engine->id_prototype, Value::fromObject(this));
    ctor.objectValue()->defineReadonlyProperty(ctx->engine->id_length, Value::fromInt32(1));
    ctor.objectValue()->defineDefaultProperty(ctx, QStringLiteral("fromCharCode"), method_fromCharCode, 1);

    defineDefaultProperty(ctx, QStringLiteral("constructor"), ctor);
    defineDefaultProperty(ctx, QStringLiteral("toString"), method_toString);
    defineDefaultProperty(ctx, QStringLiteral("valueOf"), method_valueOf);
    defineDefaultProperty(ctx, QStringLiteral("charAt"), method_charAt, 1);
    defineDefaultProperty(ctx, QStringLiteral("charCodeAt"), method_charCodeAt, 1);
    defineDefaultProperty(ctx, QStringLiteral("concat"), method_concat, 1);
    defineDefaultProperty(ctx, QStringLiteral("indexOf"), method_indexOf, 1);
    defineDefaultProperty(ctx, QStringLiteral("lastIndexOf"), method_lastIndexOf, 1);
    defineDefaultProperty(ctx, QStringLiteral("localeCompare"), method_localeCompare, 1);
    defineDefaultProperty(ctx, QStringLiteral("match"), method_match, 1);
    defineDefaultProperty(ctx, QStringLiteral("replace"), method_replace, 2);
    defineDefaultProperty(ctx, QStringLiteral("search"), method_search, 1);
    defineDefaultProperty(ctx, QStringLiteral("slice"), method_slice, 2);
    defineDefaultProperty(ctx, QStringLiteral("split"), method_split, 2);
    defineDefaultProperty(ctx, QStringLiteral("substr"), method_substr, 2);
    defineDefaultProperty(ctx, QStringLiteral("substring"), method_substring, 2);
    defineDefaultProperty(ctx, QStringLiteral("toLowerCase"), method_toLowerCase);
    defineDefaultProperty(ctx, QStringLiteral("toLocaleLowerCase"), method_toLocaleLowerCase);
    defineDefaultProperty(ctx, QStringLiteral("toUpperCase"), method_toUpperCase);
    defineDefaultProperty(ctx, QStringLiteral("toLocaleUpperCase"), method_toLocaleUpperCase);
    defineDefaultProperty(ctx, QStringLiteral("trim"), method_trim);
}

QString StringPrototype::getThisString(ExecutionContext *ctx)
{
    String* str = 0;
    Value thisObject = ctx->thisObject;
    if (StringObject *thisString = thisObject.asStringObject())
        str = thisString->value.stringValue();
    else if (thisObject.isUndefined() || thisObject.isNull())
        ctx->throwTypeError();
    else
        str = ctx->thisObject.toString(ctx);
    return str->toQString();
}

Value StringPrototype::method_toString(ExecutionContext *ctx)
{
    StringObject *o = ctx->thisObject.asStringObject();
    if (!o)
        ctx->throwTypeError();
    return o->value;
}

Value StringPrototype::method_valueOf(ExecutionContext *ctx)
{
    StringObject *o = ctx->thisObject.asStringObject();
    if (!o)
        ctx->throwTypeError();
    return o->value;
}

Value StringPrototype::method_charAt(ExecutionContext *ctx)
{
    const QString str = getThisString(ctx);

    int pos = 0;
    if (ctx->argumentCount > 0)
        pos = (int) ctx->argument(0).toInteger(ctx);

    QString result;
    if (pos >= 0 && pos < str.length())
        result += str.at(pos);

    return Value::fromString(ctx, result);
}

Value StringPrototype::method_charCodeAt(ExecutionContext *ctx)
{
    const QString str = getThisString(ctx);

    int pos = 0;
    if (ctx->argumentCount > 0)
        pos = (int) ctx->argument(0).toInteger(ctx);

    double result = qSNaN();

    if (pos >= 0 && pos < str.length())
        result = str.at(pos).unicode();

    return Value::fromDouble(result);
}

Value StringPrototype::method_concat(ExecutionContext *ctx)
{
    QString value = getThisString(ctx);

    for (unsigned i = 0; i < ctx->argumentCount; ++i) {
        Value v = __qmljs_to_string(ctx->argument(i), ctx);
        assert(v.isString());
        value += v.stringValue()->toQString();
    }

    return Value::fromString(ctx, value);
}

Value StringPrototype::method_indexOf(ExecutionContext *ctx)
{
    QString value = getThisString(ctx);

    QString searchString;
    if (ctx->argumentCount)
        searchString = ctx->argument(0).toString(ctx)->toQString();

    int pos = 0;
    if (ctx->argumentCount > 1)
        pos = (int) ctx->argument(1).toInteger(ctx);

    int index = -1;
    if (! value.isEmpty())
        index = value.indexOf(searchString, qMin(qMax(pos, 0), value.length()));

    return Value::fromDouble(index);
}

Value StringPrototype::method_lastIndexOf(ExecutionContext *ctx)
{
    const QString value = getThisString(ctx);

    QString searchString;
    if (ctx->argumentCount) {
        Value v = __qmljs_to_string(ctx->argument(0), ctx);
        searchString = v.stringValue()->toQString();
    }

    Value posArg = ctx->argument(1);
    double position = __qmljs_to_number(posArg, ctx);
    if (std::isnan(position))
        position = +qInf();
    else
        position = trunc(position);

    int pos = trunc(qMin(qMax(position, 0.0), double(value.length())));
    if (!searchString.isEmpty() && pos == value.length())
        --pos;
    int index = value.lastIndexOf(searchString, pos);
    return Value::fromDouble(index);
}

Value StringPrototype::method_localeCompare(ExecutionContext *ctx)
{
    const QString value = getThisString(ctx);
    const QString that = ctx->argument(0).toString(ctx)->toQString();
    return Value::fromDouble(QString::localeAwareCompare(value, that));
}

Value StringPrototype::method_match(ExecutionContext *ctx)
{
    if (ctx->thisObject.isUndefined() || ctx->thisObject.isNull())
        __qmljs_throw_type_error(ctx);

    String *s = ctx->thisObject.toString(ctx);

    Value regexp = ctx->argument(0);
    RegExpObject *rx = regexp.asRegExpObject();
    if (!rx)
        rx = ctx->engine->regExpCtor.asFunctionObject()->construct(ctx, &regexp, 1).asRegExpObject();

    if (!rx)
        // ### CHECK
        __qmljs_throw_type_error(ctx);

    bool global = rx->global;

    FunctionObject *exec = ctx->engine->regExpPrototype->__get__(ctx, ctx->engine->identifier(QStringLiteral("exec")), 0).asFunctionObject();

    Value arg = Value::fromString(s);
    if (!global)
        return exec->call(ctx, Value::fromObject(rx), &arg, 1);

    String *lastIndex = ctx->engine->identifier(QStringLiteral("lastIndex"));
    rx->__put__(ctx, lastIndex, Value::fromDouble(0.));
    ArrayObject *a = ctx->engine->newArrayObject(ctx);

    double previousLastIndex = 0;
    uint n = 0;
    while (1) {
        Value result = exec->call(ctx, Value::fromObject(rx), &arg, 1);
        if (result.isNull())
            break;
        assert(result.isObject());
        double thisIndex = rx->__get__(ctx, lastIndex, 0).toInteger(ctx);
        if (previousLastIndex == thisIndex) {
            previousLastIndex = thisIndex + 1;
            rx->__put__(ctx, lastIndex, Value::fromDouble(previousLastIndex));
        } else {
            previousLastIndex = thisIndex;
        }
        Value matchStr = result.objectValue()->__get__(ctx, (uint)0, (bool *)0);
        a->array.set(n, matchStr);
        ++n;
    }
    if (!n)
        return Value::nullValue();

    return Value::fromObject(a);

}

static QString makeReplacementString(const QString &input, const QString& replaceValue, uint* matchOffsets, int captureCount)
{
    QString result;
    result.reserve(replaceValue.length());
    for (int i = 0; i < replaceValue.length(); ++i) {
        if (replaceValue.at(i) == QLatin1Char('$') && i < replaceValue.length() - 1) {
            char ch = replaceValue.at(++i).toLatin1();
            uint substStart = JSC::Yarr::offsetNoMatch;
            uint substEnd = JSC::Yarr::offsetNoMatch;
            if (ch == '$') {
                result += ch;
                continue;
            } else if (ch == '&') {
                substStart = matchOffsets[0];
                substEnd = matchOffsets[1];
            } else if (ch == '`') {
                substStart = 0;
                substEnd = matchOffsets[0];
            } else if (ch == '\'') {
                substStart = matchOffsets[1];
                substEnd = input.length();
            } else if (ch >= '1' && ch <= '9') {
                char capture = ch - '0';
                if (capture > 0 && capture < captureCount) {
                    substStart = matchOffsets[capture * 2];
                    substEnd = matchOffsets[capture * 2 + 1];
                }
            } else if (ch == '0' && i < replaceValue.length() - 1) {
                int capture = (ch - '0') * 10;
                ch = replaceValue.at(++i).toLatin1();
                capture += ch - '0';
                if (capture > 0 && capture < captureCount) {
                    substStart = matchOffsets[capture * 2];
                    substEnd = matchOffsets[capture * 2 + 1];
                }
            }
            if (substStart != JSC::Yarr::offsetNoMatch && substEnd != JSC::Yarr::offsetNoMatch)
                result += input.midRef(substStart, substEnd - substStart);
        } else {
            result += replaceValue.at(i);
        }
    }
    return result;
}

Value StringPrototype::method_replace(ExecutionContext *ctx)
{
    QString string;
    if (StringObject *thisString = ctx->thisObject.asStringObject())
        string = thisString->value.stringValue()->toQString();
    else
        string = ctx->thisObject.toString(ctx)->toQString();

    int numCaptures = 0;
    QVarLengthArray<uint, 16> matchOffsets;
    int numStringMatches = 0;

    Value searchValue = ctx->argument(0);
    RegExpObject *regExp = searchValue.asRegExpObject();
    if (regExp) {
        uint offset = 0;
        while (true) {
            int oldSize = matchOffsets.size();
            matchOffsets.resize(matchOffsets.size() + regExp->value->captureCount() * 2);
            if (regExp->value->match(string, offset, matchOffsets.data() + oldSize) == JSC::Yarr::offsetNoMatch) {
                matchOffsets.resize(oldSize);
                break;
            }
            if (!regExp->global)
                break;
            offset = qMax(offset + 1, matchOffsets[oldSize + 1]);
        }
        if (regExp->global)
            regExp->lastIndexProperty->value = Value::fromUInt32(0);
        numStringMatches = matchOffsets.size() / (regExp->value->captureCount() * 2);
        numCaptures = regExp->value->captureCount();
    } else {
        numCaptures = 1;
        QString searchString = searchValue.toString(ctx)->toQString();
        int idx = string.indexOf(searchString);
        if (idx != -1) {
            numStringMatches = 1;
            matchOffsets.resize(2);
            matchOffsets[0] = idx;
            matchOffsets[1] = idx + searchString.length();
        }
    }

    QString result = string;
    Value replaceValue = ctx->argument(1);
    if (FunctionObject* searchCallback = replaceValue.asFunctionObject()) {
        int replacementDelta = 0;
        int argc = numCaptures + 2;
        Value *args = (Value*)alloca((numCaptures + 2) * sizeof(Value));
        for (int i = 0; i < numStringMatches; ++i) {
            for (int k = 0; k < numCaptures; ++k) {
                int idx = (i * numCaptures + k) * 2;
                uint start = matchOffsets[idx];
                uint end = matchOffsets[idx + 1];
                Value entry = Value::undefinedValue();
                if (start != JSC::Yarr::offsetNoMatch && end != JSC::Yarr::offsetNoMatch)
                    entry = Value::fromString(ctx, string.mid(start, end - start));
                args[k] = entry;
            }
            uint matchStart = matchOffsets[i * numCaptures * 2];
            uint matchEnd = matchOffsets[i * numCaptures * 2 + 1];
            args[numCaptures] = Value::fromUInt32(matchStart);
            args[numCaptures + 1] = Value::fromString(ctx, string);
            Value replacement = searchCallback->call(ctx, Value::undefinedValue(), args, argc);
            QString replacementString = replacement.toString(ctx)->toQString();
            result.replace(replacementDelta + matchStart, matchEnd - matchStart, replacementString);
            replacementDelta += replacementString.length() - matchEnd + matchStart;
        }
    } else {
        QString newString = replaceValue.toString(ctx)->toQString();
        int replacementDelta = 0;

        for (int i = 0; i < numStringMatches; ++i) {
            int baseIndex = i * numCaptures * 2;
            uint matchStart = matchOffsets[baseIndex];
            uint matchEnd = matchOffsets[baseIndex + 1];
            if (matchStart == JSC::Yarr::offsetNoMatch)
                continue;

            QString replacement = makeReplacementString(string, newString, matchOffsets.data() + baseIndex, numCaptures);
            result.replace(replacementDelta + matchStart, matchEnd - matchStart, replacement);
            replacementDelta += replacement.length() - matchEnd + matchStart;
        }
    }

    return Value::fromString(ctx, result);
}

Value StringPrototype::method_search(ExecutionContext *ctx)
{
    // requires Regexp
    ctx->throwUnimplemented(QStringLiteral("String.prototype.search"));
    return Value::undefinedValue();
}

Value StringPrototype::method_slice(ExecutionContext *ctx)
{
    const QString text = getThisString(ctx);
    const int length = text.length();

    int start = int (ctx->argument(0).toInteger(ctx));
    int end = ctx->argument(1).isUndefined()
            ? length : int (ctx->argument(1).toInteger(ctx));

    if (start < 0)
        start = qMax(length + start, 0);
    else
        start = qMin(start, length);

    if (end < 0)
        end = qMax(length + end, 0);
    else
        end = qMin(end, length);

    int count = qMax(0, end - start);
    return Value::fromString(ctx, text.mid(start, count));
}

Value StringPrototype::method_split(ExecutionContext *ctx)
{
    ctx->throwUnimplemented(QStringLiteral("String.prototype.splt"));
    return Value::undefinedValue();
}

Value StringPrototype::method_substr(ExecutionContext *ctx)
{
    const QString value = getThisString(ctx);

    double start = 0;
    if (ctx->argumentCount > 0)
        start = ctx->argument(0).toInteger(ctx);

    double length = +qInf();
    if (ctx->argumentCount > 1)
        length = ctx->argument(1).toInteger(ctx);

    double count = value.length();
    if (start < 0)
        start = qMax(count + start, 0.0);

    length = qMin(qMax(length, 0.0), count - start);

    qint32 x = Value::toInt32(start);
    qint32 y = Value::toInt32(length);
    return Value::fromString(ctx, value.mid(x, y));
}

Value StringPrototype::method_substring(ExecutionContext *ctx)
{
    QString value = getThisString(ctx);
    int length = value.length();

    double start = 0;
    double end = length;

    if (ctx->argumentCount > 0)
        start = ctx->argument(0).toInteger(ctx);

    if (ctx->argumentCount > 1)
        end = ctx->argument(1).toInteger(ctx);

    if (std::isnan(start) || start < 0)
        start = 0;

    if (std::isnan(end) || end < 0)
        end = 0;

    if (start > length)
        start = length;

    if (end > length)
        end = length;

    if (start > end) {
        double was = start;
        start = end;
        end = was;
    }

    qint32 x = Value::toInt32(start);
    qint32 y = Value::toInt32(end - start);
    return Value::fromString(ctx, value.mid(x, y));
}

Value StringPrototype::method_toLowerCase(ExecutionContext *ctx)
{
    QString value = getThisString(ctx);
    return Value::fromString(ctx, value.toLower());
}

Value StringPrototype::method_toLocaleLowerCase(ExecutionContext *ctx)
{
    return method_toLowerCase(ctx);
}

Value StringPrototype::method_toUpperCase(ExecutionContext *ctx)
{
    QString value = getThisString(ctx);
    return Value::fromString(ctx, value.toUpper());
}

Value StringPrototype::method_toLocaleUpperCase(ExecutionContext *ctx)
{
    return method_toUpperCase(ctx);
}

Value StringPrototype::method_fromCharCode(ExecutionContext *ctx)
{
    QString str;
    for (unsigned i = 0; i < ctx->argumentCount; ++i) {
        QChar c(ctx->argument(i).toUInt16(ctx));
        str += c;
    }
    return Value::fromString(ctx, str);
}

Value StringPrototype::method_trim(ExecutionContext *ctx)
{
    if (ctx->thisObject.isNull() || ctx->thisObject.isUndefined())
        __qmljs_throw_type_error(ctx);

    QString s = __qmljs_to_string(ctx->thisObject, ctx).stringValue()->toQString();
    const QChar *chars = s.constData();
    int start, end;
    for (start = 0; start < s.length(); ++start) {
        if (!chars[start].isSpace() && chars[start].unicode() != 0xfeff)
            break;
    }
    for (end = s.length() - 1; end >= start; --end) {
        if (!chars[end].isSpace() && chars[end].unicode() != 0xfeff)
            break;
    }

    return Value::fromString(ctx, QString(chars + start, end - start + 1));
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

//
// Array object
//
ArrayCtor::ArrayCtor(ExecutionContext *scope)
    : FunctionObject(scope)
{
}

Value ArrayCtor::call(ExecutionContext *ctx)
{
    ArrayObject *a = ctx->engine->newArrayObject(ctx);
    Array &value = a->array;
    if (ctx->argumentCount == 1 && ctx->argument(0).isNumber()) {
        bool ok;
        uint len = ctx->argument(0).asArrayLength(ctx, &ok);

        if (!ok) {
            ctx->throwRangeError(ctx->argument(0));
            return Value::undefinedValue();
        }

        value.setLengthUnchecked(len);
    } else {
        for (unsigned int i = 0; i < ctx->argumentCount; ++i) {
            value.set(i, ctx->argument(i));
        }
    }

    return Value::fromObject(a);
}

void ArrayPrototype::init(ExecutionContext *ctx, const Value &ctor)
{
    ctor.objectValue()->defineReadonlyProperty(ctx->engine->id_prototype, Value::fromObject(this));
    ctor.objectValue()->defineReadonlyProperty(ctx->engine->id_length, Value::fromInt32(1));
    ctor.objectValue()->defineDefaultProperty(ctx, QStringLiteral("isArray"), method_isArray, 1);
    defineDefaultProperty(ctx, QStringLiteral("constructor"), ctor);
    defineDefaultProperty(ctx, QStringLiteral("toString"), method_toString, 0);
    defineDefaultProperty(ctx, QStringLiteral("toLocalString"), method_toLocaleString, 0);
    defineDefaultProperty(ctx, QStringLiteral("concat"), method_concat, 1);
    defineDefaultProperty(ctx, QStringLiteral("join"), method_join, 1);
    defineDefaultProperty(ctx, QStringLiteral("pop"), method_pop, 0);
    defineDefaultProperty(ctx, QStringLiteral("push"), method_push, 1);
    defineDefaultProperty(ctx, QStringLiteral("reverse"), method_reverse, 0);
    defineDefaultProperty(ctx, QStringLiteral("shift"), method_shift, 0);
    defineDefaultProperty(ctx, QStringLiteral("slice"), method_slice, 2);
    defineDefaultProperty(ctx, QStringLiteral("sort"), method_sort, 1);
    defineDefaultProperty(ctx, QStringLiteral("splice"), method_splice, 2);
    defineDefaultProperty(ctx, QStringLiteral("unshift"), method_unshift, 1);
    defineDefaultProperty(ctx, QStringLiteral("indexOf"), method_indexOf, 1);
    defineDefaultProperty(ctx, QStringLiteral("lastIndexOf"), method_lastIndexOf, 1);
    defineDefaultProperty(ctx, QStringLiteral("every"), method_every, 1);
    defineDefaultProperty(ctx, QStringLiteral("some"), method_some, 1);
    defineDefaultProperty(ctx, QStringLiteral("forEach"), method_forEach, 1);
    defineDefaultProperty(ctx, QStringLiteral("map"), method_map, 1);
    defineDefaultProperty(ctx, QStringLiteral("filter"), method_filter, 1);
    defineDefaultProperty(ctx, QStringLiteral("reduce"), method_reduce, 1);
    defineDefaultProperty(ctx, QStringLiteral("reduceRight"), method_reduceRight, 1);
}

uint ArrayPrototype::getLength(ExecutionContext *ctx, Object *o)
{
    if (o->isArray)
        return o->array.length();
    return o->__get__(ctx, ctx->engine->id_length).toUInt32(ctx);
}

Value ArrayPrototype::method_isArray(ExecutionContext *ctx)
{
    Value arg = ctx->argument(0);
    bool isArray = arg.asArrayObject();
    return Value::fromBoolean(isArray);
}

Value ArrayPrototype::method_toString(ExecutionContext *ctx)
{
    return method_join(ctx);
}

Value ArrayPrototype::method_toLocaleString(ExecutionContext *ctx)
{
    return method_toString(ctx);
}

Value ArrayPrototype::method_concat(ExecutionContext *ctx)
{
    Array result;

    if (ArrayObject *instance = ctx->thisObject.asArrayObject())
        result = instance->array;
    else {
        QString v = ctx->thisObject.toString(ctx)->toQString();
        result.set(0, Value::fromString(ctx, v));
    }

    for (uint i = 0; i < ctx->argumentCount; ++i) {
        quint32 k = result.length();
        Value arg = ctx->argument(i);

        if (ArrayObject *elt = arg.asArrayObject())
            result.concat(elt->array);

        else
            result.set(k, arg);
    }

    return Value::fromObject(ctx->engine->newArrayObject(ctx, result));
}

Value ArrayPrototype::method_join(ExecutionContext *ctx)
{
    Value arg = ctx->argument(0);

    QString r4;
    if (arg.isUndefined())
        r4 = QStringLiteral(",");
    else
        r4 = arg.toString(ctx)->toQString();

    Value self = ctx->thisObject;
    const Value length = self.property(ctx, ctx->engine->id_length);
    const quint32 r2 = Value::toUInt32(length.isUndefined() ? 0 : length.toNumber(ctx));

    static QSet<Object *> visitedArrayElements;

    if (! r2 || visitedArrayElements.contains(self.objectValue()))
        return Value::fromString(ctx, QString());

    // avoid infinite recursion
    visitedArrayElements.insert(self.objectValue());

    QString R;

    // ### FIXME
    if (ArrayObject *a = self.asArrayObject()) {
        for (uint i = 0; i < a->array.length(); ++i) {
            if (i)
                R += r4;

            Value e = a->__get__(ctx, i);
            if (! (e.isUndefined() || e.isNull()))
                R += e.toString(ctx)->toQString();
        }
    } else {
        //
        // crazy!
        //
        Value r6 = self.property(ctx, ctx->engine->identifier(QStringLiteral("0")));
        if (!(r6.isUndefined() || r6.isNull()))
            R = r6.toString(ctx)->toQString();

        for (quint32 k = 1; k < r2; ++k) {
            R += r4;

            String *name = Value::fromDouble(k).toString(ctx);
            Value r12 = self.property(ctx, name);

            if (! (r12.isUndefined() || r12.isNull()))
                R += r12.toString(ctx)->toQString();
        }
    }

    visitedArrayElements.remove(self.objectValue());
    return Value::fromString(ctx, R);
}

Value ArrayPrototype::method_pop(ExecutionContext *ctx)
{
    Value self = ctx->thisObject;
    if (ArrayObject *instance = self.asArrayObject()) {
        Value v = instance->getValueChecked(ctx, instance->array.back());
        instance->array.pop_back();
        return v;
    }

    Value r1 = self.property(ctx, ctx->engine->id_length);
    quint32 r2 = !r1.isUndefined() ? r1.toUInt32(ctx) : 0;
    if (r2) {
        String *r6 = Value::fromDouble(r2 - 1).toString(ctx);
        Value r7 = self.property(ctx, r6);
        self.objectValue()->__delete__(ctx, r6);
        self.objectValue()->__put__(ctx, ctx->engine->id_length, Value::fromDouble(2 - 1));
        return r7;
    }

    self.objectValue()->__put__(ctx, ctx->engine->id_length, Value::fromDouble(0));
    return Value::undefinedValue();
}

Value ArrayPrototype::method_push(ExecutionContext *ctx)
{
    Value self = ctx->thisObject;
    if (ArrayObject *instance = self.asArrayObject()) {
        for (unsigned int i = 0; i < ctx->argumentCount; ++i) {
            Value val = ctx->argument(i);
            instance->array.push_back(val);
        }
        return Value::fromDouble(instance->array.length());
    }

    Value r1 = self.property(ctx, ctx->engine->id_length);
    quint32 n = !r1.isUndefined() ? r1.toUInt32(ctx) : 0;
    for (unsigned int index = 0; index < ctx->argumentCount; ++index, ++n) {
        Value r3 = ctx->argument(index);
        String *name = Value::fromDouble(n).toString(ctx);
        self.objectValue()->__put__(ctx, name, r3);
    }
    Value r = Value::fromDouble(n);
    self.objectValue()->__put__(ctx, ctx->engine->id_length, r);
    return r;
}

Value ArrayPrototype::method_reverse(ExecutionContext *ctx)
{
    ArrayObject *instance = ctx->thisObject.asArrayObject();
    if (!instance)
        ctx->throwUnimplemented(QStringLiteral("Array.prototype.reverse"));

    int lo = 0, hi = instance->array.length() - 1;

    // ###
    for (; lo < hi; ++lo, --hi) {
        Value tmp = instance->__get__(ctx, lo);
        instance->array.set(lo, instance->__get__(ctx, hi));
        instance->array.set(hi, tmp);
    }
    return Value::undefinedValue();
}

Value ArrayPrototype::method_shift(ExecutionContext *ctx)
{
    ArrayObject *instance = ctx->thisObject.asArrayObject();
    if (!instance)
        ctx->throwUnimplemented(QStringLiteral("Array.prototype.shift"));

    Value v = instance->getValueChecked(ctx, instance->array.front());
    instance->array.pop_front();
    return v;
}

Value ArrayPrototype::method_slice(ExecutionContext *ctx)
{
    Object *o = ctx->thisObject.toObject(ctx).objectValue();

    Array result;
    uint len = o->__get__(ctx, ctx->engine->id_length).toUInt32(ctx);
    double s = ctx->argument(0).toInteger(ctx);
    uint start;
    if (s < 0)
        start = (uint)qMax(len + s, 0.);
    else if (s > len)
        start = len;
    else
        start = (uint) s;
    uint end = len;
    if (!ctx->argument(1).isUndefined()) {
        double e = ctx->argument(1).toInteger(ctx);
        if (e < 0)
            end = (uint)qMax(len + e, 0.);
        else if (e > len)
            end = len;
        else
            end = (uint) e;
    }

    uint n = 0;
    for (uint i = start; i < end; ++i) {
        bool exists;
        Value v = o->__get__(ctx, i, &exists);
        if (exists)
            result.set(n, v);
        ++n;
    }
    return Value::fromObject(ctx->engine->newArrayObject(ctx, result));
}

Value ArrayPrototype::method_sort(ExecutionContext *ctx)
{
    Object *instance = __qmljs_to_object(ctx->thisObject, ctx).objectValue();

    uint len = getLength(ctx, instance);

    Value comparefn = ctx->argument(0);
    instance->array.sort(ctx, instance, comparefn, len);
    return ctx->thisObject;
}

Value ArrayPrototype::method_splice(ExecutionContext *ctx)
{
    Object *instance = __qmljs_to_object(ctx->thisObject, ctx).objectValue();
    uint len = getLength(ctx, instance);

    ArrayObject *newArray = ctx->engine->newArrayObject(ctx);

    double rs = ctx->argument(0).toInteger(ctx);
    uint start;
    if (rs < 0)
        start = (uint) qMax(0., len + rs);
    else
        start = (uint) qMin(rs, (double)len);

    uint deleteCount = (uint)qMin(qMax(ctx->argument(1).toInteger(ctx), 0.), (double)(len - start));

    newArray->array.values.resize(deleteCount);
    PropertyDescriptor *pd = newArray->array.values.data();
    for (uint i = 0; i < deleteCount; ++i) {
        pd->type = PropertyDescriptor::Data;
        pd->writable = PropertyDescriptor::Enabled;
        pd->enumberable = PropertyDescriptor::Enabled;
        pd->configurable = PropertyDescriptor::Enabled;
        pd->value = instance->__get__(ctx, start + i);
        ++pd;
    }
    newArray->array.setLengthUnchecked(deleteCount);

    uint itemCount = ctx->argumentCount < 2 ? 0 : ctx->argumentCount - 2;

    if (itemCount < deleteCount) {
        for (uint k = start; k < len - deleteCount; ++k) {
            bool exists;
            Value v = instance->__get__(ctx, k + deleteCount, &exists);
            if (exists)
                instance->array.set(k + itemCount, v);
            else
                instance->__delete__(ctx, k + itemCount);
        }
        for (uint k = len; k > len - deleteCount + itemCount; --k)
            instance->__delete__(ctx, k - 1);
    } else if (itemCount > deleteCount) {
        uint k = len - deleteCount;
        while (k > start) {
            bool exists;
            Value v = instance->__get__(ctx, k + deleteCount - 1, &exists);
            if (exists)
                instance->array.set(k + itemCount - 1, v);
            else
                instance->__delete__(ctx, k + itemCount - 1);
            --k;
        }
    }

    for (uint i = 0; i < itemCount; ++i)
        instance->array.set(start + i, ctx->argument(i + 2));

    ctx->strictMode = true;
    instance->__put__(ctx, ctx->engine->id_length, Value::fromDouble(len - deleteCount + itemCount));

    return Value::fromObject(newArray);
}

Value ArrayPrototype::method_unshift(ExecutionContext *ctx)
{
    ArrayObject *instance = ctx->thisObject.asArrayObject();
    if (!instance)
        ctx->throwUnimplemented(QStringLiteral("Array.prototype.shift"));

    for (int i = ctx->argumentCount - 1; i >= 0; --i) {
        Value v = ctx->argument(i);
        instance->array.push_front(v);
    }

    uint l = instance->array.length();
    if (l < INT_MAX)
        return Value::fromInt32(l);
    return Value::fromDouble((double)l);
}

Value ArrayPrototype::method_indexOf(ExecutionContext *ctx)
{
    Object *instance = __qmljs_to_object(ctx->thisObject, ctx).objectValue();
    uint len = getLength(ctx, instance);
    if (!len)
        return Value::fromInt32(-1);

    Value searchValue;
    uint fromIndex = 0;

    if (ctx->argumentCount >= 1)
        searchValue = ctx->argument(0);
    else
        searchValue = Value::undefinedValue();

    if (ctx->argumentCount >= 2) {
        double f = ctx->argument(1).toInteger(ctx);
        if (f >= len)
            return Value::fromInt32(-1);
        if (f < 0)
            f = qMax(len + f, 0.);
        fromIndex = (uint) f;
    }

    if (instance->isString) {
        for (uint k = fromIndex; k < len; ++k) {
            bool exists;
            Value v = instance->__get__(ctx, k, &exists);
            if (exists && __qmljs_strict_equal(v, searchValue))
                return Value::fromDouble(k);
        }
        return Value::fromInt32(-1);
    }

    return instance->array.indexOf(searchValue, fromIndex, len, ctx, instance);
}

Value ArrayPrototype::method_lastIndexOf(ExecutionContext *ctx)
{
    Object *instance = __qmljs_to_object(ctx->thisObject, ctx).objectValue();
    uint len = getLength(ctx, instance);
    if (!len)
        return Value::fromInt32(-1);

    Value searchValue;
    uint fromIndex = len - 1;

    if (ctx->argumentCount >= 1)
        searchValue = ctx->argument(0);
    else
        searchValue = Value::undefinedValue();

    if (ctx->argumentCount >= 2) {
        double f = ctx->argument(1).toInteger(ctx);
        if (f >= len)
            return Value::fromInt32(-1);
        if (f < 0)
            f = qMax(len + f, 0.);
        fromIndex = (uint) f;
    }

    for (uint k = fromIndex; k > 0; --k) {
        bool exists;
        Value v = instance->__get__(ctx, k, &exists);
        if (exists && __qmljs_strict_equal(v, searchValue))
            return Value::fromDouble(k);
    }
    return Value::fromInt32(-1);
}

Value ArrayPrototype::method_every(ExecutionContext *ctx)
{
    Object *instance = __qmljs_to_object(ctx->thisObject, ctx).objectValue();

    uint len = getLength(ctx, instance);

    FunctionObject *callback = ctx->argument(0).asFunctionObject();
    if (!callback)
        __qmljs_throw_type_error(ctx);

    Value thisArg = ctx->argument(1);

    bool ok = true;
    for (uint k = 0; ok && k < len; ++k) {
        bool exists;
        Value v = instance->__get__(ctx, k, &exists);
        if (!exists)
            continue;

        Value args[3];
        args[0] = v;
        args[1] = Value::fromDouble(k);
        args[2] = ctx->thisObject;
        Value r = callback->call(ctx, thisArg, args, 3);
        ok = __qmljs_to_boolean(r, ctx);
    }
    return Value::fromBoolean(ok);
}

Value ArrayPrototype::method_some(ExecutionContext *ctx)
{
    Object *instance = __qmljs_to_object(ctx->thisObject, ctx).objectValue();

    uint len = getLength(ctx, instance);

    FunctionObject *callback = ctx->argument(0).asFunctionObject();
    if (!callback)
        __qmljs_throw_type_error(ctx);

    Value thisArg = ctx->argument(1);

    for (uint k = 0; k < len; ++k) {
        bool exists;
        Value v = instance->__get__(ctx, k, &exists);
        if (!exists)
            continue;

        Value args[3];
        args[0] = v;
        args[1] = Value::fromDouble(k);
        args[2] = ctx->thisObject;
        Value r = callback->call(ctx, thisArg, args, 3);
        if (__qmljs_to_boolean(r, ctx))
            return Value::fromBoolean(true);
    }
    return Value::fromBoolean(false);
}

Value ArrayPrototype::method_forEach(ExecutionContext *ctx)
{
    Object *instance = __qmljs_to_object(ctx->thisObject, ctx).objectValue();

    uint len = getLength(ctx, instance);

    FunctionObject *callback = ctx->argument(0).asFunctionObject();
    if (!callback)
        __qmljs_throw_type_error(ctx);

    Value thisArg = ctx->argument(1);

    for (uint k = 0; k < len; ++k) {
        bool exists;
        Value v = instance->__get__(ctx, k, &exists);
        if (!exists)
            continue;

        Value args[3];
        args[0] = v;
        args[1] = Value::fromDouble(k);
        args[2] = ctx->thisObject;
        callback->call(ctx, thisArg, args, 3);
    }
    return Value::undefinedValue();
}

Value ArrayPrototype::method_map(ExecutionContext *ctx)
{
    Object *instance = __qmljs_to_object(ctx->thisObject, ctx).objectValue();

    uint len = getLength(ctx, instance);

    FunctionObject *callback = ctx->argument(0).asFunctionObject();
    if (!callback)
        __qmljs_throw_type_error(ctx);

    Value thisArg = ctx->argument(1);

    ArrayObject *a = ctx->engine->newArrayObject(ctx);
    a->array.setLength(len);

    for (uint k = 0; k < len; ++k) {
        bool exists;
        Value v = instance->__get__(ctx, k, &exists);
        if (!exists)
            continue;

        Value args[3];
        args[0] = v;
        args[1] = Value::fromDouble(k);
        args[2] = ctx->thisObject;
        Value mapped = callback->call(ctx, thisArg, args, 3);
        a->array.set(k, mapped);
    }
    return Value::fromObject(a);
}

Value ArrayPrototype::method_filter(ExecutionContext *ctx)
{
    Object *instance = __qmljs_to_object(ctx->thisObject, ctx).objectValue();

    uint len = getLength(ctx, instance);

    FunctionObject *callback = ctx->argument(0).asFunctionObject();
    if (!callback)
        __qmljs_throw_type_error(ctx);

    Value thisArg = ctx->argument(1);

    ArrayObject *a = ctx->engine->newArrayObject(ctx);

    uint to = 0;
    for (uint k = 0; k < len; ++k) {
        bool exists;
        Value v = instance->__get__(ctx, k, &exists);
        if (!exists)
            continue;

        Value args[3];
        args[0] = v;
        args[1] = Value::fromDouble(k);
        args[2] = ctx->thisObject;
        Value selected = callback->call(ctx, thisArg, args, 3);
        if (__qmljs_to_boolean(selected, ctx)) {
            a->array.set(to, v);
            ++to;
        }
    }
    return Value::fromObject(a);
}

Value ArrayPrototype::method_reduce(ExecutionContext *ctx)
{
    Object *instance = __qmljs_to_object(ctx->thisObject, ctx).objectValue();

    uint len = getLength(ctx, instance);

    FunctionObject *callback = ctx->argument(0).asFunctionObject();
    if (!callback)
        __qmljs_throw_type_error(ctx);

    uint k = 0;
    Value acc;
    if (ctx->argumentCount > 1) {
        acc = ctx->argument(1);
    } else {
        bool kPresent = false;
        while (k < len && !kPresent) {
            Value v = instance->__get__(ctx, k, &kPresent);
            if (kPresent)
                acc = v;
            ++k;
        }
        if (!kPresent)
            __qmljs_throw_type_error(ctx);
    }

    while (k < len) {
        bool kPresent;
        Value v = instance->__get__(ctx, k, &kPresent);
        if (kPresent) {
            Value args[4];
            args[0] = acc;
            args[1] = v;
            args[2] = Value::fromDouble(k);
            args[3] = ctx->thisObject;
            acc = callback->call(ctx, Value::undefinedValue(), args, 4);
        }
        ++k;
    }
    return acc;
}

Value ArrayPrototype::method_reduceRight(ExecutionContext *ctx)
{
    Object *instance = __qmljs_to_object(ctx->thisObject, ctx).objectValue();

    uint len = getLength(ctx, instance);

    FunctionObject *callback = ctx->argument(0).asFunctionObject();
    if (!callback)
        __qmljs_throw_type_error(ctx);

    if (len == 0) {
        if (ctx->argumentCount == 1)
            __qmljs_throw_type_error(ctx);
        return ctx->argument(1);
    }

    uint k = len;
    Value acc;
    if (ctx->argumentCount > 1) {
        acc = ctx->argument(1);
    } else {
        bool kPresent = false;
        while (k > 0 && !kPresent) {
            Value v = instance->__get__(ctx, k - 1, &kPresent);
            if (kPresent)
                acc = v;
            --k;
        }
        if (!kPresent)
            __qmljs_throw_type_error(ctx);
    }

    while (k > 0) {
        bool kPresent;
        Value v = instance->__get__(ctx, k - 1, &kPresent);
        if (kPresent) {
            Value args[4];
            args[0] = acc;
            args[1] = v;
            args[2] = Value::fromDouble(k - 1);
            args[3] = ctx->thisObject;
            acc = callback->call(ctx, Value::undefinedValue(), args, 4);
        }
        --k;
    }
    return acc;
}

//
// Function object
//
FunctionCtor::FunctionCtor(ExecutionContext *scope)
    : FunctionObject(scope)
{
}

// 15.3.2
Value FunctionCtor::construct(ExecutionContext *ctx)
{
    MemoryManager::GCBlocker gcBlocker(ctx->engine->memoryManager);

    QString args;
    QString body;
    if (ctx->argumentCount > 0) {
        for (uint i = 0; i < ctx->argumentCount - 1; ++i) {
            if (i)
                args += QLatin1String(", ");
            args += ctx->argument(i).toString(ctx)->toQString();
        }
        body = ctx->argument(ctx->argumentCount - 1).toString(ctx)->toQString();
    }

    QString function = QLatin1String("function(") + args + QLatin1String("){") + body + QLatin1String("}");

    QQmlJS::Engine ee, *engine = &ee;
    Lexer lexer(engine);
    lexer.setCode(function, 1, false);
    Parser parser(engine);

    const bool parsed = parser.parseExpression();

    if (!parsed)
        ctx->throwSyntaxError(0);

    using namespace AST;
    FunctionExpression *fe = AST::cast<FunctionExpression *>(parser.rootNode());
    if (!fe)
        ctx->throwSyntaxError(0);

    IR::Module module;

    Codegen cg(ctx);
    IR::Function *irf = cg(QString(), fe, &module);

    QScopedPointer<EvalInstructionSelection> isel(ctx->engine->iselFactory->create(ctx->engine, &module));
    VM::Function *vmf = isel->vmFunction(irf);

    return Value::fromObject(ctx->engine->newScriptFunction(ctx->engine->rootContext, vmf));
}

// 15.3.1: This is equivalent to new Function(...)
Value FunctionCtor::call(ExecutionContext *ctx)
{
    return construct(ctx);
}

void FunctionPrototype::init(ExecutionContext *ctx, const Value &ctor)
{
    ctor.objectValue()->defineReadonlyProperty(ctx->engine->id_prototype, Value::fromObject(this));
    ctor.objectValue()->defineReadonlyProperty(ctx->engine->id_length, Value::fromInt32(1));
    defineDefaultProperty(ctx, QStringLiteral("constructor"), ctor);
    defineDefaultProperty(ctx, QStringLiteral("toString"), method_toString, 0);
    defineDefaultProperty(ctx, QStringLiteral("apply"), method_apply, 2);
    defineDefaultProperty(ctx, QStringLiteral("call"), method_call, 1);
    defineDefaultProperty(ctx, QStringLiteral("bind"), method_bind, 1);
}

Value FunctionPrototype::method_toString(ExecutionContext *ctx)
{
    FunctionObject *fun = ctx->thisObject.asFunctionObject();
    if (!fun)
        ctx->throwTypeError();

    return Value::fromString(ctx, QStringLiteral("function() { [code] }"));
}

Value FunctionPrototype::method_apply(ExecutionContext *ctx)
{
    Value thisArg = ctx->argument(0);

    Value arg = ctx->argument(1);
    QVector<Value> args;

    if (Object *arr = arg.asObject()) {
        quint32 len = arr->__get__(ctx, ctx->engine->id_length).toUInt32(ctx);
        for (quint32 i = 0; i < len; ++i) {
            Value a = arr->__get__(ctx, i);
            args.append(a);
        }
    } else if (!(arg.isUndefined() || arg.isNull())) {
        ctx->throwTypeError();
        return Value::undefinedValue();
    }

    FunctionObject *o = ctx->thisObject.asFunctionObject();
    if (!o)
        ctx->throwTypeError();

    return o->callDirect(ctx, thisArg, args.data(), args.size());
}

Value FunctionPrototype::method_call(ExecutionContext *ctx)
{
    Value thisArg = ctx->argument(0);

    QVector<Value> args(ctx->argumentCount ? ctx->argumentCount - 1 : 0);
    if (ctx->argumentCount)
        qCopy(ctx->arguments + 1,
              ctx->arguments + ctx->argumentCount, args.begin());

    FunctionObject *o = ctx->thisObject.asFunctionObject();
    if (!o)
        ctx->throwTypeError();

    return o->callDirect(ctx, thisArg, args.data(), args.size());
}

Value FunctionPrototype::method_bind(ExecutionContext *ctx)
{
    FunctionObject *target = ctx->thisObject.asFunctionObject();
    if (!target)
        ctx->throwTypeError();

    Value boundThis = ctx->argument(0);
    QVector<Value> boundArgs;
    for (uint i = 1; i < ctx->argumentCount; ++i)
        boundArgs += ctx->argument(i);


    BoundFunction *f = ctx->engine->newBoundFunction(ctx, target, boundThis, boundArgs);
    return Value::fromObject(f);
}

//
// RegExp object
//
RegExpCtor::RegExpCtor(ExecutionContext *scope)
    : FunctionObject(scope)
{
}

Value RegExpCtor::construct(ExecutionContext *ctx)
{
    Value r = ctx->argumentCount > 0 ? ctx->argument(0) : Value::undefinedValue();
    Value f = ctx->argumentCount > 1 ? ctx->argument(1) : Value::undefinedValue();
    if (RegExpObject *re = r.asRegExpObject()) {
        if (!f.isUndefined())
            ctx->throwTypeError();

        RegExpObject *o = ctx->engine->newRegExpObject(re->value, re->global);
        return Value::fromObject(o);
    }

    if (r.isUndefined())
        r = Value::fromString(ctx, QString());
    else if (!r.isString())
        r = __qmljs_to_string(r, ctx);

    bool global = false;
    bool ignoreCase = false;
    bool multiLine = false;
    if (!f.isUndefined()) {
        f = __qmljs_to_string(f, ctx);
        QString str = f.stringValue()->toQString();
        for (int i = 0; i < str.length(); ++i) {
            if (str.at(i) == QChar('g') && !global) {
                global = true;
            } else if (str.at(i) == QChar('i') && !ignoreCase) {
                ignoreCase = true;
            } else if (str.at(i) == QChar('m') && !multiLine) {
                multiLine = true;
            } else {
                ctx->throwSyntaxError(0);
            }
        }
    }

    RefPtr<RegExp> re = RegExp::create(ctx->engine, r.stringValue()->toQString(), ignoreCase, multiLine);
    if (!re->isValid())
        ctx->throwSyntaxError(0);

    RegExpObject *o = ctx->engine->newRegExpObject(re, global);
    return Value::fromObject(o);
}

Value RegExpCtor::call(ExecutionContext *ctx)
{
    if (ctx->argumentCount > 0 && ctx->argument(0).asRegExpObject()) {
        if (ctx->argumentCount == 1 || ctx->argument(1).isUndefined())
            return ctx->argument(0);
    }

    return construct(ctx);
}

void RegExpPrototype::init(ExecutionContext *ctx, const Value &ctor)
{
    ctor.objectValue()->defineReadonlyProperty(ctx->engine->id_prototype, Value::fromObject(this));
    ctor.objectValue()->defineReadonlyProperty(ctx->engine->id_length, Value::fromInt32(2));
    defineDefaultProperty(ctx, QStringLiteral("constructor"), ctor);
    defineDefaultProperty(ctx, QStringLiteral("exec"), method_exec, 1);
    defineDefaultProperty(ctx, QStringLiteral("test"), method_test, 1);
    defineDefaultProperty(ctx, QStringLiteral("toString"), method_toString, 0);
}

Value RegExpPrototype::method_exec(ExecutionContext *ctx)
{
    RegExpObject *r = ctx->thisObject.asRegExpObject();
    if (!r)
        ctx->throwTypeError();

    Value arg = ctx->argument(0);
    arg = __qmljs_to_string(arg, ctx);
    QString s = arg.stringValue()->toQString();

    int offset = r->global ? r->lastIndexProperty->value.toInt32(ctx) : 0;
    if (offset < 0 || offset > s.length()) {
        r->lastIndexProperty->value = Value::fromInt32(0);
        return Value::nullValue();
    }

    uint* matchOffsets = (uint*)alloca(r->value->captureCount() * 2 * sizeof(uint));
    int result = r->value->match(s, offset, matchOffsets);
    if (result == -1) {
        r->lastIndexProperty->value = Value::fromInt32(0);
        return Value::nullValue();
    }

    // fill in result data
    ArrayObject *array = ctx->engine->newArrayObject(ctx)->asArrayObject();
    for (int i = 0; i < r->value->captureCount(); ++i) {
        int start = matchOffsets[i * 2];
        int end = matchOffsets[i * 2 + 1];
        Value entry = Value::undefinedValue();
        if (start != -1 && end != -1)
            entry = Value::fromString(ctx, s.mid(start, end - start));
        array->array.push_back(entry);
    }

    array->__put__(ctx, QLatin1String("index"), Value::fromInt32(result));
    array->__put__(ctx, QLatin1String("input"), arg);

    if (r->global)
        r->lastIndexProperty->value = Value::fromInt32(matchOffsets[1]);

    return Value::fromObject(array);
}

Value RegExpPrototype::method_test(ExecutionContext *ctx)
{
    Value r = method_exec(ctx);
    return Value::fromBoolean(!r.isNull());
}

Value RegExpPrototype::method_toString(ExecutionContext *ctx)
{
    RegExpObject *r = ctx->thisObject.asRegExpObject();
    if (!r)
        ctx->throwTypeError();

    QString result = QChar('/') + r->value->pattern();
    result += QChar('/');
    // ### 'g' option missing
    if (r->value->ignoreCase())
        result += QChar('i');
    if (r->value->multiLine())
        result += QChar('m');
    return Value::fromString(ctx, result);
}

//
// ErrorCtr
//
ErrorCtor::ErrorCtor(ExecutionContext *scope)
    : FunctionObject(scope)
{
}

Value ErrorCtor::construct(ExecutionContext *ctx)
{
    return Value::fromObject(ctx->engine->newErrorObject(ctx->argument(0)));
}

Value ErrorCtor::call(ExecutionContext *ctx)
{
    return construct(ctx);
}

Value EvalErrorCtor::construct(ExecutionContext *ctx)
{
    return Value::fromObject(new (ctx->engine->memoryManager) EvalErrorObject(ctx));
}

Value RangeErrorCtor::construct(ExecutionContext *ctx)
{
    return Value::fromObject(new (ctx->engine->memoryManager) RangeErrorObject(ctx));
}

Value ReferenceErrorCtor::construct(ExecutionContext *ctx)
{
    return Value::fromObject(new (ctx->engine->memoryManager) ReferenceErrorObject(ctx));
}

Value SyntaxErrorCtor::construct(ExecutionContext *ctx)
{
    return Value::fromObject(new (ctx->engine->memoryManager) SyntaxErrorObject(ctx, 0));
}

Value TypeErrorCtor::construct(ExecutionContext *ctx)
{
    return Value::fromObject(new (ctx->engine->memoryManager) TypeErrorObject(ctx));
}

Value URIErrorCtor::construct(ExecutionContext *ctx)
{
    return Value::fromObject(new (ctx->engine->memoryManager) URIErrorObject(ctx));
}

void ErrorPrototype::init(ExecutionContext *ctx, const Value &ctor, Object *obj)
{
    ctor.objectValue()->defineReadonlyProperty(ctx->engine->id_prototype, Value::fromObject(obj));
    ctor.objectValue()->defineReadonlyProperty(ctx->engine->id_length, Value::fromInt32(1));
    obj->defineDefaultProperty(ctx, QStringLiteral("constructor"), ctor);
    obj->defineDefaultProperty(ctx, QStringLiteral("toString"), method_toString, 0);
    obj->defineDefaultProperty(ctx, QStringLiteral("message"), Value::fromString(ctx, QString()));
}

Value ErrorPrototype::method_toString(ExecutionContext *ctx)
{
    Object *o = ctx->thisObject.asObject();
    if (!o)
        __qmljs_throw_type_error(ctx);

    Value name = o->__get__(ctx, ctx->engine->newString(QString::fromLatin1("name")));
    QString qname;
    if (name.isUndefined())
        qname = QString::fromLatin1("Error");
    else
        qname = __qmljs_to_string(name, ctx).stringValue()->toQString();

    Value message = o->__get__(ctx, ctx->engine->newString(QString::fromLatin1("message")));
    QString qmessage;
    if (!message.isUndefined())
        qmessage = __qmljs_to_string(message, ctx).stringValue()->toQString();

    QString str;
    if (qname.isEmpty()) {
        str = qmessage;
    } else if (qmessage.isEmpty()) {
        str = qname;
    } else {
        str = qname + QLatin1String(": ") + qmessage;
    }

    return Value::fromString(ctx, str);
}


//
// Math object
//
MathObject::MathObject(ExecutionContext *ctx)
{
    defineReadonlyProperty(ctx->engine, QStringLiteral("E"), Value::fromDouble(::exp(1.0)));
    defineReadonlyProperty(ctx->engine, QStringLiteral("LN2"), Value::fromDouble(::log(2.0)));
    defineReadonlyProperty(ctx->engine, QStringLiteral("LN10"), Value::fromDouble(::log(10.0)));
    defineReadonlyProperty(ctx->engine, QStringLiteral("LOG2E"), Value::fromDouble(1.0/::log(2.0)));
    defineReadonlyProperty(ctx->engine, QStringLiteral("LOG10E"), Value::fromDouble(1.0/::log(10.0)));
    defineReadonlyProperty(ctx->engine, QStringLiteral("PI"), Value::fromDouble(qt_PI));
    defineReadonlyProperty(ctx->engine, QStringLiteral("SQRT1_2"), Value::fromDouble(::sqrt(0.5)));
    defineReadonlyProperty(ctx->engine, QStringLiteral("SQRT2"), Value::fromDouble(::sqrt(2.0)));

    defineDefaultProperty(ctx, QStringLiteral("abs"), method_abs, 1);
    defineDefaultProperty(ctx, QStringLiteral("acos"), method_acos, 1);
    defineDefaultProperty(ctx, QStringLiteral("asin"), method_asin, 0);
    defineDefaultProperty(ctx, QStringLiteral("atan"), method_atan, 1);
    defineDefaultProperty(ctx, QStringLiteral("atan2"), method_atan2, 2);
    defineDefaultProperty(ctx, QStringLiteral("ceil"), method_ceil, 1);
    defineDefaultProperty(ctx, QStringLiteral("cos"), method_cos, 1);
    defineDefaultProperty(ctx, QStringLiteral("exp"), method_exp, 1);
    defineDefaultProperty(ctx, QStringLiteral("floor"), method_floor, 1);
    defineDefaultProperty(ctx, QStringLiteral("log"), method_log, 1);
    defineDefaultProperty(ctx, QStringLiteral("max"), method_max, 2);
    defineDefaultProperty(ctx, QStringLiteral("min"), method_min, 2);
    defineDefaultProperty(ctx, QStringLiteral("pow"), method_pow, 2);
    defineDefaultProperty(ctx, QStringLiteral("random"), method_random, 0);
    defineDefaultProperty(ctx, QStringLiteral("round"), method_round, 1);
    defineDefaultProperty(ctx, QStringLiteral("sin"), method_sin, 1);
    defineDefaultProperty(ctx, QStringLiteral("sqrt"), method_sqrt, 1);
    defineDefaultProperty(ctx, QStringLiteral("tan"), method_tan, 1);
}

/* copies the sign from y to x and returns the result */
static double copySign(double x, double y)
{
    uchar *xch = (uchar *)&x;
    uchar *ych = (uchar *)&y;
    if (QSysInfo::ByteOrder == QSysInfo::BigEndian)
        xch[0] = (xch[0] & 0x7f) | (ych[0] & 0x80);
    else
        xch[7] = (xch[7] & 0x7f) | (ych[7] & 0x80);
    return x;
}

Value MathObject::method_abs(ExecutionContext *ctx)
{
    double v = ctx->argument(0).toNumber(ctx);
    if (v == 0) // 0 | -0
        return Value::fromDouble(0);

    return Value::fromDouble(v < 0 ? -v : v);
}

Value MathObject::method_acos(ExecutionContext *ctx)
{
    double v = ctx->argument(0).toNumber(ctx);
    if (v > 1)
        return Value::fromDouble(qSNaN());

    return Value::fromDouble(::acos(v));
}

Value MathObject::method_asin(ExecutionContext *ctx)
{
    double v = ctx->argument(0).toNumber(ctx);
    if (v > 1)
        return Value::fromDouble(qSNaN());
    else
        return Value::fromDouble(::asin(v));
}

Value MathObject::method_atan(ExecutionContext *ctx)
{
    double v = ctx->argument(0).toNumber(ctx);
    if (v == 0.0)
        return Value::fromDouble(v);
    else
        return Value::fromDouble(::atan(v));
}

Value MathObject::method_atan2(ExecutionContext *ctx)
{
    double v1 = ctx->argument(0).toNumber(ctx);
    double v2 = ctx->argument(1).toNumber(ctx);
    if ((v1 < 0) && qIsFinite(v1) && qIsInf(v2) && (copySign(1.0, v2) == 1.0)) {
        return Value::fromDouble(copySign(0, -1.0));
    }
    if ((v1 == 0.0) && (v2 == 0.0)) {
        if ((copySign(1.0, v1) == 1.0) && (copySign(1.0, v2) == -1.0)) {
            return Value::fromDouble(qt_PI);
        } else if ((copySign(1.0, v1) == -1.0) && (copySign(1.0, v2) == -1.0)) {
            return Value::fromDouble(-qt_PI);
        }
    }
    return Value::fromDouble(::atan2(v1, v2));
}

Value MathObject::method_ceil(ExecutionContext *ctx)
{
    double v = ctx->argument(0).toNumber(ctx);
    if (v < 0.0 && v > -1.0)
        return Value::fromDouble(copySign(0, -1.0));
    else
        return Value::fromDouble(::ceil(v));
}

Value MathObject::method_cos(ExecutionContext *ctx)
{
    double v = ctx->argument(0).toNumber(ctx);
    return Value::fromDouble(::cos(v));
}

Value MathObject::method_exp(ExecutionContext *ctx)
{
    double v = ctx->argument(0).toNumber(ctx);
    if (qIsInf(v)) {
        if (copySign(1.0, v) == -1.0)
            return Value::fromDouble(0);
        else
            return Value::fromDouble(qInf());
    } else {
        return Value::fromDouble(::exp(v));
    }
}

Value MathObject::method_floor(ExecutionContext *ctx)
{
    double v = ctx->argument(0).toNumber(ctx);
    return Value::fromDouble(::floor(v));
}

Value MathObject::method_log(ExecutionContext *ctx)
{
    double v = ctx->argument(0).toNumber(ctx);
    if (v < 0)
        return Value::fromDouble(qSNaN());
    else
        return Value::fromDouble(::log(v));
}

Value MathObject::method_max(ExecutionContext *ctx)
{
    double mx = -qInf();
    for (unsigned i = 0; i < ctx->argumentCount; ++i) {
        double x = ctx->argument(i).toNumber(ctx);
        if (x > mx || std::isnan(x))
            mx = x;
    }
    return Value::fromDouble(mx);
}

Value MathObject::method_min(ExecutionContext *ctx)
{
    double mx = qInf();
    for (unsigned i = 0; i < ctx->argumentCount; ++i) {
        double x = ctx->argument(i).toNumber(ctx);
        if ((x == 0 && mx == x && copySign(1.0, x) == -1.0)
                || (x < mx) || std::isnan(x)) {
            mx = x;
        }
    }
    return Value::fromDouble(mx);
}

Value MathObject::method_pow(ExecutionContext *ctx)
{
    double x = ctx->argument(0).toNumber(ctx);
    double y = ctx->argument(1).toNumber(ctx);

    if (std::isnan(y))
        return Value::fromDouble(qSNaN());

    if (y == 0) {
        return Value::fromDouble(1);
    } else if (((x == 1) || (x == -1)) && std::isinf(y)) {
        return Value::fromDouble(qSNaN());
    } else if (((x == 0) && copySign(1.0, x) == 1.0) && (y < 0)) {
        return Value::fromDouble(qInf());
    } else if ((x == 0) && copySign(1.0, x) == -1.0) {
        if (y < 0) {
            if (::fmod(-y, 2.0) == 1.0)
                return Value::fromDouble(-qInf());
            else
                return Value::fromDouble(qInf());
        } else if (y > 0) {
            if (::fmod(y, 2.0) == 1.0)
                return Value::fromDouble(copySign(0, -1.0));
            else
                return Value::fromDouble(0);
        }
    }

#ifdef Q_OS_AIX
    else if (qIsInf(x) && copySign(1.0, x) == -1.0) {
        if (y > 0) {
            if (::fmod(y, 2.0) == 1.0)
                return Value::number(ctx, -qInf());
            else
                return Value::number(ctx, qInf());
        } else if (y < 0) {
            if (::fmod(-y, 2.0) == 1.0)
                return Value::number(ctx, copySign(0, -1.0));
            else
                return Value::number(ctx, 0);
        }
    }
#endif
    else {
        return Value::fromDouble(::pow(x, y));
    }
    // ###
    return Value::fromDouble(qSNaN());
}

Value MathObject::method_random(ExecutionContext */*ctx*/)
{
    return Value::fromDouble(qrand() / (double) RAND_MAX);
}

Value MathObject::method_round(ExecutionContext *ctx)
{
    double v = ctx->argument(0).toNumber(ctx);
    v = copySign(::floor(v + 0.5), v);
    return Value::fromDouble(v);
}

Value MathObject::method_sin(ExecutionContext *ctx)
{
    double v = ctx->argument(0).toNumber(ctx);
    return Value::fromDouble(::sin(v));
}

Value MathObject::method_sqrt(ExecutionContext *ctx)
{
    double v = ctx->argument(0).toNumber(ctx);
    return Value::fromDouble(::sqrt(v));
}

Value MathObject::method_tan(ExecutionContext *ctx)
{
    double v = ctx->argument(0).toNumber(ctx);
    if (v == 0.0)
        return Value::fromDouble(v);
    else
        return Value::fromDouble(::tan(v));
}

