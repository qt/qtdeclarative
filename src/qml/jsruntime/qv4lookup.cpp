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
#include "qv4lookup_p.h"
#include "qv4functionobject_p.h"
#include "qv4scopedvalue_p.h"

QT_BEGIN_NAMESPACE

using namespace QV4;

Property *Lookup::lookup(Object *obj, PropertyAttributes *attrs)
{
    int i = 0;
    while (i < Size && obj) {
        classList[i] = obj->internalClass;

        index = obj->internalClass->find(name);
        if (index != UINT_MAX) {
            level = i;
            *attrs = obj->internalClass->propertyData.at(index);
            return obj->memberData + index;
        }

        obj = obj->prototype();
        ++i;
    }
    level = Size;

    while (obj) {
        index = obj->internalClass->find(name);
        if (index != UINT_MAX) {
            *attrs = obj->internalClass->propertyData.at(index);
            return obj->memberData + index;
        }

        obj = obj->prototype();
    }
    return 0;
}


ReturnedValue Lookup::getterGeneric(QV4::Lookup *l, const ValueRef object)
{
    if (Object *o = object->asObject())
        return o->getLookup(l);

    ExecutionEngine *engine = l->name->engine();
    Object *proto;
    switch (object->type()) {
    case Value::Undefined_Type:
    case Value::Null_Type:
        return engine->currentContext()->throwTypeError();
    case Value::Boolean_Type:
        proto = engine->booleanClass->prototype;
        break;
    case Value::Managed_Type:
        Q_ASSERT(object->isString());
        proto = engine->stringObjectClass->prototype;
        if (l->name->equals(engine->id_length)) {
            // special case, as the property is on the object itself
            l->getter = stringLengthGetter;
            return stringLengthGetter(l, object);
        }
        break;
    case Value::Integer_Type:
    default: // Number
        proto = engine->numberClass->prototype;
    }

    PropertyAttributes attrs;
    Property *p = l->lookup(proto, &attrs);
    if (p) {
        l->type = object->type();
        l->proto = proto;
        if (attrs.isData()) {
            if (l->level == 0)
                l->getter = Lookup::primitiveGetter0;
            else if (l->level == 1)
                l->getter = Lookup::primitiveGetter1;
            return p->value.asReturnedValue();
        } else {
            if (l->level == 0)
                l->getter = Lookup::primitiveGetterAccessor0;
            else if (l->level == 1)
                l->getter = Lookup::primitiveGetterAccessor1;
            return proto->getValue(object, p, attrs);
        }
    }

    return Encode::undefined();
}

ReturnedValue Lookup::getter0(Lookup *l, const ValueRef object)
{
    if (object->isManaged()) {
        // we can safely cast to a QV4::Object here. If object is actually a string,
        // the internal class won't match
        Object *o = object->objectValue();
        if (l->classList[0] == o->internalClass)
            return static_cast<Object *>(o)->memberData[l->index].value.asReturnedValue();
    }
    l->getter = getterGeneric;
    return getterGeneric(l, object);
}

ReturnedValue Lookup::getter1(Lookup *l, const ValueRef object)
{
    if (object->isManaged()) {
        // we can safely cast to a QV4::Object here. If object is actually a string,
        // the internal class won't match
        Object *o = object->objectValue();
        if (l->classList[0] == o->internalClass &&
            l->classList[1] == o->prototype()->internalClass)
            return o->prototype()->memberData[l->index].value.asReturnedValue();
    }
    l->getter = getterGeneric;
    return getterGeneric(l, object);
}

ReturnedValue Lookup::getter2(Lookup *l, const ValueRef object)
{
    if (object->isManaged()) {
        // we can safely cast to a QV4::Object here. If object is actually a string,
        // the internal class won't match
        Object *o = object->objectValue();
        if (l->classList[0] == o->internalClass) {
            o = o->prototype();
            if (l->classList[1] == o->internalClass) {
                o = o->prototype();
                if (l->classList[2] == o->internalClass)
                    return o->memberData[l->index].value.asReturnedValue();
            }
        }
    }
    l->getter = getterGeneric;
    return getterGeneric(l, object);
}

ReturnedValue Lookup::getterAccessor0(Lookup *l, const ValueRef object)
{
    if (object->isManaged()) {
        // we can safely cast to a QV4::Object here. If object is actually a string,
        // the internal class won't match
        Object *o = object->objectValue();
        if (l->classList[0] == o->internalClass) {
            Scope scope(o->engine());
            FunctionObject *getter = o->memberData[l->index].getter();
            if (!getter)
                return Encode::undefined();

            ScopedCallData callData(scope, 0);
            callData->thisObject = *object;
            return getter->call(callData);
        }
    }
    l->getter = getterGeneric;
    return getterGeneric(l, object);
}

ReturnedValue Lookup::getterAccessor1(Lookup *l, const ValueRef object)
{
    if (object->isManaged()) {
        // we can safely cast to a QV4::Object here. If object is actually a string,
        // the internal class won't match
        Object *o = object->objectValue();
        if (l->classList[0] == o->internalClass &&
            l->classList[1] == o->prototype()->internalClass) {
            Scope scope(o->engine());
            FunctionObject *getter = o->prototype()->memberData[l->index].getter();
            if (!getter)
                return Encode::undefined();

            ScopedCallData callData(scope, 0);
            callData->thisObject = *object;
            return getter->call(callData);
        }
    }
    l->getter = getterGeneric;
    return getterGeneric(l, object);
}

ReturnedValue Lookup::getterAccessor2(Lookup *l, const ValueRef object)
{
    if (object->isManaged()) {
        // we can safely cast to a QV4::Object here. If object is actually a string,
        // the internal class won't match
        Object *o = object->objectValue();
        if (l->classList[0] == o->internalClass) {
            o = o->prototype();
            if (l->classList[1] == o->internalClass) {
                o = o->prototype();
                if (l->classList[2] == o->internalClass) {
                    Scope scope(o->engine());
                    FunctionObject *getter = o->memberData[l->index].getter();
                    if (!getter)
                        return Encode::undefined();

                    ScopedCallData callData(scope, 0);
                    callData->thisObject = *object;
                    return getter->call(callData);
                }
            }
        }
    }
    l->getter = getterGeneric;
    return getterGeneric(l, object);
}


ReturnedValue Lookup::primitiveGetter0(Lookup *l, const ValueRef object)
{
    if (object->type() == l->type) {
        Object *o = l->proto;
        if (l->classList[0] == o->internalClass)
            return o->memberData[l->index].value.asReturnedValue();
    }
    l->getter = getterGeneric;
    return getterGeneric(l, object);
}

ReturnedValue Lookup::primitiveGetter1(Lookup *l, const ValueRef object)
{
    if (object->type() == l->type) {
        Object *o = l->proto;
        if (l->classList[0] == o->internalClass &&
            l->classList[1] == o->prototype()->internalClass)
            return o->prototype()->memberData[l->index].value.asReturnedValue();
    }
    l->getter = getterGeneric;
    return getterGeneric(l, object);
}

ReturnedValue Lookup::primitiveGetterAccessor0(Lookup *l, const ValueRef object)
{
    if (object->type() == l->type) {
        Object *o = l->proto;
        if (l->classList[0] == o->internalClass) {
            Scope scope(o->engine());
            FunctionObject *getter = o->memberData[l->index].getter();
            if (!getter)
                return Encode::undefined();

            ScopedCallData callData(scope, 0);
            callData->thisObject = *object;
            return getter->call(callData);
        }
    }
    l->getter = getterGeneric;
    return getterGeneric(l, object);
}

ReturnedValue Lookup::primitiveGetterAccessor1(Lookup *l, const ValueRef object)
{
    if (object->type() == l->type) {
        Object *o = l->proto;
        if (l->classList[0] == o->internalClass &&
            l->classList[1] == o->prototype()->internalClass) {
            Scope scope(o->engine());
            FunctionObject *getter = o->prototype()->memberData[l->index].getter();
            if (!getter)
                return Encode::undefined();

            ScopedCallData callData(scope, 0);
            callData->thisObject = *object;
            return getter->call(callData);
        }
    }
    l->getter = getterGeneric;
    return getterGeneric(l, object);
}

ReturnedValue Lookup::stringLengthGetter(Lookup *l, const ValueRef object)
{
    if (String *s = object->asString())
        return Encode(s->length());

    l->getter = getterGeneric;
    return getterGeneric(l, object);
}


ReturnedValue Lookup::globalGetterGeneric(Lookup *l, ExecutionContext *ctx)
{
    Object *o = ctx->engine->globalObject;
    PropertyAttributes attrs;
    Property *p = l->lookup(o, &attrs);
    if (p) {
        if (attrs.isData()) {
            if (l->level == 0)
                l->globalGetter = globalGetter0;
            else if (l->level == 1)
                l->globalGetter = globalGetter1;
            else if (l->level == 2)
                l->globalGetter = globalGetter2;
            return p->value.asReturnedValue();
        } else {
            if (l->level == 0)
                l->globalGetter = globalGetterAccessor0;
            else if (l->level == 1)
                l->globalGetter = globalGetterAccessor1;
            else if (l->level == 2)
                l->globalGetter = globalGetterAccessor2;
            return o->getValue(p, attrs);
        }
    }
    Scope scope(ctx);
    Scoped<String> n(scope, l->name);
    return ctx->throwReferenceError(n);
}

ReturnedValue Lookup::globalGetter0(Lookup *l, ExecutionContext *ctx)
{
    Object *o = ctx->engine->globalObject;
    if (l->classList[0] == o->internalClass)
        return o->memberData[l->index].value.asReturnedValue();

    l->globalGetter = globalGetterGeneric;
    return globalGetterGeneric(l, ctx);
}

ReturnedValue Lookup::globalGetter1(Lookup *l, ExecutionContext *ctx)
{
    Object *o = ctx->engine->globalObject;
    if (l->classList[0] == o->internalClass &&
        l->classList[1] == o->prototype()->internalClass)
        return o->prototype()->memberData[l->index].value.asReturnedValue();

    l->globalGetter = globalGetterGeneric;
    return globalGetterGeneric(l, ctx);
}

ReturnedValue Lookup::globalGetter2(Lookup *l, ExecutionContext *ctx)
{
    Object *o = ctx->engine->globalObject;
    if (l->classList[0] == o->internalClass) {
        o = o->prototype();
        if (l->classList[1] == o->internalClass) {
            o = o->prototype();
            if (l->classList[2] == o->internalClass) {
                return o->prototype()->memberData[l->index].value.asReturnedValue();
            }
        }
    }
    l->globalGetter = globalGetterGeneric;
    return globalGetterGeneric(l, ctx);
}

ReturnedValue Lookup::globalGetterAccessor0(Lookup *l, ExecutionContext *ctx)
{
    Object *o = ctx->engine->globalObject;
    if (l->classList[0] == o->internalClass) {
        Scope scope(o->engine());
        FunctionObject *getter = o->memberData[l->index].getter();
        if (!getter)
            return Encode::undefined();

        ScopedCallData callData(scope, 0);
        callData->thisObject = Primitive::undefinedValue();
        return getter->call(callData);
    }
    l->globalGetter = globalGetterGeneric;
    return globalGetterGeneric(l, ctx);
}

ReturnedValue Lookup::globalGetterAccessor1(Lookup *l, ExecutionContext *ctx)
{
    Object *o = ctx->engine->globalObject;
    if (l->classList[0] == o->internalClass &&
        l->classList[1] == o->prototype()->internalClass) {
        Scope scope(o->engine());
        FunctionObject *getter = o->prototype()->memberData[l->index].getter();
        if (!getter)
            return Encode::undefined();

        ScopedCallData callData(scope, 0);
        callData->thisObject = Primitive::undefinedValue();
        return getter->call(callData);
    }
    l->globalGetter = globalGetterGeneric;
    return globalGetterGeneric(l, ctx);
}

ReturnedValue Lookup::globalGetterAccessor2(Lookup *l, ExecutionContext *ctx)
{
    Object *o = ctx->engine->globalObject;
    if (l->classList[0] == o->internalClass) {
        o = o->prototype();
        if (l->classList[1] == o->internalClass) {
            o = o->prototype();
            if (l->classList[2] == o->internalClass) {
                Scope scope(o->engine());
                FunctionObject *getter = o->memberData[l->index].getter();
                if (!getter)
                    return Encode::undefined();

                ScopedCallData callData(scope, 0);
                callData->thisObject = Primitive::undefinedValue();
                return getter->call(callData);
            }
        }
    }
    l->globalGetter = globalGetterGeneric;
    return globalGetterGeneric(l, ctx);
}

void Lookup::setterGeneric(Lookup *l, const ValueRef object, const ValueRef value)
{
    Scope scope(l->name->engine());
    ScopedObject o(scope, object);
    if (!o) {
        o = __qmljs_convert_to_object(scope.engine->currentContext(), object);
        if (!o) // type error
            return;
        ScopedString s(scope, l->name);
        o->put(s, value);
        return;
    }
    o->setLookup(l, value);
}

void Lookup::setter0(Lookup *l, const ValueRef object, const ValueRef value)
{
    Object *o = object->asObject();
    if (o && o->internalClass == l->classList[0]) {
        o->memberData[l->index].value = *value;
        return;
    }

    l->setter = setterGeneric;
    setterGeneric(l, object, value);
}

void Lookup::setterInsert0(Lookup *l, const ValueRef object, const ValueRef value)
{
    Object *o = object->asObject();
    if (o && o->internalClass == l->classList[0]) {
        if (!o->prototype()) {
            if (l->index >= o->memberDataAlloc)
                o->ensureMemberIndex(l->index);
            o->memberData[l->index].value = *value;
            o->internalClass = l->classList[3];
            return;
        }
    }

    l->setter = setterGeneric;
    setterGeneric(l, object, value);
}

void Lookup::setterInsert1(Lookup *l, const ValueRef object, const ValueRef value)
{
    Object *o = object->asObject();
    if (o && o->internalClass == l->classList[0]) {
        Object *p = o->prototype();
        if (p && p->internalClass == l->classList[1]) {
            if (l->index >= o->memberDataAlloc)
                o->ensureMemberIndex(l->index);
            o->memberData[l->index].value = *value;
            o->internalClass = l->classList[3];
            return;
        }
    }

    l->setter = setterGeneric;
    setterGeneric(l, object, value);
}

void Lookup::setterInsert2(Lookup *l, const ValueRef object, const ValueRef value)
{
    Object *o = object->asObject();
    if (o && o->internalClass == l->classList[0]) {
        Object *p = o->prototype();
        if (p && p->internalClass == l->classList[1]) {
            p = p->prototype();
            if (p && p->internalClass == l->classList[2]) {
                if (l->index >= o->memberDataAlloc)
                    o->ensureMemberIndex(l->index);
                o->memberData[l->index].value = *value;
                o->internalClass = l->classList[3];
                return;
            }
        }
    }

    l->setter = setterGeneric;
    setterGeneric(l, object, value);
}

QT_END_NAMESPACE
