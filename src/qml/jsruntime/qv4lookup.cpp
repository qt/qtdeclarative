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

ReturnedValue Lookup::getterGeneric(Lookup *l, ExecutionEngine *engine, const Value &object)
{
    if (const Object *o = object.as<Object>())
        return o->getLookup(l);

    Object *proto;
    switch (object.type()) {
    case Value::Undefined_Type:
    case Value::Null_Type:
        return engine->throwTypeError();
    case Value::Boolean_Type:
        proto = engine->booleanPrototype();
        break;
    case Value::Managed_Type: {
        Q_ASSERT(object.isString());
        proto = engine->stringPrototype();
        Scope scope(engine);
        ScopedString name(scope, engine->currentStackFrame->v4Function->compilationUnit->runtimeStrings[l->nameIndex]);
        if (name->equals(engine->id_length())) {
            // special case, as the property is on the object itself
            l->getter = stringLengthGetter;
            return stringLengthGetter(l, engine, object);
        }
        break;
    }
    case Value::Integer_Type:
    default: // Number
        proto = engine->numberPrototype();
    }

    PropertyAttributes attrs;
    ReturnedValue v = l->lookup(object, proto, &attrs);
    if (v != Primitive::emptyValue().asReturnedValue()) {
        l->type = object.type();
        l->proto = proto->d();
        if (attrs.isData()) {
            if (l->level == 0) {
                uint nInline = l->proto->vtable()->nInlineProperties;
                if (l->index < nInline)
                    l->getter = Lookup::primitiveGetter0Inline;
                else {
                    l->index -= nInline;
                    l->getter = Lookup::primitiveGetter0MemberData;
                }
            } else if (l->level == 1)
                l->getter = Lookup::primitiveGetter1;
            return v;
        } else {
            if (l->level == 0)
                l->getter = Lookup::primitiveGetterAccessor0;
            else if (l->level == 1)
                l->getter = Lookup::primitiveGetterAccessor1;
            return v;
        }
    }

    return Encode::undefined();
}

ReturnedValue Lookup::getterTwoClasses(Lookup *l, ExecutionEngine *engine, const Value &object)
{
    Lookup l1 = *l;

    if (l1.getter == Lookup::getter0MemberData || l1.getter == Lookup::getter0Inline || l1.getter == Lookup::getter1) {
        if (const Object *o = object.as<Object>()) {
            ReturnedValue v = o->getLookup(l);
            Lookup l2 = *l;

            if (l2.index != UINT_MAX) {
                if (l1.getter != Lookup::getter0Inline) {
                    if (l2.getter == Lookup::getter0Inline ||
                        (l1.getter != Lookup::getter0MemberData && l2.getter == Lookup::getter0MemberData))
                        // sort the better getter first
                        qSwap(l1, l2);
                }

                l->classList[0] = l1.classList[0];
                l->classList[1] = l1.classList[1];
                l->classList[2] = l2.classList[0];
                l->classList[3] = l2.classList[1];
                l->index = l1.index;
                l->index2 = l2.index;

                if (l1.getter == Lookup::getter0Inline) {
                    if (l2.getter == Lookup::getter0Inline)
                        l->getter = Lookup::getter0Inlinegetter0Inline;
                    else if (l2.getter == Lookup::getter0MemberData)
                        l->getter = Lookup::getter0Inlinegetter0MemberData;
                    else if (l2.getter == Lookup::getter1)
                        l->getter = Lookup::getter0Inlinegetter1;
                    else
                        Q_UNREACHABLE();
                } else if (l1.getter == Lookup::getter0MemberData) {
                    if (l2.getter == Lookup::getter0MemberData)
                        l->getter = Lookup::getter0MemberDatagetter0MemberData;
                    else if (l2.getter == Lookup::getter1)
                        l->getter = Lookup::getter0MemberDatagetter1;
                    else
                        Q_UNREACHABLE();
                } else {
                    Q_ASSERT(l1.getter == Lookup::getter1 && l2.getter == Lookup::getter1);
                    l->getter = Lookup::getter1getter1;
                }
                return v;
            }
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
        if (l->classList[0] == o->internalClass)
            return o->memberData->values.data()[l->index].asReturnedValue();
    }
    return getterTwoClasses(l, engine, object);
}

ReturnedValue Lookup::getter0Inline(Lookup *l, ExecutionEngine *engine, const Value &object)
{
    // we can safely cast to a QV4::Object here. If object is actually a string,
    // the internal class won't match
    Heap::Object *o = static_cast<Heap::Object *>(object.heapObject());
    if (o) {
        if (l->classList[0] == o->internalClass)
            return o->inlinePropertyData(l->index)->asReturnedValue();
    }
    return getterTwoClasses(l, engine, object);
}

ReturnedValue Lookup::getter1(Lookup *l, ExecutionEngine *engine, const Value &object)
{
    // we can safely cast to a QV4::Object here. If object is actually a string,
    // the internal class won't match
    Heap::Object *o = static_cast<Heap::Object *>(object.heapObject());
    if (o) {
        if (l->classList[0] == o->internalClass && l->classList[1] == l->proto->internalClass)
            return l->proto->propertyData(l->index)->asReturnedValue();
    }
    return getterTwoClasses(l, engine, object);
}

ReturnedValue Lookup::getter2(Lookup *l, ExecutionEngine *engine, const Value &object)
{
    // we can safely cast to a QV4::Object here. If object is actually a string,
    // the internal class won't match
    Heap::Object *o = static_cast<Heap::Object *>(object.heapObject());
    if (o) {
        if (l->classList[0] == o->internalClass) {
            Q_ASSERT(l->proto == o->prototype());
            if (l->classList[1] == l->proto->internalClass) {
                Heap::Object *p = l->proto->prototype();
                if (l->classList[2] == p->internalClass)
                    return p->propertyData(l->index)->asReturnedValue();
            }
        }
    }
    l->getter = getterFallback;
    return getterFallback(l, engine, object);
}

ReturnedValue Lookup::getter0Inlinegetter0Inline(Lookup *l, ExecutionEngine *engine, const Value &object)
{
    // we can safely cast to a QV4::Object here. If object is actually a string,
    // the internal class won't match
    Heap::Object *o = static_cast<Heap::Object *>(object.heapObject());
    if (o) {
        if (l->classList[0] == o->internalClass)
            return o->inlinePropertyData(l->index)->asReturnedValue();
        if (l->classList[2] == o->internalClass)
            return o->inlinePropertyData(l->index2)->asReturnedValue();
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
        if (l->classList[0] == o->internalClass)
            return o->inlinePropertyData(l->index)->asReturnedValue();
        if (l->classList[2] == o->internalClass)
            return o->memberData->values.data()[l->index2].asReturnedValue();
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
        if (l->classList[0] == o->internalClass)
            return o->memberData->values.data()[l->index].asReturnedValue();
        if (l->classList[2] == o->internalClass)
            return o->memberData->values.data()[l->index2].asReturnedValue();
    }
    l->getter = getterFallback;
    return getterFallback(l, engine, object);
}

ReturnedValue Lookup::getter0Inlinegetter1(Lookup *l, ExecutionEngine *engine, const Value &object)
{
    // we can safely cast to a QV4::Object here. If object is actually a string,
    // the internal class won't match
    Heap::Object *o = static_cast<Heap::Object *>(object.heapObject());
    if (o) {
        if (l->classList[0] == o->internalClass)
            return o->inlinePropertyData(l->index)->asReturnedValue();
        if (l->classList[2] == o->internalClass && l->classList[3] == o->prototype()->internalClass)
            return o->prototype()->propertyData(l->index2)->asReturnedValue();
    }
    l->getter = getterFallback;
    return getterFallback(l, engine, object);
}

ReturnedValue Lookup::getter0MemberDatagetter1(Lookup *l, ExecutionEngine *engine, const Value &object)
{
    // we can safely cast to a QV4::Object here. If object is actually a string,
    // the internal class won't match
    Heap::Object *o = static_cast<Heap::Object *>(object.heapObject());
    if (o) {
        if (l->classList[0] == o->internalClass)
            return o->memberData->values.data()[l->index].asReturnedValue();
        if (l->classList[2] == o->internalClass && l->classList[3] == o->prototype()->internalClass)
            return o->prototype()->propertyData(l->index2)->asReturnedValue();
    }
    l->getter = getterFallback;
    return getterFallback(l, engine, object);
}

ReturnedValue Lookup::getter1getter1(Lookup *l, ExecutionEngine *engine, const Value &object)
{
    // we can safely cast to a QV4::Object here. If object is actually a string,
    // the internal class won't match
    Heap::Object *o = static_cast<Heap::Object *>(object.heapObject());
    if (o) {
        if (l->classList[0] == o->internalClass &&
            l->classList[1] == o->prototype()->internalClass)
            return o->prototype()->propertyData(l->index)->asReturnedValue();
        if (l->classList[2] == o->internalClass &&
            l->classList[3] == o->prototype()->internalClass)
            return o->prototype()->propertyData(l->index2)->asReturnedValue();
        return getterFallback(l, engine, object);
    }
    l->getter = getterFallback;
    return getterFallback(l, engine, object);
}


ReturnedValue Lookup::getterAccessor0(Lookup *l, ExecutionEngine *engine, const Value &object)
{
    // we can safely cast to a QV4::Object here. If object is actually a string,
    // the internal class won't match
    Heap::Object *o = static_cast<Heap::Object *>(object.heapObject());
    if (o) {
        if (l->classList[0] == o->internalClass) {
            Scope scope(o->internalClass->engine);
            ScopedFunctionObject getter(scope, o->propertyData(l->index + Object::GetterOffset));
            if (!getter)
                return Encode::undefined();

            JSCall jsCall(scope, getter, 0);
            jsCall->thisObject = object;
            return jsCall.call();
        }
    }
    l->getter = getterFallback;
    return getterFallback(l, engine, object);
}

ReturnedValue Lookup::getterAccessor1(Lookup *l, ExecutionEngine *engine, const Value &object)
{
    // we can safely cast to a QV4::Object here. If object is actually a string,
    // the internal class won't match
    Heap::Object *o = static_cast<Heap::Object *>(object.heapObject());
    if (o) {
        if (l->classList[0] == o->internalClass &&
            l->classList[1] == l->proto->internalClass) {
            Scope scope(o->internalClass->engine);
            ScopedFunctionObject getter(scope, o->prototype()->propertyData(l->index + Object::GetterOffset));
            if (!getter)
                return Encode::undefined();

            JSCall jsCall(scope, getter, 0);
            jsCall->thisObject = object;
            return jsCall.call();
        }
    }
    l->getter = getterFallback;
    return getterFallback(l, engine, object);
}

ReturnedValue Lookup::getterAccessor2(Lookup *l, ExecutionEngine *engine, const Value &object)
{
    // we can safely cast to a QV4::Object here. If object is actually a string,
    // the internal class won't match
    Heap::Object *o = static_cast<Heap::Object *>(object.heapObject());
    if (o) {
        if (l->classList[0] == o->internalClass) {
            Q_ASSERT(o->prototype() == l->proto);
            if (l->classList[1] == l->proto->internalClass) {
                o = l->proto->prototype();
                if (l->classList[2] == o->internalClass) {
                    Scope scope(o->internalClass->engine);
                    ScopedFunctionObject getter(scope, o->propertyData(l->index + Object::GetterOffset));
                    if (!getter)
                        return Encode::undefined();

                    JSCall jsCall(scope, getter, 0);
                    jsCall->thisObject = object;
                    return jsCall.call();
                }
            }
        }
    }
    l->getter = getterFallback;
    return getterFallback(l, engine, object);
}

ReturnedValue Lookup::primitiveGetter0Inline(Lookup *l, ExecutionEngine *engine, const Value &object)
{
    if (object.type() == l->type) {
        Heap::Object *o = l->proto;
        if (l->classList[0] == o->internalClass)
            return o->inlinePropertyData(l->index)->asReturnedValue();
    }
    l->getter = getterGeneric;
    return getterGeneric(l, engine, object);
}

ReturnedValue Lookup::primitiveGetter0MemberData(Lookup *l, ExecutionEngine *engine, const Value &object)
{
    if (object.type() == l->type) {
        Heap::Object *o = l->proto;
        if (l->classList[0] == o->internalClass)
            return o->memberData->values.data()[l->index].asReturnedValue();
    }
    l->getter = getterGeneric;
    return getterGeneric(l, engine, object);
}

ReturnedValue Lookup::primitiveGetter1(Lookup *l, ExecutionEngine *engine, const Value &object)
{
    if (object.type() == l->type) {
        Heap::Object *o = l->proto;
        if (l->classList[0] == o->internalClass &&
            l->classList[1] == o->prototype()->internalClass)
            return o->prototype()->propertyData(l->index)->asReturnedValue();
    }
    l->getter = getterGeneric;
    return getterGeneric(l, engine, object);
}

ReturnedValue Lookup::primitiveGetterAccessor0(Lookup *l, ExecutionEngine *engine, const Value &object)
{
    if (object.type() == l->type) {
        Heap::Object *o = l->proto;
        if (l->classList[0] == o->internalClass) {
            Scope scope(o->internalClass->engine);
            ScopedFunctionObject getter(scope, o->propertyData(l->index + Object::GetterOffset));
            if (!getter)
                return Encode::undefined();

            JSCall jsCall(scope, getter, 0);
            jsCall->thisObject = object;
            return jsCall.call();
        }
    }
    l->getter = getterGeneric;
    return getterGeneric(l, engine, object);
}

ReturnedValue Lookup::primitiveGetterAccessor1(Lookup *l, ExecutionEngine *engine, const Value &object)
{
    if (object.type() == l->type) {
        Heap::Object *o = l->proto;
        if (l->classList[0] == o->internalClass &&
            l->classList[1] == o->prototype()->internalClass) {
            Scope scope(o->internalClass->engine);
            ScopedFunctionObject getter(scope, o->prototype()->propertyData(l->index + Object::GetterOffset));
            if (!getter)
                return Encode::undefined();

            JSCall jsCall(scope, getter, 0);
            jsCall->thisObject = object;
            return jsCall.call();
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

ReturnedValue Lookup::arrayLengthGetter(Lookup *l, ExecutionEngine *engine, const Value &object)
{
    if (const ArrayObject *a = object.as<ArrayObject>())
        return a->propertyData(Heap::ArrayObject::LengthPropertyIndex)->asReturnedValue();

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
                if (l->index < nInline)
                    l->globalGetter = globalGetter0Inline;
                else {
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
        return o->d()->inlinePropertyData(l->index)->asReturnedValue();

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

        JSCall jsCall(scope, getter, 0);
        return jsCall.call();
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

        JSCall jsCall(scope, getter, 0);
        return jsCall.call();
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

                JSCall jsCall(scope, getter, 0);
                return jsCall.call();
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
