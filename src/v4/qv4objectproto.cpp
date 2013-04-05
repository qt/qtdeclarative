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


#include "qv4objectproto.h"
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
#include <qv4jsir_p.h>
#include <qv4codegen_p.h>
#include <qv4isel_masm_p.h>

#ifndef Q_OS_WIN
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


DEFINE_MANAGED_VTABLE(ObjectCtor);

ObjectCtor::ObjectCtor(ExecutionContext *scope)
    : FunctionObject(scope)
{
    vtbl = &static_vtbl;
}

Value ObjectCtor::construct(Managed *that, ExecutionContext *ctx, Value *args, int argc)
{
    ObjectCtor *ctor = static_cast<ObjectCtor *>(that);
    if (!argc || args[0].isUndefined() || args[0].isNull()) {
        Object *obj = ctx->engine->newObject();
        Value proto = ctor->get(ctx, ctx->engine->id_prototype);
        if (proto.isObject())
            obj->prototype = proto.objectValue();
        return Value::fromObject(obj);
    }
    return __qmljs_to_object(ctx, args[0]);
}

Value ObjectCtor::call(Managed *, ExecutionContext *ctx, const Value &/*thisObject*/, Value *args, int argc)
{
    if (!argc || args[0].isUndefined() || args[0].isNull())
        return Value::fromObject(ctx->engine->newObject());
    return __qmljs_to_object(ctx, args[0]);
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

Value ObjectPrototype::method_getPrototypeOf(CallContext *ctx)
{
    Value o = ctx->argument(0);
    if (! o.isObject())
        ctx->throwTypeError();

    Object *p = o.objectValue()->prototype;
    return p ? Value::fromObject(p) : Value::nullValue();
}

Value ObjectPrototype::method_getOwnPropertyDescriptor(CallContext *ctx)
{
    Value O = ctx->argument(0);
    if (!O.isObject())
        ctx->throwTypeError();

    String *name = ctx->argument(1).toString(ctx);
    PropertyDescriptor *desc = O.objectValue()->__getOwnProperty__(ctx, name);
    return fromPropertyDescriptor(ctx, desc);
}

Value ObjectPrototype::method_getOwnPropertyNames(ExecutionContext *parentCtx, Value, Value *argv, int argc)
{
    Object *O = argc ? argv[0].asObject() : 0;
    if (!O)
        parentCtx->throwTypeError();

    ArrayObject *array = parentCtx->engine->newArrayObject(parentCtx)->asArrayObject();
    ObjectIterator it(parentCtx, O, ObjectIterator::NoFlags);
    while (1) {
        Value v = it.nextPropertyNameAsString();
        if (v.isNull())
            break;
        array->push_back(v);
    }
    return Value::fromObject(array);
}

Value ObjectPrototype::method_create(CallContext *ctx)
{
    Value O = ctx->argument(0);
    if (!O.isObject() && !O.isNull())
        ctx->throwTypeError();

    Object *newObject = ctx->engine->newObject();
    newObject->prototype = O.asObject();

    Value objValue = Value::fromObject(newObject);
    if (ctx->argumentCount > 1 && !ctx->argument(1).isUndefined()) {
        ctx->arguments[0] = objValue;
        method_defineProperties(ctx);
    }

    return objValue;
}

Value ObjectPrototype::method_defineProperty(CallContext *ctx)
{
    Value O = ctx->argument(0);
    if (!O.isObject())
        ctx->throwTypeError();

    String *name = ctx->argument(1).toString(ctx);

    Value attributes = ctx->argument(2);
    PropertyDescriptor pd;
    toPropertyDescriptor(ctx, attributes, &pd);

    if (!O.objectValue()->__defineOwnProperty__(ctx, name, &pd))
        ctx->throwTypeError();

    return O;
}

Value ObjectPrototype::method_defineProperties(CallContext *ctx)
{
    Value O = ctx->argument(0);
    if (!O.isObject())
        ctx->throwTypeError();

    Object *o = ctx->argument(1).toObject(ctx);

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
            ctx->throwTypeError();
    }

    return O;
}

Value ObjectPrototype::method_seal(CallContext *ctx)
{
    if (!ctx->argument(0).isObject())
        ctx->throwTypeError();

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

Value ObjectPrototype::method_freeze(CallContext *ctx)
{
    if (!ctx->argument(0).isObject())
        ctx->throwTypeError();

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

Value ObjectPrototype::method_preventExtensions(CallContext *ctx)
{
    if (!ctx->argument(0).isObject())
        ctx->throwTypeError();

    Object *o = ctx->argument(0).objectValue();
    o->extensible = false;
    return ctx->argument(0);
}

Value ObjectPrototype::method_isSealed(CallContext *ctx)
{
    if (!ctx->argument(0).isObject())
        ctx->throwTypeError();

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

Value ObjectPrototype::method_isFrozen(CallContext *ctx)
{
    if (!ctx->argument(0).isObject())
        ctx->throwTypeError();

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

Value ObjectPrototype::method_isExtensible(CallContext *ctx)
{
    if (!ctx->argument(0).isObject())
        ctx->throwTypeError();

    Object *o = ctx->argument(0).objectValue();
    return Value::fromBoolean(o->extensible);
}

Value ObjectPrototype::method_keys(CallContext *ctx)
{
    if (!ctx->argument(0).isObject())
        ctx->throwTypeError();

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
        a->push_back(key);
    }

    return Value::fromObject(a);
}

Value ObjectPrototype::method_toString(CallContext *ctx)
{
    if (ctx->thisObject.isUndefined()) {
        return Value::fromString(ctx, QStringLiteral("[object Undefined]"));
    } else if (ctx->thisObject.isNull()) {
        return Value::fromString(ctx, QStringLiteral("[object Null]"));
    } else {
        Value obj = __qmljs_to_object(ctx, ctx->thisObject);
        QString className = obj.objectValue()->className();
        return Value::fromString(ctx, QString::fromUtf8("[object %1]").arg(className));
    }
}

Value ObjectPrototype::method_toLocaleString(CallContext *ctx)
{
    Object *o = ctx->thisObject.toObject(ctx);
    Value ts = o->get(ctx, ctx->engine->newString(QStringLiteral("toString")));
    FunctionObject *f = ts.asFunctionObject();
    if (!f)
        ctx->throwTypeError();
    return f->call(ctx, Value::fromObject(o), 0, 0);
}

Value ObjectPrototype::method_valueOf(CallContext *ctx)
{
    return Value::fromObject(ctx->thisObject.toObject(ctx));
}

Value ObjectPrototype::method_hasOwnProperty(CallContext *ctx)
{
    String *P = ctx->argument(0).toString(ctx);
    Object *O = ctx->thisObject.toObject(ctx);
    bool r = O->__getOwnProperty__(ctx, P) != 0;
    return Value::fromBoolean(r);
}

Value ObjectPrototype::method_isPrototypeOf(CallContext *ctx)
{
    Value V = ctx->argument(0);
    if (! V.isObject())
        return Value::fromBoolean(false);

    Object *O = ctx->thisObject.toObject(ctx);
    Object *proto = V.objectValue()->prototype;
    while (proto) {
        if (O == proto)
            return Value::fromBoolean(true);
        proto = proto->prototype;
    }
    return Value::fromBoolean(false);
}

Value ObjectPrototype::method_propertyIsEnumerable(CallContext *ctx)
{
    String *p = ctx->argument(0).toString(ctx);

    Object *o = ctx->thisObject.toObject(ctx);
    PropertyDescriptor *pd = o->__getOwnProperty__(ctx, p);
    return Value::fromBoolean(pd && pd->isEnumerable());
}

Value ObjectPrototype::method_defineGetter(CallContext *ctx)
{
    if (ctx->argumentCount < 2)
        ctx->throwTypeError();
    String *prop = ctx->argument(0).toString(ctx);

    FunctionObject *f = ctx->argument(1).asFunctionObject();
    if (!f)
        ctx->throwTypeError();

    Object *o = ctx->thisObject.toObject(ctx);

    PropertyDescriptor pd = PropertyDescriptor::fromAccessor(f, 0);
    pd.configurable = PropertyDescriptor::Enabled;
    pd.enumerable = PropertyDescriptor::Enabled;
    o->__defineOwnProperty__(ctx, prop, &pd);
    return Value::undefinedValue();
}

Value ObjectPrototype::method_defineSetter(CallContext *ctx)
{
    if (ctx->argumentCount < 2)
        ctx->throwTypeError();
    String *prop = ctx->argument(0).toString(ctx);

    FunctionObject *f = ctx->argument(1).asFunctionObject();
    if (!f)
        ctx->throwTypeError();

    Object *o = ctx->thisObject.toObject(ctx);

    PropertyDescriptor pd = PropertyDescriptor::fromAccessor(0, f);
    pd.configurable = PropertyDescriptor::Enabled;
    pd.enumerable = PropertyDescriptor::Enabled;
    o->__defineOwnProperty__(ctx, prop, &pd);
    return Value::undefinedValue();
}

void ObjectPrototype::toPropertyDescriptor(ExecutionContext *ctx, Value v, PropertyDescriptor *desc)
{
    if (!v.isObject())
        ctx->throwTypeError();

    Object *o = v.objectValue();

    desc->type = PropertyDescriptor::Generic;

    desc->enumerable = PropertyDescriptor::Undefined;
    if (o->__hasProperty__(ctx, ctx->engine->id_enumerable))
        desc->enumerable = o->get(ctx, ctx->engine->id_enumerable).toBoolean() ? PropertyDescriptor::Enabled : PropertyDescriptor::Disabled;

    desc->configurable = PropertyDescriptor::Undefined;
    if (o->__hasProperty__(ctx, ctx->engine->id_configurable))
        desc->configurable = o->get(ctx, ctx->engine->id_configurable).toBoolean() ? PropertyDescriptor::Enabled : PropertyDescriptor::Disabled;

    desc->get = 0;
    if (o->__hasProperty__(ctx, ctx->engine->id_get)) {
        Value get = o->get(ctx, ctx->engine->id_get);
        FunctionObject *f = get.asFunctionObject();
        if (f) {
            desc->get = f;
        } else if (get.isUndefined()) {
            desc->get = (FunctionObject *)0x1;
        } else {
            ctx->throwTypeError();
        }
        desc->type = PropertyDescriptor::Accessor;
    }

    desc->set = 0;
    if (o->__hasProperty__(ctx, ctx->engine->id_set)) {
        Value set = o->get(ctx, ctx->engine->id_set);
        FunctionObject *f = set.asFunctionObject();
        if (f) {
            desc->set = f;
        } else if (set.isUndefined()) {
            desc->set = (FunctionObject *)0x1;
        } else {
            ctx->throwTypeError();
        }
        desc->type = PropertyDescriptor::Accessor;
    }

    desc->writable = PropertyDescriptor::Undefined;
    if (o->__hasProperty__(ctx, ctx->engine->id_writable)) {
        if (desc->isAccessor())
            ctx->throwTypeError();
        desc->writable = o->get(ctx, ctx->engine->id_writable).toBoolean() ? PropertyDescriptor::Enabled : PropertyDescriptor::Disabled;
        // writable forces it to be a data descriptor
        desc->value = Value::undefinedValue();
    }

    if (o->__hasProperty__(ctx, ctx->engine->id_value)) {
        if (desc->isAccessor())
            ctx->throwTypeError();
        desc->value = o->get(ctx, ctx->engine->id_value);
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
    pd.enumerable = PropertyDescriptor::Enabled;
    pd.configurable = PropertyDescriptor::Enabled;

    if (desc->isData()) {
        pd.value = desc->value;
        o->__defineOwnProperty__(ctx, engine->newString(QStringLiteral("value")), &pd);
        pd.value = Value::fromBoolean(desc->writable == PropertyDescriptor::Enabled ? true : false);
        o->__defineOwnProperty__(ctx, engine->newString(QStringLiteral("writable")), &pd);
    } else {
        pd.value = desc->get ? Value::fromObject(desc->get) : Value::undefinedValue();
        o->__defineOwnProperty__(ctx, engine->newString(QStringLiteral("get")), &pd);
        pd.value = desc->set ? Value::fromObject(desc->set) : Value::undefinedValue();
        o->__defineOwnProperty__(ctx, engine->newString(QStringLiteral("set")), &pd);
    }
    pd.value = Value::fromBoolean(desc->enumerable == PropertyDescriptor::Enabled ? true : false);
    o->__defineOwnProperty__(ctx, engine->newString(QStringLiteral("enumerable")), &pd);
    pd.value = Value::fromBoolean(desc->configurable == PropertyDescriptor::Enabled ? true : false);
    o->__defineOwnProperty__(ctx, engine->newString(QStringLiteral("configurable")), &pd);

    return Value::fromObject(o);
}
