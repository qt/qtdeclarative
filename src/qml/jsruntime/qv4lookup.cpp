/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include "qv4lookup_p.h"
#include "qv4functionobject_p.h"
#include "qv4jscall_p.h"
#include "qv4string_p.h"
#include <private/qv4identifiertable_p.h>

QT_BEGIN_NAMESPACE

using namespace QV4;


ReturnedValue Lookup::lookup(const Value &thisObject, Object *o, PropertyAttributes *attrs)
{
    ExecutionEngine *engine = o->engine();
    Identifier *name = engine->identifierTable->identifier(engine->currentStackFrame->v4Function->compilationUnit->runtimeStrings[nameIndex]);
    int i = 0;
    Heap::Object *obj = o->d();
    while (i < Size && obj) {
        classList[i] = obj->internalClass;

        index = obj->internalClass->find(name);
        if (index != UINT_MAX) {
            level = i;
            *attrs = obj->internalClass->propertyData.at(index);
            const Value *v = obj->propertyData(index);
            return !attrs->isAccessor() ? v->asReturnedValue() : Object::getValue(thisObject, *v, *attrs);
        }

        obj = obj->prototype();
        ++i;
    }
    level = Size;

    while (obj) {
        index = obj->internalClass->find(name);
        if (index != UINT_MAX) {
            *attrs = obj->internalClass->propertyData.at(index);
            const Value *v = obj->propertyData(index);
            return !attrs->isAccessor() ? v->asReturnedValue() : Object::getValue(thisObject, *v, *attrs);
        }

        obj = obj->prototype();
    }
    return Primitive::emptyValue().asReturnedValue();
}

ReturnedValue Lookup::lookup(const Object *thisObject, PropertyAttributes *attrs)
{
    Heap::Object *obj = thisObject->d();
    ExecutionEngine *engine = thisObject->engine();
    Identifier *name = engine->identifierTable->identifier(engine->currentStackFrame->v4Function->compilationUnit->runtimeStrings[nameIndex]);

    int i = 0;
    while (i < Size && obj) {
        classList[i] = obj->internalClass;

        index = obj->internalClass->find(name);
        if (index != UINT_MAX) {
            level = i;
            *attrs = obj->internalClass->propertyData.at(index);
            const Value *v = obj->propertyData(index);
            return !attrs->isAccessor() ? v->asReturnedValue() : thisObject->getValue(*v, *attrs);
        }

        obj = obj->prototype();
        ++i;
    }
    level = Size;

    while (obj) {
        index = obj->internalClass->find(name);
        if (index != UINT_MAX) {
            *attrs = obj->internalClass->propertyData.at(index);
            const Value *v = obj->propertyData(index);
            return !attrs->isAccessor() ? v->asReturnedValue() : thisObject->getValue(*v, *attrs);
        }

        obj = obj->prototype();
    }
    return Primitive::emptyValue().asReturnedValue();
}

void Lookup::resolveProtoGetter(Identifier *name, const Heap::Object *proto)
{
    while (proto) {
        uint index = proto->internalClass->find(name);
        if (index != UINT_MAX) {
            PropertyAttributes attrs = proto->internalClass->propertyData.at(index);
            protoLookup.data = proto->propertyData(index);
            if (attrs.isData()) {
                getter = getterProto;
            } else {
                getter = getterProtoAccessor;
            }
            return;
        }
        proto = proto->prototype();
    }
    // ### put in a getterNotFound!
    getter = getterFallback;
}

ReturnedValue Lookup::resolveGetter(ExecutionEngine *engine, const Object *object)
{
    Heap::Object *obj = object->d();
    Identifier *name = engine->identifierTable->identifier(engine->currentStackFrame->v4Function->compilationUnit->runtimeStrings[nameIndex]);

    uint index = obj->internalClass->find(name);
    if (index != UINT_MAX) {
        PropertyAttributes attrs = obj->internalClass->propertyData.at(index);
        uint nInline = obj->vtable()->nInlineProperties;
        if (attrs.isData()) {
            if (index < obj->vtable()->nInlineProperties) {
                index += obj->vtable()->inlinePropertyOffset;
                getter = getter0Inline;
            } else {
                index -= nInline;
                getter = getter0MemberData;
            }
        } else {
            getter = getterAccessor;
        }
        objectLookup.ic = obj->internalClass;
        objectLookup.offset = index;
        return getter(this, engine, *object);
    }

    protoLookup.icIdentifier = obj->internalClass->id;
    resolveProtoGetter(name, obj->prototype());
    return getter(this, engine, *object);
}

ReturnedValue Lookup::resolvePrimitiveGetter(ExecutionEngine *engine, const Value &object)
{
    primitiveLookup.type = object.type();
    switch (primitiveLookup.type) {
    case Value::Undefined_Type:
    case Value::Null_Type:
        return engine->throwTypeError();
    case Value::Boolean_Type:
        primitiveLookup.proto = engine->booleanPrototype()->d();
        break;
    case Value::Managed_Type: {
        // ### Should move this over to the Object path, as strings also have an internalClass
        Q_ASSERT(object.isString());
        primitiveLookup.proto = engine->stringPrototype()->d();
        Scope scope(engine);
        ScopedString name(scope, engine->currentStackFrame->v4Function->compilationUnit->runtimeStrings[nameIndex]);
        if (name->equals(engine->id_length())) {
            // special case, as the property is on the object itself
            getter = stringLengthGetter;
            return stringLengthGetter(this, engine, object);
        }
        break;
    }
    case Value::Integer_Type:
    default: // Number
        primitiveLookup.proto = engine->numberPrototype()->d();
    }

    Identifier *name = engine->identifierTable->identifier(engine->currentStackFrame->v4Function->compilationUnit->runtimeStrings[nameIndex]);
    protoLookup.icIdentifier = primitiveLookup.proto->internalClass->id;
    resolveProtoGetter(name, primitiveLookup.proto);

    if (getter == getterProto)
        getter = primitiveGetterProto;
    else if (getter == getterProtoAccessor)
        getter = primitiveGetterAccessor;
    return getter(this, engine, object);
}

ReturnedValue Lookup::getterGeneric(Lookup *l, ExecutionEngine *engine, const Value &object)
{
    if (const Object *o = object.as<Object>())
        return l->resolveGetter(engine, o);
    return l->resolvePrimitiveGetter(engine, object);
}

ReturnedValue Lookup::getterTwoClasses(Lookup *l, ExecutionEngine *engine, const Value &object)
{
    if (const Object *o = object.as<Object>()) {
        Lookup first = *l;
        Lookup second = *l;

        ReturnedValue result = second.resolveGetter(engine, o);

        if (first.getter == getter0Inline && (second.getter == getter0Inline || second.getter == getter0MemberData)) {
            l->objectLookupTwoClasses.ic = first.objectLookup.ic;
            l->objectLookupTwoClasses.ic2 = second.objectLookup.ic;
            l->objectLookupTwoClasses.offset = first.objectLookup.offset;
            l->objectLookupTwoClasses.offset2 = second.objectLookup.offset;
            l->getter = second.getter == getter0Inline ? getter0Inlinegetter0Inline : getter0Inlinegetter0MemberData;
            return result;
        }
        if (first.getter == getter0MemberData && (second.getter == getter0Inline || second.getter == getter0MemberData)) {
            l->objectLookupTwoClasses.ic = second.objectLookup.ic;
            l->objectLookupTwoClasses.ic2 = first.objectLookup.ic;
            l->objectLookupTwoClasses.offset = second.objectLookup.offset;
            l->objectLookupTwoClasses.offset2 = first.objectLookup.offset;
            l->getter = second.getter == getter0Inline ? getter0Inlinegetter0MemberData : getter0MemberDatagetter0MemberData;
            return result;
        }
        if (first.getter == getterProto && second.getter == getterProto) {
            l->protoLookupTwoClasses.icIdentifier = first.protoLookup.icIdentifier;
            l->protoLookupTwoClasses.icIdentifier2 = second.protoLookup.icIdentifier;
            l->protoLookupTwoClasses.data = first.protoLookup.data;
            l->protoLookupTwoClasses.data2 = second.protoLookup.data;
            l->getter = getterProtoTwoClasses;
            return result;
        }
        if (first.getter == getterProtoAccessor && second.getter == getterProtoAccessor) {
            l->protoLookupTwoClasses.icIdentifier = first.protoLookup.icIdentifier;
            l->protoLookupTwoClasses.icIdentifier2 = second.protoLookup.icIdentifier;
            l->protoLookupTwoClasses.data = first.protoLookup.data;
            l->protoLookupTwoClasses.data2 = second.protoLookup.data;
            l->getter = getterProtoAccessorTwoClasses;
            return result;
        }

    }

    l->getter = getterFallback;
    return getterFallback(l, engine, object);
}

ReturnedValue Lookup::getterFallback(Lookup *l, ExecutionEngine *engine, const Value &object)
{
    QV4::Scope scope(engine);
    QV4::ScopedObject o(scope, object.toObject(scope.engine));
    if (!o)
        return Encode::undefined();
    ScopedString name(scope, engine->currentStackFrame->v4Function->compilationUnit->runtimeStrings[l->nameIndex]);
    return o->get(name);
}

ReturnedValue Lookup::getter0MemberData(Lookup *l, ExecutionEngine *engine, const Value &object)
{
    // we can safely cast to a QV4::Object here. If object is actually a string,
    // the internal class won't match
    Heap::Object *o = static_cast<Heap::Object *>(object.heapObject());
    if (o) {
        if (l->objectLookup.ic == o->internalClass)
            return o->memberData->values.data()[l->objectLookup.offset].asReturnedValue();
    }
    return getterTwoClasses(l, engine, object);
}

ReturnedValue Lookup::getter0Inline(Lookup *l, ExecutionEngine *engine, const Value &object)
{
    // we can safely cast to a QV4::Object here. If object is actually a string,
    // the internal class won't match
    Heap::Object *o = static_cast<Heap::Object *>(object.heapObject());
    if (o) {
        if (l->objectLookup.ic == o->internalClass)
            return o->inlinePropertyDataWithOffset(l->objectLookup.offset)->asReturnedValue();
    }
    return getterTwoClasses(l, engine, object);
}

ReturnedValue Lookup::getterProto(Lookup *l, ExecutionEngine *engine, const Value &object)
{
    // we can safely cast to a QV4::Object here. If object is actually a string,
    // the internal class won't match
    Heap::Object *o = static_cast<Heap::Object *>(object.heapObject());
    if (o) {
        if (l->protoLookup.icIdentifier == o->internalClass->id)
            return l->protoLookup.data->asReturnedValue();
    }
    return getterTwoClasses(l, engine, object);
}

ReturnedValue Lookup::getter0Inlinegetter0Inline(Lookup *l, ExecutionEngine *engine, const Value &object)
{
    // we can safely cast to a QV4::Object here. If object is actually a string,
    // the internal class won't match
    Heap::Object *o = static_cast<Heap::Object *>(object.heapObject());
    if (o) {
        if (l->objectLookupTwoClasses.ic == o->internalClass)
            return o->inlinePropertyDataWithOffset(l->objectLookupTwoClasses.offset)->asReturnedValue();
        if (l->objectLookupTwoClasses.ic2 == o->internalClass)
            return o->inlinePropertyDataWithOffset(l->objectLookupTwoClasses.offset2)->asReturnedValue();
    }
    l->getter = getterFallback;
    return getterFallback(l, engine, object);
}

ReturnedValue Lookup::getter0Inlinegetter0MemberData(Lookup *l, ExecutionEngine *engine, const Value &object)
{
    // we can safely cast to a QV4::Object here. If object is actually a string,
    // the internal class won't match
    Heap::Object *o = static_cast<Heap::Object *>(object.heapObject());
    if (o) {
        if (l->objectLookupTwoClasses.ic == o->internalClass)
            return o->inlinePropertyDataWithOffset(l->objectLookupTwoClasses.offset)->asReturnedValue();
        if (l->objectLookupTwoClasses.ic2 == o->internalClass)
            return o->memberData->values.data()[l->objectLookupTwoClasses.offset2].asReturnedValue();
    }
    l->getter = getterFallback;
    return getterFallback(l, engine, object);
}

ReturnedValue Lookup::getter0MemberDatagetter0MemberData(Lookup *l, ExecutionEngine *engine, const Value &object)
{
    // we can safely cast to a QV4::Object here. If object is actually a string,
    // the internal class won't match
    Heap::Object *o = static_cast<Heap::Object *>(object.heapObject());
    if (o) {
        if (l->objectLookupTwoClasses.ic == o->internalClass)
            return o->memberData->values.data()[l->objectLookupTwoClasses.offset].asReturnedValue();
        if (l->objectLookupTwoClasses.ic2 == o->internalClass)
            return o->memberData->values.data()[l->objectLookupTwoClasses.offset2].asReturnedValue();
    }
    l->getter = getterFallback;
    return getterFallback(l, engine, object);
}

ReturnedValue Lookup::getterProtoTwoClasses(Lookup *l, ExecutionEngine *engine, const Value &object)
{
    // we can safely cast to a QV4::Object here. If object is actually a string,
    // the internal class won't match
    Heap::Object *o = static_cast<Heap::Object *>(object.heapObject());
    if (o) {
        if (l->protoLookupTwoClasses.icIdentifier == o->internalClass->id)
            return l->protoLookupTwoClasses.data->asReturnedValue();
        if (l->protoLookupTwoClasses.icIdentifier2 == o->internalClass->id)
            return l->protoLookupTwoClasses.data2->asReturnedValue();
        return getterFallback(l, engine, object);
    }
    l->getter = getterFallback;
    return getterFallback(l, engine, object);
}

ReturnedValue Lookup::getterAccessor(Lookup *l, ExecutionEngine *engine, const Value &object)
{
    // we can safely cast to a QV4::Object here. If object is actually a string,
    // the internal class won't match
    Heap::Object *o = static_cast<Heap::Object *>(object.heapObject());
    if (o) {
        if (l->objectLookup.ic == o->internalClass) {
            const Value *getter = o->propertyData(l->objectLookup.offset);
            if (!getter->isFunctionObject()) // ### catch at resolve time
                return Encode::undefined();

            return static_cast<const FunctionObject *>(getter)->call(&object, nullptr, 0);
        }
    }
    l->getter = getterFallback;
    return getterFallback(l, engine, object);
}

ReturnedValue Lookup::getterProtoAccessor(Lookup *l, ExecutionEngine *engine, const Value &object)
{
    // we can safely cast to a QV4::Object here. If object is actually a string,
    // the internal class won't match
    Heap::Object *o = static_cast<Heap::Object *>(object.heapObject());
    if (o && l->protoLookup.icIdentifier == o->internalClass->id) {
        const Value *getter = l->protoLookup.data;
        if (!getter->isFunctionObject()) // ### catch at resolve time
            return Encode::undefined();

        return static_cast<const FunctionObject *>(getter)->call(&object, nullptr, 0);
    }
    l->getter = getterTwoClasses;
    return getterTwoClasses(l, engine, object);
}

ReturnedValue Lookup::getterProtoAccessorTwoClasses(Lookup *l, ExecutionEngine *engine, const Value &object)
{
    // we can safely cast to a QV4::Object here. If object is actually a string,
    // the internal class won't match
    Heap::Object *o = static_cast<Heap::Object *>(object.heapObject());
    if (o) {
        const Value *getter = nullptr;
        if (l->protoLookupTwoClasses.icIdentifier == o->internalClass->id)
            getter = l->protoLookupTwoClasses.data;
        else if (l->protoLookupTwoClasses.icIdentifier2 == o->internalClass->id)
            getter = l->protoLookupTwoClasses.data2;
        if (getter) {
            if (!getter->isFunctionObject()) // ### catch at resolve time
                return Encode::undefined();

            return static_cast<const FunctionObject *>(getter)->call(&object, nullptr, 0);
        }
    }
    l->getter = getterFallback;
    return getterFallback(l, engine, object);
}

ReturnedValue Lookup::primitiveGetterProto(Lookup *l, ExecutionEngine *engine, const Value &object)
{
    if (object.type() == l->primitiveLookup.type) {
        Heap::Object *o = l->primitiveLookup.proto;
        if (l->primitiveLookup.icIdentifier == o->internalClass->id)
            return l->primitiveLookup.data->asReturnedValue();
    }
    l->getter = getterGeneric;
    return getterGeneric(l, engine, object);
}

ReturnedValue Lookup::primitiveGetterAccessor(Lookup *l, ExecutionEngine *engine, const Value &object)
{
    if (object.type() == l->primitiveLookup.type) {
        Heap::Object *o = l->primitiveLookup.proto;
        if (l->primitiveLookup.icIdentifier == o->internalClass->id) {
            const Value *getter = l->primitiveLookup.data;
            if (!getter->isFunctionObject()) // ### catch at resolve time
                return Encode::undefined();

            return static_cast<const FunctionObject *>(getter)->call(&object, nullptr, 0);
        }
    }
    l->getter = getterGeneric;
    return getterGeneric(l, engine, object);
}

ReturnedValue Lookup::stringLengthGetter(Lookup *l, ExecutionEngine *engine, const Value &object)
{
    if (const String *s = object.as<String>())
        return Encode(s->d()->length());

    l->getter = getterGeneric;
    return getterGeneric(l, engine, object);
}

ReturnedValue Lookup::globalGetterGeneric(Lookup *l, ExecutionEngine *engine)
{
    Object *o = engine->globalObject;
    PropertyAttributes attrs;
    ReturnedValue v = l->lookup(o, &attrs);
    if (v != Primitive::emptyValue().asReturnedValue()) {
        if (attrs.isData()) {
            if (l->level == 0) {
                uint nInline = o->d()->vtable()->nInlineProperties;
                if (l->index < nInline) {
                    l->index += o->d()->vtable()->inlinePropertyOffset;
                    l->globalGetter = globalGetter0Inline;
                } else {
                    l->index -= nInline;
                    l->globalGetter = globalGetter0MemberData;
                }
            } else if (l->level == 1)
                l->globalGetter = globalGetter1;
            else if (l->level == 2)
                l->globalGetter = globalGetter2;
            return v;
        } else {
            if (l->level == 0)
                l->globalGetter = globalGetterAccessor0;
            else if (l->level == 1)
                l->globalGetter = globalGetterAccessor1;
            else if (l->level == 2)
                l->globalGetter = globalGetterAccessor2;
            return v;
        }
    }
    Scope scope(engine);
    ScopedString n(scope, engine->currentStackFrame->v4Function->compilationUnit->runtimeStrings[l->nameIndex]);
    return engine->throwReferenceError(n);
}

ReturnedValue Lookup::globalGetter0Inline(Lookup *l, ExecutionEngine *engine)
{
    Object *o = engine->globalObject;
    if (l->classList[0] == o->internalClass())
        return o->d()->inlinePropertyDataWithOffset(l->index)->asReturnedValue();

    l->globalGetter = globalGetterGeneric;
    return globalGetterGeneric(l, engine);
}

ReturnedValue Lookup::globalGetter0MemberData(Lookup *l, ExecutionEngine *engine)
{
    Object *o = engine->globalObject;
    if (l->classList[0] == o->internalClass())
        return o->d()->memberData->values.data()[l->index].asReturnedValue();

    l->globalGetter = globalGetterGeneric;
    return globalGetterGeneric(l, engine);
}

ReturnedValue Lookup::globalGetter1(Lookup *l, ExecutionEngine *engine)
{
    Object *o = engine->globalObject;
    if (l->classList[0] == o->internalClass() &&
        l->classList[1] == o->prototype()->internalClass)
        return o->prototype()->propertyData(l->index)->asReturnedValue();

    l->globalGetter = globalGetterGeneric;
    return globalGetterGeneric(l, engine);
}

ReturnedValue Lookup::globalGetter2(Lookup *l, ExecutionEngine *engine)
{
    Heap::Object *o = engine->globalObject->d();
    if (l->classList[0] == o->internalClass) {
        o = o->prototype();
        if (l->classList[1] == o->internalClass) {
            o = o->prototype();
            if (l->classList[2] == o->internalClass) {
                return o->prototype()->propertyData(l->index)->asReturnedValue();
            }
        }
    }
    l->globalGetter = globalGetterGeneric;
    return globalGetterGeneric(l, engine);
}

ReturnedValue Lookup::globalGetterAccessor0(Lookup *l, ExecutionEngine *engine)
{
    Object *o = engine->globalObject;
    if (l->classList[0] == o->internalClass()) {
        Scope scope(o->engine());
        ScopedFunctionObject getter(scope, o->propertyData(l->index + Object::GetterOffset));
        if (!getter)
            return Encode::undefined();

        JSCallData jsCallData(scope);
        return getter->call(jsCallData);
    }
    l->globalGetter = globalGetterGeneric;
    return globalGetterGeneric(l, engine);
}

ReturnedValue Lookup::globalGetterAccessor1(Lookup *l, ExecutionEngine *engine)
{
    Object *o = engine->globalObject;
    if (l->classList[0] == o->internalClass() &&
        l->classList[1] == o->prototype()->internalClass) {
        Scope scope(o->engine());
        ScopedFunctionObject getter(scope, o->prototype()->propertyData(l->index + Object::GetterOffset));
        if (!getter)
            return Encode::undefined();

        JSCallData jsCallData(scope);
        return getter->call(jsCallData);
    }
    l->globalGetter = globalGetterGeneric;
    return globalGetterGeneric(l, engine);
}

ReturnedValue Lookup::globalGetterAccessor2(Lookup *l, ExecutionEngine *engine)
{
    Heap::Object *o = engine->globalObject->d();
    if (l->classList[0] == o->internalClass) {
        o = o->prototype();
        if (l->classList[1] == o->internalClass) {
            o = o->prototype();
            if (l->classList[2] == o->internalClass) {
                Scope scope(o->internalClass->engine);
                ScopedFunctionObject getter(scope, o->propertyData(l->index + Object::GetterOffset));
                if (!getter)
                    return Encode::undefined();

                JSCallData jsCallData(scope);
                return getter->call(jsCallData);
            }
        }
    }
    l->globalGetter = globalGetterGeneric;
    return globalGetterGeneric(l, engine);
}

bool Lookup::setterGeneric(Lookup *l, ExecutionEngine *engine, Value &object, const Value &value)
{
    Scope scope(engine);
    ScopedObject o(scope, object);
    if (!o) {
        o = RuntimeHelpers::convertToObject(scope.engine, object);
        if (!o) // type error
            return false;
        ScopedString name(scope, engine->currentStackFrame->v4Function->compilationUnit->runtimeStrings[l->nameIndex]);
        return o->put(name, value);
    }
    return o->setLookup(l, value);
}

bool Lookup::setterTwoClasses(Lookup *l, ExecutionEngine *engine, Value &object, const Value &value)
{
    Lookup l1 = *l;

    if (Object *o = object.as<Object>()) {
        if (!o->setLookup(l, value))
            return false;

        if (l->setter == Lookup::setter0 || l->setter == Lookup::setter0Inline) {
            l->setter = setter0setter0;
            l->classList[1] = l1.classList[0];
            l->index2 = l1.index;
            return true;
        }
    }

    l->setter = setterFallback;
    return setterFallback(l, engine, object, value);
}

bool Lookup::setterFallback(Lookup *l, ExecutionEngine *engine, Value &object, const Value &value)
{
    QV4::Scope scope(engine);
    QV4::ScopedObject o(scope, object.toObject(scope.engine));
    if (!o)
        return false;

    ScopedString name(scope, engine->currentStackFrame->v4Function->compilationUnit->runtimeStrings[l->nameIndex]);
    return o->put(name, value);
}

bool Lookup::setter0(Lookup *l, ExecutionEngine *engine, Value &object, const Value &value)
{
    Object *o = static_cast<Object *>(object.managed());
    if (o && o->internalClass() == l->classList[0]) {
        o->setProperty(engine, l->index, value);
        return true;
    }

    return setterTwoClasses(l, engine, object, value);
}

bool Lookup::setter0Inline(Lookup *l, ExecutionEngine *engine, Value &object, const Value &value)
{
    Object *o = static_cast<Object *>(object.managed());
    if (o && o->internalClass() == l->classList[0]) {
        o->d()->setInlineProperty(engine, l->index, value);
        return true;
    }

    return setterTwoClasses(l, engine, object, value);
}

bool Lookup::setterInsert0(Lookup *l, ExecutionEngine *engine, Value &object, const Value &value)
{
    Object *o = static_cast<Object *>(object.managed());
    if (o && o->internalClass() == l->classList[0]) {
        Q_ASSERT(!o->prototype());
        o->setInternalClass(l->classList[3]);
        o->setProperty(l->index, value);
        return true;
    }

    l->setter = setterFallback;
    return setterFallback(l, engine, object, value);
}

bool Lookup::setterInsert1(Lookup *l, ExecutionEngine *engine, Value &object, const Value &value)
{
    Object *o = static_cast<Object *>(object.managed());
    if (o && o->internalClass() == l->classList[0]) {
        Heap::Object *p = o->prototype();
        Q_ASSERT(p);
        if (p->internalClass == l->classList[1]) {
            Q_ASSERT(!p->prototype());
            o->setInternalClass(l->classList[3]);
            o->setProperty(l->index, value);
            return true;
        }
    }

    l->setter = setterFallback;
    return setterFallback(l, engine, object, value);
}

bool Lookup::setterInsert2(Lookup *l, ExecutionEngine *engine, Value &object, const Value &value)
{
    Object *o = static_cast<Object *>(object.managed());
    if (o && o->internalClass() == l->classList[0]) {
        Heap::Object *p = o->prototype();
        Q_ASSERT(p);
        if (p->internalClass == l->classList[1]) {
            p = p->prototype();
            Q_ASSERT(p);
            if (p->internalClass == l->classList[2]) {
                Q_ASSERT(!p->prototype());
                o->setInternalClass(l->classList[3]);
                o->setProperty(l->index, value);
                return true;
            }
        }
    }

    l->setter = setterFallback;
    return setterFallback(l, engine, object, value);
}

bool Lookup::setter0setter0(Lookup *l, ExecutionEngine *engine, Value &object, const Value &value)
{
    Object *o = static_cast<Object *>(object.managed());
    if (o) {
        if (o->internalClass() == l->classList[0]) {
            o->setProperty(l->index, value);
            return true;
        }
        if (o->internalClass() == l->classList[1]) {
            o->setProperty(l->index2, value);
            return true;
        }
    }

    l->setter = setterFallback;
    return setterFallback(l, engine, object, value);
}

QT_END_NAMESPACE
