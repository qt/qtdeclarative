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

#include "qv4object_p.h"
#include "qv4jsir_p.h"
#include "qv4isel_p.h"
#include "qv4objectproto_p.h"
#include "qv4stringobject_p.h"
#include "qv4argumentsobject_p.h"
#include "qv4mm_p.h"
#include "qv4lookup_p.h"
#include "qv4scopedvalue_p.h"
#include "qv4memberdata_p.h"

#include <private/qqmljsengine_p.h>
#include <private/qqmljslexer_p.h>
#include <private/qqmljsparser_p.h>
#include <private/qqmljsast_p.h>
#include <qv4jsir_p.h>
#include <qv4codegen_p.h>
#include "private/qlocale_tools_p.h"

#include <QtCore/qmath.h>
#include <QtCore/QDebug>
#include <cassert>
#include <typeinfo>
#include <iostream>
#include <stdint.h>
#include "qv4alloca_p.h"

using namespace QV4;

DEFINE_OBJECT_VTABLE(Object);

Object::Object(ExecutionEngine *engine)
    : Managed(engine->objectClass)
{
}

Object::Object(InternalClass *ic)
    : Managed(ic)
{
    Q_ASSERT(internalClass->vtable && internalClass->vtable != &Managed::static_vtbl);

    Q_ASSERT(!memberData.d());
    if (internalClass->size) {
        Scope scope(engine());
        ScopedObject protectThis(scope, this);
        memberData.ensureIndex(engine(), internalClass->size);
    }
}

Object::~Object()
{
    _data = 0;
}

bool Object::setPrototype(Object *proto)
{
    Object *pp = proto;
    while (pp) {
        if (pp == this)
            return false;
        pp = pp->prototype();
    }
    internalClass = internalClass->changePrototype(proto);
    return true;
}

void Object::destroy(Managed *that)
{
    static_cast<Object *>(that)->~Object();
}

void Object::put(ExecutionContext *ctx, const QString &name, const ValueRef value)
{
    Scope scope(ctx);
    ScopedString n(scope, ctx->engine->newString(name));
    put(n, value);
}

ReturnedValue Object::getValue(const ValueRef thisObject, const Property *p, PropertyAttributes attrs)
{
    if (!attrs.isAccessor())
        return p->value.asReturnedValue();
    FunctionObject *getter = p->getter();
    if (!getter)
        return Encode::undefined();

    Scope scope(getter->engine());
    ScopedCallData callData(scope, 0);
    callData->thisObject = *thisObject;
    return getter->call(callData);
}

void Object::putValue(Property *pd, PropertyAttributes attrs, const ValueRef value)
{
    if (internalClass->engine->hasException)
        return;

    if (attrs.isAccessor()) {
        if (FunctionObject *set = pd->setter()) {
            Scope scope(set->engine());
            ScopedCallData callData(scope, 1);
            callData->args[0] = *value;
            callData->thisObject = this;
            set->call(callData);
            return;
        }
        goto reject;
    }

    if (!attrs.isWritable())
        goto reject;

    pd->value = *value;
    return;

  reject:
    if (engine()->currentContext()->strictMode)
        engine()->currentContext()->throwTypeError();
}

void Object::defineDefaultProperty(const QString &name, ValueRef value)
{
    ExecutionEngine *e = engine();
    Scope scope(e);
    ScopedString s(scope, e->newIdentifier(name));
    defineDefaultProperty(s, value);
}

void Object::defineDefaultProperty(const QString &name, ReturnedValue (*code)(CallContext *), int argumentCount)
{
    ExecutionEngine *e = engine();
    Scope scope(e);
    ScopedString s(scope, e->newIdentifier(name));
    Scoped<FunctionObject> function(scope, e->newBuiltinFunction(e->rootContext, s, code));
    function->defineReadonlyProperty(e->id_length, Primitive::fromInt32(argumentCount));
    defineDefaultProperty(s, function);
}

void Object::defineDefaultProperty(const StringRef name, ReturnedValue (*code)(CallContext *), int argumentCount)
{
    ExecutionEngine *e = engine();
    Scope scope(e);
    Scoped<FunctionObject> function(scope, e->newBuiltinFunction(e->rootContext, name, code));
    function->defineReadonlyProperty(e->id_length, Primitive::fromInt32(argumentCount));
    defineDefaultProperty(name, function);
}

void Object::defineAccessorProperty(const QString &name, ReturnedValue (*getter)(CallContext *), ReturnedValue (*setter)(CallContext *))
{
    ExecutionEngine *e = engine();
    Scope scope(e);
    Scoped<String> s(scope, e->newIdentifier(name));
    defineAccessorProperty(s, getter, setter);
}

void Object::defineAccessorProperty(const StringRef name, ReturnedValue (*getter)(CallContext *), ReturnedValue (*setter)(CallContext *))
{
    ExecutionEngine *v4 = engine();
    QV4::Scope scope(v4);
    ScopedProperty p(scope);
    p->setGetter(getter ? v4->newBuiltinFunction(v4->rootContext, name, getter)->getPointer() : 0);
    p->setSetter(setter ? v4->newBuiltinFunction(v4->rootContext, name, setter)->getPointer() : 0);
    insertMember(name, p, QV4::Attr_Accessor|QV4::Attr_NotConfigurable|QV4::Attr_NotEnumerable);
}

void Object::defineReadonlyProperty(const QString &name, ValueRef value)
{
    QV4::ExecutionEngine *e = engine();
    Scope scope(e);
    ScopedString s(scope, e->newIdentifier(name));
    defineReadonlyProperty(s, value);
}

void Object::defineReadonlyProperty(const StringRef name, ValueRef value)
{
    insertMember(name, value, Attr_ReadOnly);
}

void Object::markObjects(Managed *that, ExecutionEngine *e)
{
    Object *o = static_cast<Object *>(that);

    o->memberData.mark(e);
    if (o->arrayData)
        o->arrayData->mark(e);
}

void Object::ensureMemberIndex(uint idx)
{
    memberData.ensureIndex(engine(), idx);
}

void Object::insertMember(const StringRef s, const Property &p, PropertyAttributes attributes)
{
    uint idx;
    InternalClass::addMember(this, s.getPointer(), attributes, &idx);


    ensureMemberIndex(internalClass->size);

    if (attributes.isAccessor()) {
        hasAccessorProperty = 1;
        Property *pp = propertyAt(idx);
        pp->value = p.value;
        pp->set = p.set;
    } else {
        memberData[idx] = p.value;
    }
}

// Section 8.12.1
Property *Object::__getOwnProperty__(const StringRef name, PropertyAttributes *attrs)
{
    uint idx = name->asArrayIndex();
    if (idx != UINT_MAX)
        return __getOwnProperty__(idx, attrs);

    uint member = internalClass->find(name);
    if (member < UINT_MAX) {
        if (attrs)
            *attrs = internalClass->propertyData[member];
        return propertyAt(member);
    }

    if (attrs)
        *attrs = Attr_Invalid;
    return 0;
}

Property *Object::__getOwnProperty__(uint index, PropertyAttributes *attrs)
{
    Property *p = arrayData->getProperty(index);
    if (p) {
        if (attrs)
            *attrs = arrayData->attributes(index);
        return p;
    }
    if (isStringObject()) {
        if (attrs)
            *attrs = Attr_NotConfigurable|Attr_NotWritable;
        return static_cast<StringObject *>(this)->getIndex(index);
    }

    if (attrs)
        *attrs = Attr_Invalid;
    return 0;
}

// Section 8.12.2
Property *Object::__getPropertyDescriptor__(const StringRef name, PropertyAttributes *attrs) const
{
    uint idx = name->asArrayIndex();
    if (idx != UINT_MAX)
        return __getPropertyDescriptor__(idx, attrs);


    const Object *o = this;
    while (o) {
        uint idx = o->internalClass->find(name.getPointer());
        if (idx < UINT_MAX) {
            if (attrs)
                *attrs = o->internalClass->propertyData[idx];
            return o->propertyAt(idx);
        }

        o = o->prototype();
    }
    if (attrs)
        *attrs = Attr_Invalid;
    return 0;
}

Property *Object::__getPropertyDescriptor__(uint index, PropertyAttributes *attrs) const
{
    const Object *o = this;
    while (o) {
        Property *p = o->arrayData->getProperty(index);
        if (p) {
            if (attrs)
                *attrs = o->arrayData->attributes(index);
            return p;
        }
        if (o->isStringObject()) {
            Property *p = static_cast<const StringObject *>(o)->getIndex(index);
            if (p) {
                if (attrs)
                    *attrs = (Attr_NotWritable|Attr_NotConfigurable);
                return p;
            }
        }
        o = o->prototype();
    }
    if (attrs)
        *attrs = Attr_Invalid;
    return 0;
}

bool Object::hasProperty(const StringRef name) const
{
    uint idx = name->asArrayIndex();
    if (idx != UINT_MAX)
        return hasProperty(idx);

    const Object *o = this;
    while (o) {
        if (o->hasOwnProperty(name))
            return true;

        o = o->prototype();
    }

    return false;
}

bool Object::hasProperty(uint index) const
{
    const Object *o = this;
    while (o) {
        if (o->hasOwnProperty(index))
                return true;

        o = o->prototype();
    }

    return false;
}

bool Object::hasOwnProperty(const StringRef name) const
{
    uint idx = name->asArrayIndex();
    if (idx != UINT_MAX)
        return hasOwnProperty(idx);

    if (internalClass->find(name) < UINT_MAX)
        return true;
    if (!query(name).isEmpty())
        return true;
    return false;
}

bool Object::hasOwnProperty(uint index) const
{
    if (!arrayData->isEmpty(index))
        return true;
    if (isStringObject()) {
        String *s = static_cast<const StringObject *>(this)->value.asString();
        if (index < (uint)s->length())
            return true;
    }
    if (!queryIndexed(index).isEmpty())
        return true;
    return false;
}

ReturnedValue Object::construct(Managed *m, CallData *)
{
    return m->engine()->currentContext()->throwTypeError();
}

ReturnedValue Object::call(Managed *m, CallData *)
{
    return m->engine()->currentContext()->throwTypeError();
}

ReturnedValue Object::get(Managed *m, const StringRef name, bool *hasProperty)
{
    return static_cast<Object *>(m)->internalGet(name, hasProperty);
}

ReturnedValue Object::getIndexed(Managed *m, uint index, bool *hasProperty)
{
    return static_cast<Object *>(m)->internalGetIndexed(index, hasProperty);
}

void Object::put(Managed *m, const StringRef name, const ValueRef value)
{
    static_cast<Object *>(m)->internalPut(name, value);
}

void Object::putIndexed(Managed *m, uint index, const ValueRef value)
{
    static_cast<Object *>(m)->internalPutIndexed(index, value);
}

PropertyAttributes Object::query(const Managed *m, StringRef name)
{
    uint idx = name->asArrayIndex();
    if (idx != UINT_MAX)
        return queryIndexed(m, idx);

    const Object *o = static_cast<const Object *>(m);
    idx = o->internalClass->find(name.getPointer());
    if (idx < UINT_MAX)
        return o->internalClass->propertyData[idx];

    return Attr_Invalid;
}

PropertyAttributes Object::queryIndexed(const Managed *m, uint index)
{
    const Object *o = static_cast<const Object *>(m);
    if (o->arrayData->get(index) != Primitive::emptyValue().asReturnedValue())
        return o->arrayData->attributes(index);

    if (o->isStringObject()) {
        String *s = static_cast<const StringObject *>(o)->value.asString();
        if (index < (uint)s->length())
            return (Attr_NotWritable|Attr_NotConfigurable);
    }
    return Attr_Invalid;
}

bool Object::deleteProperty(Managed *m, const StringRef name)
{
    return static_cast<Object *>(m)->internalDeleteProperty(name);
}

bool Object::deleteIndexedProperty(Managed *m, uint index)
{
    return static_cast<Object *>(m)->internalDeleteIndexedProperty(index);
}

ReturnedValue Object::getLookup(Managed *m, Lookup *l)
{
    Object *o = static_cast<Object *>(m);
    PropertyAttributes attrs;
    ReturnedValue v = l->lookup(o, &attrs);
    if (v != Primitive::emptyValue().asReturnedValue()) {
        if (attrs.isData()) {
            if (l->level == 0)
                l->getter = Lookup::getter0;
            else if (l->level == 1)
                l->getter = Lookup::getter1;
            else if (l->level == 2)
                l->getter = Lookup::getter2;
            else
                l->getter = Lookup::getterFallback;
            return v;
        } else {
            if (l->level == 0)
                l->getter = Lookup::getterAccessor0;
            else if (l->level == 1)
                l->getter = Lookup::getterAccessor1;
            else if (l->level == 2)
                l->getter = Lookup::getterAccessor2;
            else
                l->getter = Lookup::getterFallback;
            return v;
        }
    }
    return Encode::undefined();
}

void Object::setLookup(Managed *m, Lookup *l, const ValueRef value)
{
    Scope scope(m->engine());
    ScopedObject o(scope, static_cast<Object *>(m));

    InternalClass *c = o->internalClass;
    uint idx = c->find(l->name);
    if (!o->isArrayObject() || idx != ArrayObject::LengthPropertyIndex) {
        if (idx != UINT_MAX && o->internalClass->propertyData[idx].isData() && o->internalClass->propertyData[idx].isWritable()) {
            l->classList[0] = o->internalClass;
            l->index = idx;
            l->setter = Lookup::setter0;
            o->memberData[idx] = *value;
            return;
        }

        if (idx != UINT_MAX) {
            o->putValue(o->propertyAt(idx), o->internalClass->propertyData[idx], value);
            return;
        }
    }

    ScopedString s(scope, l->name);
    o->put(s, value);

    if (o->internalClass == c)
        return;
    idx = o->internalClass->find(l->name);
    if (idx == UINT_MAX)
        return;
    l->classList[0] = c;
    l->classList[3] = o->internalClass;
    l->index = idx;
    if (!o->prototype()) {
        l->setter = Lookup::setterInsert0;
        return;
    }
    o = o->prototype();
    l->classList[1] = o->internalClass;
    if (!o->prototype()) {
        l->setter = Lookup::setterInsert1;
        return;
    }
    o = o->prototype();
    l->classList[2] = o->internalClass;
    if (!o->prototype()) {
        l->setter = Lookup::setterInsert2;
        return;
    }
    l->setter = Lookup::setterGeneric;
}

void Object::advanceIterator(Managed *m, ObjectIterator *it, StringRef name, uint *index, Property *pd, PropertyAttributes *attrs)
{
    Object *o = static_cast<Object *>(m);
    name = (String *)0;
    *index = UINT_MAX;

    if (o->arrayData) {
        if (!it->arrayIndex)
            it->arrayNode = o->sparseBegin();

        // sparse arrays
        if (it->arrayNode) {
            while (it->arrayNode != o->sparseEnd()) {
                int k = it->arrayNode->key();
                uint pidx = it->arrayNode->value;
                Property *p = reinterpret_cast<Property *>(o->arrayData->data + pidx);
                it->arrayNode = it->arrayNode->nextNode();
                PropertyAttributes a = o->arrayData->attributes(k);
                if (!(it->flags & ObjectIterator::EnumerableOnly) || a.isEnumerable()) {
                    it->arrayIndex = k + 1;
                    *index = k;
                    *attrs = a;
                    pd->copy(*p, a);
                    return;
                }
            }
            it->arrayNode = 0;
            it->arrayIndex = UINT_MAX;
        }
        // dense arrays
        while (it->arrayIndex < o->arrayData->length()) {
            Value *val = o->arrayData->data + it->arrayIndex;
            PropertyAttributes a = o->arrayData->attributes(it->arrayIndex);
            ++it->arrayIndex;
            if (!val->isEmpty()
                && (!(it->flags & ObjectIterator::EnumerableOnly) || a.isEnumerable())) {
                *index = it->arrayIndex - 1;
                *attrs = a;
                pd->value = *val;
                return;
            }
        }
    }

    while (it->memberIndex < o->internalClass->size) {
        String *n = o->internalClass->nameMap.at(it->memberIndex);
        if (!n) {
            // accessor properties have a dummy entry with n == 0
            ++it->memberIndex;
            continue;
        }

        Property *p = o->propertyAt(it->memberIndex);
        PropertyAttributes a = o->internalClass->propertyData[it->memberIndex];
        ++it->memberIndex;
        if (!(it->flags & ObjectIterator::EnumerableOnly) || a.isEnumerable()) {
            name = n;
            *attrs = a;
            pd->copy(*p, a);
            return;
        }
    }

    *attrs = PropertyAttributes();
}

// Section 8.12.3
ReturnedValue Object::internalGet(const StringRef name, bool *hasProperty)
{
    uint idx = name->asArrayIndex();
    if (idx != UINT_MAX)
        return getIndexed(idx, hasProperty);

    name->makeIdentifier();

    Object *o = this;
    while (o) {
        uint idx = o->internalClass->find(name.getPointer());
        if (idx < UINT_MAX) {
            if (hasProperty)
                *hasProperty = true;
            return getValue(o->propertyAt(idx), o->internalClass->propertyData.at(idx));
        }

        o = o->prototype();
    }

    if (hasProperty)
        *hasProperty = false;
    return Encode::undefined();
}

ReturnedValue Object::internalGetIndexed(uint index, bool *hasProperty)
{
    Property *pd = 0;
    PropertyAttributes attrs;
    Object *o = this;
    while (o) {
        Property *p = o->arrayData->getProperty(index);
        if (p) {
            pd = p;
            attrs = o->arrayData->attributes(index);
            break;
        }
        if (o->isStringObject()) {
            pd = static_cast<StringObject *>(o)->getIndex(index);
            if (pd) {
                attrs = (Attr_NotWritable|Attr_NotConfigurable);
                break;
            }
        }
        o = o->prototype();
    }

    if (pd) {
        if (hasProperty)
            *hasProperty = true;
        return getValue(pd, attrs);
    }

    if (hasProperty)
        *hasProperty = false;
    return Encode::undefined();
}


// Section 8.12.5
void Object::internalPut(const StringRef name, const ValueRef value)
{
    if (internalClass->engine->hasException)
        return;

    uint idx = name->asArrayIndex();
    if (idx != UINT_MAX)
        return putIndexed(idx, value);

    name->makeIdentifier();

    uint member = internalClass->find(name.getPointer());
    Property *pd = 0;
    PropertyAttributes attrs;
    if (member < UINT_MAX) {
        pd = propertyAt(member);
        attrs = internalClass->propertyData[member];
    }

    // clause 1
    if (pd) {
        if (attrs.isAccessor()) {
            if (pd->setter())
                goto cont;
            goto reject;
        } else if (!attrs.isWritable())
            goto reject;
        else if (isArrayObject() && name->equals(engine()->id_length)) {
            bool ok;
            uint l = value->asArrayLength(&ok);
            if (!ok) {
                engine()->currentContext()->throwRangeError(value);
                return;
            }
            ok = setArrayLength(l);
            if (!ok)
                goto reject;
        } else {
            pd->value = *value;
        }
        return;
    } else if (!prototype()) {
        if (!extensible)
            goto reject;
    } else {
        // clause 4
        if ((pd = prototype()->__getPropertyDescriptor__(name, &attrs))) {
            if (attrs.isAccessor()) {
                if (!pd->setter())
                    goto reject;
            } else if (!extensible || !attrs.isWritable()) {
                goto reject;
            }
        } else if (!extensible) {
            goto reject;
        }
    }

    cont:

    // Clause 5
    if (pd && attrs.isAccessor()) {
        assert(pd->setter() != 0);

        Scope scope(engine());
        ScopedCallData callData(scope, 1);
        callData->args[0] = *value;
        callData->thisObject = this;
        pd->setter()->call(callData);
        return;
    }

    insertMember(name, value);
    return;

  reject:
    if (engine()->currentContext()->strictMode) {
        QString message = QStringLiteral("Cannot assign to read-only property \"");
        message += name->toQString();
        message += QLatin1Char('\"');
        engine()->currentContext()->throwTypeError(message);
    }
}

void Object::internalPutIndexed(uint index, const ValueRef value)
{
    if (internalClass->engine->hasException)
        return;

    PropertyAttributes attrs;

    Property *pd = arrayData->getProperty(index);
    if (pd)
        attrs = arrayData->attributes(index);

    if (!pd && isStringObject()) {
        pd = static_cast<StringObject *>(this)->getIndex(index);
        if (pd)
            // not writable
            goto reject;
    }

    // clause 1
    if (pd) {
        if (attrs.isAccessor()) {
            if (pd->setter())
                goto cont;
            goto reject;
        } else if (!attrs.isWritable())
            goto reject;
        else
            pd->value = *value;
        return;
    } else if (!prototype()) {
        if (!extensible)
            goto reject;
    } else {
        // clause 4
        if ((pd = prototype()->__getPropertyDescriptor__(index, &attrs))) {
            if (attrs.isAccessor()) {
                if (!pd->setter())
                    goto reject;
            } else if (!extensible || !attrs.isWritable()) {
                goto reject;
            }
        } else if (!extensible) {
            goto reject;
        }
    }

    cont:

    // Clause 5
    if (pd && attrs.isAccessor()) {
        assert(pd->setter() != 0);

        Scope scope(engine());
        ScopedCallData callData(scope, 1);
        callData->args[0] = *value;
        callData->thisObject = this;
        pd->setter()->call(callData);
        return;
    }

    arraySet(index, value);
    return;

  reject:
    if (engine()->currentContext()->strictMode)
        engine()->currentContext()->throwTypeError();
}

// Section 8.12.7
bool Object::internalDeleteProperty(const StringRef name)
{
    if (internalClass->engine->hasException)
        return false;

    uint idx = name->asArrayIndex();
    if (idx != UINT_MAX)
        return deleteIndexedProperty(idx);

    name->makeIdentifier();

    uint memberIdx = internalClass->find(name);
    if (memberIdx != UINT_MAX) {
        if (internalClass->propertyData[memberIdx].isConfigurable()) {
            InternalClass::removeMember(this, name->identifier);
            return true;
        }
        if (engine()->currentContext()->strictMode)
            engine()->currentContext()->throwTypeError();
        return false;
    }

    return true;
}

bool Object::internalDeleteIndexedProperty(uint index)
{
    if (internalClass->engine->hasException)
        return false;

    if (!arrayData || arrayData->vtable()->del(this, index))
        return true;

    if (engine()->currentContext()->strictMode)
        engine()->currentContext()->throwTypeError();
    return false;
}

// Section 8.12.9
bool Object::__defineOwnProperty__(ExecutionContext *ctx, const StringRef name, const Property &p, PropertyAttributes attrs)
{
    uint idx = name->asArrayIndex();
    if (idx != UINT_MAX)
        return __defineOwnProperty__(ctx, idx, p, attrs);

    name->makeIdentifier();

    Scope scope(ctx);
    Property *current;
    PropertyAttributes *cattrs;
    uint memberIndex;

    if (isArrayObject() && name->equals(ctx->engine->id_length)) {
        assert(ArrayObject::LengthPropertyIndex == internalClass->find(ctx->engine->id_length));
        Property *lp = propertyAt(ArrayObject::LengthPropertyIndex);
        cattrs = internalClass->propertyData.constData() + ArrayObject::LengthPropertyIndex;
        if (attrs.isEmpty() || p.isSubset(attrs, *lp, *cattrs))
            return true;
        if (!cattrs->isWritable() || attrs.type() == PropertyAttributes::Accessor || attrs.isConfigurable() || attrs.isEnumerable())
            goto reject;
        bool succeeded = true;
        if (attrs.type() == PropertyAttributes::Data) {
            bool ok;
            uint l = p.value.asArrayLength(&ok);
            if (!ok) {
                ScopedValue v(scope, p.value);
                ctx->throwRangeError(v);
                return false;
            }
            succeeded = setArrayLength(l);
        }
        if (attrs.hasWritable() && !attrs.isWritable())
            cattrs->setWritable(false);
        if (!succeeded)
            goto reject;
        if (attrs.isAccessor())
            hasAccessorProperty = 1;
        return true;
    }

    // Clause 1
    memberIndex = internalClass->find(name.getPointer());
    current = (memberIndex < UINT_MAX) ? propertyAt(memberIndex) : 0;
    cattrs = internalClass->propertyData.constData() + memberIndex;

    if (!current) {
        // clause 3
        if (!extensible)
            goto reject;
        // clause 4
        Property pd;
        pd.copy(p, attrs);
        pd.fullyPopulated(&attrs);
        insertMember(name, pd, attrs);
        return true;
    }

    return __defineOwnProperty__(ctx, memberIndex, name, p, attrs);
reject:
  if (ctx->strictMode)
      ctx->throwTypeError();
  return false;
}

bool Object::__defineOwnProperty__(ExecutionContext *ctx, uint index, const Property &p, PropertyAttributes attrs)
{
    // 15.4.5.1, 4b
    if (isArrayObject() && index >= getLength() && !internalClass->propertyData[ArrayObject::LengthPropertyIndex].isWritable())
        goto reject;

    if (ArgumentsObject::isNonStrictArgumentsObject(this))
        return static_cast<ArgumentsObject *>(this)->defineOwnProperty(ctx, index, p, attrs);

    return defineOwnProperty2(ctx, index, p, attrs);
reject:
  if (ctx->strictMode)
      ctx->throwTypeError();
  return false;
}

bool Object::defineOwnProperty2(ExecutionContext *ctx, uint index, const Property &p, PropertyAttributes attrs)
{
    Property *current = 0;

    // Clause 1
    {
        current = arrayData->getProperty(index);
        if (!current && isStringObject())
            current = static_cast<StringObject *>(this)->getIndex(index);
    }

    if (!current) {
        // clause 3
        if (!extensible)
            goto reject;
        // clause 4
        Property pp;
        pp.copy(p, attrs);
        pp.fullyPopulated(&attrs);
        if (attrs == Attr_Data) {
            Scope scope(ctx);
            ScopedValue v(scope, pp.value);
            arraySet(index, v);
        } else {
            arraySet(index, pp, attrs);
        }
        return true;
    }

    return __defineOwnProperty__(ctx, index, StringRef::null(), p, attrs);
reject:
  if (ctx->strictMode)
      ctx->throwTypeError();
  return false;
}

bool Object::__defineOwnProperty__(ExecutionContext *ctx, uint index, const StringRef member, const Property &p, PropertyAttributes attrs)
{
    // clause 5
    if (attrs.isEmpty())
        return true;

    Property *current;
    PropertyAttributes cattrs;
    if (!member.isNull()) {
        current = propertyAt(index);
        cattrs = internalClass->propertyData[index];
    } else {
        current = arrayData->getProperty(index);
        cattrs = arrayData->attributes(index);
    }

    // clause 6
    if (p.isSubset(attrs, *current, cattrs))
        return true;

    // clause 7
    if (!cattrs.isConfigurable()) {
        if (attrs.isConfigurable())
            goto reject;
        if (attrs.hasEnumerable() && attrs.isEnumerable() != cattrs.isEnumerable())
            goto reject;
    }

    // clause 8
    if (attrs.isGeneric() || current->value.isEmpty())
        goto accept;

    // clause 9
    if (cattrs.isData() != attrs.isData()) {
        // 9a
        if (!cattrs.isConfigurable())
            goto reject;
        if (cattrs.isData()) {
            // 9b
            cattrs.setType(PropertyAttributes::Accessor);
            cattrs.clearWritable();
            if (member.isNull()) {
                // need to convert the array and the slot
                initSparseArray();
                setArrayAttributes(index, cattrs);
                current = arrayData->getProperty(index);
            }
            current->setGetter(0);
            current->setSetter(0);
        } else {
            // 9c
            cattrs.setType(PropertyAttributes::Data);
            cattrs.setWritable(false);
            if (member.isNull()) {
                // need to convert the array and the slot
                setArrayAttributes(index, cattrs);
                current = arrayData->getProperty(index);
            }
            current->value = Primitive::undefinedValue();
        }
    } else if (cattrs.isData() && attrs.isData()) { // clause 10
        if (!cattrs.isConfigurable() && !cattrs.isWritable()) {
            if (attrs.isWritable() || !current->value.sameValue(p.value))
                goto reject;
        }
    } else { // clause 10
        Q_ASSERT(cattrs.isAccessor() && attrs.isAccessor());
        if (!cattrs.isConfigurable()) {
            if (!p.value.isEmpty() && current->value.val != p.value.val)
                goto reject;
            if (!p.set.isEmpty() && current->set.val != p.set.val)
                goto reject;
        }
    }

  accept:

    current->merge(cattrs, p, attrs);
    if (!member.isNull()) {
        InternalClass::changeMember(this, member.getPointer(), cattrs);
    } else {
        setArrayAttributes(index, cattrs);
    }
    if (cattrs.isAccessor())
        hasAccessorProperty = 1;
    return true;
  reject:
    if (ctx->strictMode)
        ctx->throwTypeError();
    return false;
}


bool Object::__defineOwnProperty__(ExecutionContext *ctx, const QString &name, const Property &p, PropertyAttributes attrs)
{
    Scope scope(ctx);
    ScopedString s(scope, ctx->engine->newString(name));
    return __defineOwnProperty__(ctx, s, p, attrs);
}


void Object::copyArrayData(Object *other)
{
    Q_ASSERT(isArrayObject());
    Scope scope(engine());

    if (other->protoHasArray() || other->hasAccessorProperty) {
        uint len = other->getLength();
        Q_ASSERT(len);

        ScopedValue v(scope);
        for (uint i = 0; i < len; ++i) {
            arraySet(i, (v = other->getIndexed(i)));
        }
    } else if (!other->arrayData) {
        ;
    } else if (other->hasAccessorProperty && other->arrayData->attrs && other->arrayData->isSparse()){
        // do it the slow way
        ScopedValue v(scope);
        for (const SparseArrayNode *it = static_cast<const SparseArrayData *>(other->arrayData)->sparse->begin();
             it != static_cast<const SparseArrayData *>(other->arrayData)->sparse->end(); it = it->nextNode()) {
            v = other->getValue(reinterpret_cast<Property *>(other->arrayData->data + it->value), other->arrayData->attrs[it->value]);
            arraySet(it->key(), v);
        }
    } else {
        Q_ASSERT(!arrayData && other->arrayData);
        ArrayData::realloc(this, other->arrayData->type, 0, other->arrayData->alloc, other->arrayData->attrs);
        if (other->arrayType() == ArrayData::Sparse) {
            SparseArrayData *od = static_cast<SparseArrayData *>(other->arrayData);
            SparseArrayData *dd = static_cast<SparseArrayData *>(arrayData);
            dd->sparse = new SparseArray(*od->sparse);
            dd->freeList = od->freeList;
        } else {
            SimpleArrayData *d = static_cast<SimpleArrayData *>(arrayData);
            d->len = static_cast<SimpleArrayData *>(other->arrayData)->len;
            d->offset = 0;
        }
        memcpy(arrayData->data, other->arrayData->data, arrayData->alloc*sizeof(Value));
    }
    setArrayLengthUnchecked(other->getLength());
}

uint Object::getLength(const Managed *m)
{
    Scope scope(m->engine());
    ScopedValue v(scope, static_cast<Object *>(const_cast<Managed *>(m))->get(scope.engine->id_length));
    return v->toUInt32();
}

bool Object::setArrayLength(uint newLen)
{
    Q_ASSERT(isArrayObject());
    if (!internalClass->propertyData[ArrayObject::LengthPropertyIndex].isWritable())
        return false;
    uint oldLen = getLength();
    bool ok = true;
    if (newLen < oldLen) {
        if (!arrayData) {
            Q_ASSERT(!newLen);
        } else {
            uint l = arrayData->vtable()->truncate(this, newLen);
            if (l != newLen)
                ok = false;
            newLen = l;
        }
    } else {
        if (newLen >= 0x100000)
            initSparseArray();
    }
    setArrayLengthUnchecked(newLen);
    return ok;
}

void Object::initSparseArray()
{
    if (arrayType() == ArrayData::Sparse)
        return;

    ArrayData::realloc(this, ArrayData::Sparse, 0, 0, false);
}


DEFINE_OBJECT_VTABLE(ArrayObject);

ArrayObject::ArrayObject(ExecutionEngine *engine, const QStringList &list)
    : Object(engine->arrayClass)
{
    init(engine);
    Scope scope(engine);
    ScopedValue protectThis(scope, this);

    // Converts a QStringList to JS.
    // The result is a new Array object with length equal to the length
    // of the QStringList, and the elements being the QStringList's
    // elements converted to JS Strings.
    int len = list.count();
    arrayReserve(len);
    ScopedValue v(scope);
    for (int ii = 0; ii < len; ++ii)
        arrayPut(ii, (v = engine->newString(list.at(ii))));
    setArrayLengthUnchecked(len);
}

void ArrayObject::init(ExecutionEngine *engine)
{
    Q_UNUSED(engine);

    memberData[LengthPropertyIndex] = Primitive::fromInt32(0);
}

ReturnedValue ArrayObject::getLookup(Managed *m, Lookup *l)
{
    if (l->name->equals(m->engine()->id_length)) {
        // special case, as the property is on the object itself
        l->getter = Lookup::arrayLengthGetter;
        ArrayObject *a = static_cast<ArrayObject *>(m);
        return a->memberData[ArrayObject::LengthPropertyIndex].asReturnedValue();
    }
    return Object::getLookup(m, l);
}

uint ArrayObject::getLength(const Managed *m)
{
    const ArrayObject *a = static_cast<const ArrayObject *>(m);
    if (a->memberData[ArrayObject::LengthPropertyIndex].isInteger())
        return a->memberData[ArrayObject::LengthPropertyIndex].integerValue();
    return Primitive::toUInt32(a->memberData[ArrayObject::LengthPropertyIndex].doubleValue());
}

QStringList ArrayObject::toQStringList() const
{
    QStringList result;

    QV4::ExecutionEngine *engine = internalClass->engine;
    Scope scope(engine);
    ScopedValue v(scope);

    uint32_t length = getLength();
    for (uint32_t i = 0; i < length; ++i) {
        v = const_cast<ArrayObject *>(this)->getIndexed(i);
        result.append(v->toQStringNoThrow());
    }
    return result;
}
