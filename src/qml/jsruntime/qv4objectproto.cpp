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


#include "qv4objectproto_p.h"
#include "qv4mm_p.h"
#include "qv4scopedvalue_p.h"
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

using namespace QV4;


DEFINE_MANAGED_VTABLE(ObjectCtor);

ObjectCtor::ObjectCtor(ExecutionContext *scope)
    : FunctionObject(scope, scope->engine->newIdentifier(QStringLiteral("Object")))
{
    vtbl = &static_vtbl;
}

ReturnedValue ObjectCtor::construct(Managed *that, CallData *callData)
{
    ExecutionEngine *v4 = that->engine();
    Scope scope(v4);
    ObjectCtor *ctor = static_cast<ObjectCtor *>(that);
    if (!callData->argc || callData->args[0].isUndefined() || callData->args[0].isNull()) {
        Scoped<Object> obj(scope, v4->newObject());
        Scoped<Object> proto(scope, ctor->get(v4->id_prototype));
        if (!!proto)
            obj->setPrototype(proto.getPointer());
        return obj.asReturnedValue();
    }
    return Value::fromReturnedValue(__qmljs_to_object(v4->current, ValueRef(&callData->args[0]))).asReturnedValue();
}

ReturnedValue ObjectCtor::call(Managed *m, CallData *callData)
{
    if (!callData->argc || callData->args[0].isUndefined() || callData->args[0].isNull())
        return m->engine()->newObject()->asReturnedValue();
    return __qmljs_to_object(m->engine()->current, ValueRef(&callData->args[0]));
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
    defineDefaultProperty(ctx, QStringLiteral("__defineGetter__"), method_defineGetter, 2);
    defineDefaultProperty(ctx, QStringLiteral("__defineSetter__"), method_defineSetter, 2);

    ExecutionEngine *v4 = ctx->engine;
    Property *p = insertMember(v4->id___proto__, Attr_Accessor|Attr_NotEnumerable);
    p->setGetter(v4->newBuiltinFunction(v4->rootContext, v4->id___proto__, method_get_proto));
    p->setSetter(v4->newBuiltinFunction(v4->rootContext, v4->id___proto__, method_set_proto));
}

ReturnedValue ObjectPrototype::method_getPrototypeOf(SimpleCallContext *ctx)
{
    Scope scope(ctx);
    Scoped<Object> o(scope, ctx->argument(0));
    if (!o)
        ctx->throwTypeError();

    Scoped<Object> p(scope, o->prototype());
    return !!p ? p->asReturnedValue() : Encode::null();
}

ReturnedValue ObjectPrototype::method_getOwnPropertyDescriptor(SimpleCallContext *ctx)
{
    Scope scope(ctx);
    Scoped<Object> O(scope, ctx->argument(0));
    if (!O)
        ctx->throwTypeError();

    ScopedValue v(scope, ctx->argument(1));
    Scoped<String> name(scope, v->toString(ctx));
    PropertyAttributes attrs;
    Property *desc = O->__getOwnProperty__(name.getPointer(), &attrs);
    return fromPropertyDescriptor(ctx, desc, attrs);
}

ReturnedValue ObjectPrototype::method_getOwnPropertyNames(SimpleCallContext *context)
{
    Object *O = context->argumentCount ? context->arguments[0].asObject() : 0;
    if (!O)
        context->throwTypeError();

    ArrayObject *array = getOwnPropertyNames(context->engine, context->arguments[0]);
    return Value::fromObject(array).asReturnedValue();
}

ReturnedValue ObjectPrototype::method_create(SimpleCallContext *ctx)
{
    Scope scope(ctx);
    ScopedValue O(scope, ctx->argument(0));
    if (!O->isObject() && !O->isNull())
        ctx->throwTypeError();

    Scoped<Object> newObject(scope, ctx->engine->newObject());
    newObject->setPrototype(O->asObject());

    if (ctx->argumentCount > 1 && !ctx->arguments[1].isUndefined()) {
        ctx->arguments[0] = newObject.asValue();
        return method_defineProperties(ctx);
    }

    return newObject.asReturnedValue();
}

ReturnedValue ObjectPrototype::method_defineProperty(SimpleCallContext *ctx)
{
    Scope scope(ctx);
    Scoped<Object> O(scope, ctx->argument(0));
    if (!O)
        ctx->throwTypeError();

    Scoped<String> name(scope, ctx->argument(1), Scoped<String>::Convert);

    ScopedValue attributes(scope, ctx->argument(2));
    Property pd;
    PropertyAttributes attrs;
    toPropertyDescriptor(ctx, attributes, &pd, &attrs);

    if (!O->__defineOwnProperty__(ctx, name.getPointer(), pd, attrs))
        ctx->throwTypeError();

    return O.asReturnedValue();
}

ReturnedValue ObjectPrototype::method_defineProperties(SimpleCallContext *ctx)
{
    Scope scope(ctx);
    Scoped<Object> O(scope, ctx->argument(0));
    if (!O)
        ctx->throwTypeError();

    Scoped<Object> o(scope, ctx->argument(1), Scoped<Object>::Convert);

    ObjectIterator it(o.getPointer(), ObjectIterator::EnumerableOnly);
    while (1) {
        uint index;
        String *name;
        PropertyAttributes attrs;
        Property *pd = it.next(&name, &index, &attrs);
        if (!pd)
            break;
        Property n;
        PropertyAttributes nattrs;
        toPropertyDescriptor(ctx, Value::fromReturnedValue(o->getValue(pd, attrs)), &n, &nattrs);
        bool ok;
        if (name)
            ok = O->__defineOwnProperty__(ctx, name, n, nattrs);
        else
            ok = O->__defineOwnProperty__(ctx, index, n, nattrs);
        if (!ok)
            ctx->throwTypeError();
    }

    return O.asReturnedValue();
}

ReturnedValue ObjectPrototype::method_seal(SimpleCallContext *ctx)
{
    Scope scope(ctx);
    Scoped<Object> o(scope, ctx->argument(0));
    if (!o)
        ctx->throwTypeError();

    o->extensible = false;

    o->internalClass = o->internalClass->sealed();

    o->ensureArrayAttributes();
    for (uint i = 0; i < o->arrayDataLen; ++i) {
        if (!o->arrayAttributes[i].isGeneric())
            o->arrayAttributes[i].setConfigurable(false);
    }

    return o.asReturnedValue();
}

ReturnedValue ObjectPrototype::method_freeze(SimpleCallContext *ctx)
{
    Scope scope(ctx);
    Scoped<Object> o(scope, ctx->argument(0));
    if (!o)
        ctx->throwTypeError();

    o->extensible = false;

    o->internalClass = o->internalClass->frozen();

    o->ensureArrayAttributes();
    for (uint i = 0; i < o->arrayDataLen; ++i) {
        if (!o->arrayAttributes[i].isGeneric())
            o->arrayAttributes[i].setConfigurable(false);
        if (o->arrayAttributes[i].isData())
            o->arrayAttributes[i].setWritable(false);
    }
    return o.asReturnedValue();
}

ReturnedValue ObjectPrototype::method_preventExtensions(SimpleCallContext *ctx)
{
    Scope scope(ctx);
    Scoped<Object> o(scope, ctx->argument(0));
    if (!o)
        ctx->throwTypeError();

    o->extensible = false;
    return o.asReturnedValue();
}

ReturnedValue ObjectPrototype::method_isSealed(SimpleCallContext *ctx)
{
    Scope scope(ctx);
    Scoped<Object> o(scope, ctx->argument(0));
    if (!o)
        ctx->throwTypeError();

    if (o->extensible)
        return Encode(false);

    if (o->internalClass != o->internalClass->sealed())
        return Encode(false);

    if (!o->arrayDataLen)
        return Encode(true);

    if (!o->arrayAttributes)
        return Encode(false);

    for (uint i = 0; i < o->arrayDataLen; ++i) {
        if (!o->arrayAttributes[i].isGeneric())
            if (o->arrayAttributes[i].isConfigurable())
                return Encode(false);
    }

    return Encode(true);
}

ReturnedValue ObjectPrototype::method_isFrozen(SimpleCallContext *ctx)
{
    Scope scope(ctx);
    Scoped<Object> o(scope, ctx->argument(0));
    if (!o)
        ctx->throwTypeError();

    if (o->extensible)
        return Encode(false);

    if (o->internalClass != o->internalClass->frozen())
        return Encode(false);

    if (!o->arrayDataLen)
        return Encode(true);

    if (!o->arrayAttributes)
        return Encode(false);

    for (uint i = 0; i < o->arrayDataLen; ++i) {
        if (!o->arrayAttributes[i].isGeneric())
            if (o->arrayAttributes[i].isConfigurable() || o->arrayAttributes[i].isWritable())
                return Encode(false);
    }

    return Encode(true);
}

ReturnedValue ObjectPrototype::method_isExtensible(SimpleCallContext *ctx)
{
    Scope scope(ctx);
    Scoped<Object> o(scope, ctx->argument(0));
    if (!o)
        ctx->throwTypeError();

    return Encode((bool)o->extensible);
}

ReturnedValue ObjectPrototype::method_keys(SimpleCallContext *ctx)
{
    Scope scope(ctx);
    Scoped<Object> o(scope, ctx->argument(0));
    if (!o)
        ctx->throwTypeError();

    Scoped<ArrayObject> a(scope, ctx->engine->newArrayObject());

    ObjectIterator it(o.getPointer(), ObjectIterator::EnumerableOnly);
    ScopedValue name(scope);
    while (1) {
        name = it.nextPropertyNameAsString();
        if (name->isNull())
            break;
        a->push_back(name);
    }

    return a.asReturnedValue();
}

ReturnedValue ObjectPrototype::method_toString(SimpleCallContext *ctx)
{
    if (ctx->thisObject.isUndefined()) {
        return Value::fromString(ctx, QStringLiteral("[object Undefined]")).asReturnedValue();
    } else if (ctx->thisObject.isNull()) {
        return Value::fromString(ctx, QStringLiteral("[object Null]")).asReturnedValue();
    } else {
        Value obj = Value::fromReturnedValue(__qmljs_to_object(ctx, ValueRef(&ctx->thisObject)));
        QString className = obj.objectValue()->className();
        return Value::fromString(ctx, QString::fromUtf8("[object %1]").arg(className)).asReturnedValue();
    }
}

ReturnedValue ObjectPrototype::method_toLocaleString(SimpleCallContext *ctx)
{
    Scope scope(ctx);
    Object *o = ctx->thisObject.toObject(ctx);
    Scoped<FunctionObject> f(scope, o->get(ctx->engine->newString(QStringLiteral("toString"))));
    if (!f)
        ctx->throwTypeError();
    ScopedCallData callData(scope, 0);
    callData->thisObject = Value::fromObject(o);
    return f->call(callData);
}

ReturnedValue ObjectPrototype::method_valueOf(SimpleCallContext *ctx)
{
    return Value::fromObject(ctx->thisObject.toObject(ctx)).asReturnedValue();
}

ReturnedValue ObjectPrototype::method_hasOwnProperty(SimpleCallContext *ctx)
{
    Scope scope(ctx);
    Scoped<String> P(scope, ctx->argument(0), Scoped<String>::Convert);
    Scoped<Object> O(scope, ctx->thisObject, Scoped<Object>::Convert);
    bool r = O->__getOwnProperty__(P.getPointer()) != 0;
    if (!r)
        r = !O->query(P.getPointer()).isEmpty();
    return Encode(r);
}

ReturnedValue ObjectPrototype::method_isPrototypeOf(SimpleCallContext *ctx)
{
    Scope scope(ctx);
    Scoped<Object> V(scope, ctx->argument(0));
    if (!V)
        return Encode(false);

    Scoped<Object> O(scope, ctx->thisObject, Scoped<Object>::Convert);
    Scoped<Object> proto(scope, V->prototype());
    while (proto) {
        if (O.getPointer() == proto.getPointer())
            return Encode(true);
        proto = proto->prototype();
    }
    return Encode(false);
}

ReturnedValue ObjectPrototype::method_propertyIsEnumerable(SimpleCallContext *ctx)
{
    Scope scope(ctx);
    Scoped<String> p(scope, ctx->argument(0), Scoped<String>::Convert);

    Scoped<Object> o(scope, ctx->thisObject, Scoped<Object>::Convert);
    PropertyAttributes attrs;
    o->__getOwnProperty__(p.getPointer(), &attrs);
    return Encode(attrs.isEnumerable());
}

ReturnedValue ObjectPrototype::method_defineGetter(SimpleCallContext *ctx)
{
    if (ctx->argumentCount < 2)
        ctx->throwTypeError();

    Scope scope(ctx);
    Scoped<String> prop(scope, ctx->argument(0), Scoped<String>::Convert);

    Scoped<FunctionObject> f(scope, ctx->argument(1));
    if (!f)
        ctx->throwTypeError();

    Scoped<Object> o(scope, ctx->thisObject);
    if (!o) {
        if (!ctx->thisObject.isUndefined())
            return Encode::undefined();
        o = ctx->engine->globalObject;
    }

    Property pd = Property::fromAccessor(f.getPointer(), 0);
    o->__defineOwnProperty__(ctx, prop.getPointer(), pd, Attr_Accessor);
    return Encode::undefined();
}

ReturnedValue ObjectPrototype::method_defineSetter(SimpleCallContext *ctx)
{
    if (ctx->argumentCount < 2)
        ctx->throwTypeError();

    Scope scope(ctx);
    Scoped<String> prop(scope, ctx->argument(0), Scoped<String>::Convert);

    Scoped<FunctionObject> f(scope, ctx->argument(1));
    if (!f)
        ctx->throwTypeError();

    Scoped<Object> o(scope, ctx->thisObject);
    if (!o) {
        if (!ctx->thisObject.isUndefined())
            return Encode::undefined();
        o = ctx->engine->globalObject;
    }

    Property pd = Property::fromAccessor(0, f.getPointer());
    o->__defineOwnProperty__(ctx, prop.getPointer(), pd, Attr_Accessor);
    return Encode::undefined();
}

ReturnedValue ObjectPrototype::method_get_proto(SimpleCallContext *ctx)
{
    Object *o = ctx->thisObject.asObject();
    if (!o)
        ctx->throwTypeError();

    return Value::fromObject(o->prototype()).asReturnedValue();
}

ReturnedValue ObjectPrototype::method_set_proto(SimpleCallContext *ctx)
{
    Scope scope(ctx);
    Scoped<Object> o(scope, ctx->thisObject);
    if (!o || !ctx->argumentCount)
        ctx->throwTypeError();

    if (ctx->arguments[0].isNull()) {
        o->setPrototype(0);
        return Encode::undefined();
    }

    Scoped<Object> p(scope, ctx->arguments[0]);
    bool ok = false;
    if (!!p) {
        if (o->prototype() == p.getPointer()) {
            ok = true;
        } else if (o->extensible) {
            ok = o->setPrototype(p.getPointer());
        }
    }
    if (!ok)
        ctx->throwTypeError(QStringLiteral("Cyclic __proto__ value"));
    return Encode::undefined();
}

void ObjectPrototype::toPropertyDescriptor(ExecutionContext *ctx, Value v, Property *desc, PropertyAttributes *attrs)
{
    if (!v.isObject())
        ctx->throwTypeError();

    Scope scope(ctx);
    Object *o = v.objectValue();

    attrs->clear();
    desc->setGetter(0);
    desc->setSetter(0);

    if (o->__hasProperty__(ctx->engine->id_enumerable))
        attrs->setEnumerable(Value::fromReturnedValue(o->get(ctx->engine->id_enumerable)).toBoolean());

    if (o->__hasProperty__(ctx->engine->id_configurable))
        attrs->setConfigurable(Value::fromReturnedValue(o->get(ctx->engine->id_configurable)).toBoolean());

    if (o->__hasProperty__(ctx->engine->id_get)) {
        ScopedValue get(scope, o->get(ctx->engine->id_get));
        FunctionObject *f = get->asFunctionObject();
        qDebug() << "get" << (void *)get.asReturnedValue() << f;
        if (f) {
            desc->setGetter(f);
        } else if (get->isUndefined()) {
            desc->setGetter((FunctionObject *)0x1);
        } else {
            ctx->throwTypeError();
        }
        attrs->setType(PropertyAttributes::Accessor);
    }

    if (o->__hasProperty__(ctx->engine->id_set)) {
        ScopedValue set(scope, o->get(ctx->engine->id_set));
        FunctionObject *f = set->asFunctionObject();
        if (f) {
            desc->setSetter(f);
        } else if (set->isUndefined()) {
            desc->setSetter((FunctionObject *)0x1);
        } else {
            ctx->throwTypeError();
        }
        attrs->setType(PropertyAttributes::Accessor);
    }

    if (o->__hasProperty__(ctx->engine->id_writable)) {
        if (attrs->isAccessor())
            ctx->throwTypeError();
        attrs->setWritable(Value::fromReturnedValue(o->get(ctx->engine->id_writable)).toBoolean());
        // writable forces it to be a data descriptor
        desc->value = Value::undefinedValue();
    }

    if (o->__hasProperty__(ctx->engine->id_value)) {
        if (attrs->isAccessor())
            ctx->throwTypeError();
        desc->value = Value::fromReturnedValue(o->get(ctx->engine->id_value));
        attrs->setType(PropertyAttributes::Data);
    }

    if (attrs->isGeneric())
        desc->value = Value::emptyValue();
}


ReturnedValue ObjectPrototype::fromPropertyDescriptor(ExecutionContext *ctx, const Property *desc, PropertyAttributes attrs)
{
    if (!desc)
        return Encode::undefined();

    ExecutionEngine *engine = ctx->engine;
    Scope scope(engine);
//    Let obj be the result of creating a new object as if by the expression new Object() where Object is the standard built-in constructor with that name.
    Scoped<Object> o(scope, engine->newObject());

    Property pd;
    if (attrs.isData()) {
        pd.value = desc->value;
        o->__defineOwnProperty__(ctx, engine->newString(QStringLiteral("value")), pd, Attr_Data);
        pd.value = Value::fromBoolean(attrs.isWritable());
        o->__defineOwnProperty__(ctx, engine->newString(QStringLiteral("writable")), pd, Attr_Data);
    } else {
        pd.value = desc->getter() ? Value::fromObject(desc->getter()) : Value::undefinedValue();
        o->__defineOwnProperty__(ctx, engine->newString(QStringLiteral("get")), pd, Attr_Data);
        pd.value = desc->setter() ? Value::fromObject(desc->setter()) : Value::undefinedValue();
        o->__defineOwnProperty__(ctx, engine->newString(QStringLiteral("set")), pd, Attr_Data);
    }
    pd.value = Value::fromBoolean(attrs.isEnumerable());
    o->__defineOwnProperty__(ctx, engine->newString(QStringLiteral("enumerable")), pd, Attr_Data);
    pd.value = Value::fromBoolean(attrs.isConfigurable());
    o->__defineOwnProperty__(ctx, engine->newString(QStringLiteral("configurable")), pd, Attr_Data);

    return o.asReturnedValue();
}


ArrayObject *ObjectPrototype::getOwnPropertyNames(ExecutionEngine *v4, const Value &o)
{
    Scope scope(v4);
    Scoped<ArrayObject> array(scope, v4->newArrayObject());
    Object *O = o.asObject();
    if (!O)
        return array.getPointer();

    ObjectIterator it(O, ObjectIterator::NoFlags);
    ScopedValue name(scope);
    while (1) {
        name = it.nextPropertyNameAsString();
        if (name->isNull())
            break;
        array->push_back(name);
    }
    return array.getPointer();
}
