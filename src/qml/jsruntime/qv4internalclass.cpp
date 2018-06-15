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

#include <qv4internalclass_p.h>
#include <qv4string_p.h>
#include <qv4engine_p.h>
#include <qv4identifier_p.h>
#include "qv4object_p.h"
#include "qv4identifiertable_p.h"
#include "qv4value_p.h"

QT_BEGIN_NAMESPACE

namespace QV4 {

static const uchar prime_deltas[] = {
    0,  0,  1,  3,  1,  5,  3,  3,  1,  9,  7,  5,  3,  9, 25,  3,
    1, 21,  3, 21,  7, 15,  9,  5,  3, 29, 15,  0,  0,  0,  0,  0
};

static inline int primeForNumBits(int numBits)
{
    return (1 << numBits) + prime_deltas[numBits];
}

PropertyHashData::PropertyHashData(int numBits)
    : refCount(1)
    , size(0)
    , numBits(numBits)
{
    alloc = primeForNumBits(numBits);
    entries = (PropertyHash::Entry *)malloc(alloc*sizeof(PropertyHash::Entry));
    memset(entries, 0, alloc*sizeof(PropertyHash::Entry));
}

void PropertyHash::addEntry(const PropertyHash::Entry &entry, int classSize)
{
    // fill up to max 50%
    bool grow = (d->alloc <= d->size*2);

    if (classSize < d->size || grow)
        detach(grow, classSize);

    uint idx = entry.identifier.id % d->alloc;
    while (d->entries[idx].identifier.isValid()) {
        ++idx;
        idx %= d->alloc;
    }
    d->entries[idx] = entry;
    ++d->size;
}

int PropertyHash::removeIdentifier(Identifier identifier, int classSize)
{
    int val = -1;
    PropertyHashData *dd = new PropertyHashData(d->numBits);
    for (int i = 0; i < d->alloc; ++i) {
        const Entry &e = d->entries[i];
        if (!e.identifier.isValid() || e.index >= static_cast<unsigned>(classSize))
            continue;
        if (e.identifier == identifier) {
            val = e.index;
            continue;
        }
        uint idx = e.identifier.id % dd->alloc;
        while (dd->entries[idx].identifier.isValid()) {
            ++idx;
            idx %= dd->alloc;
        }
        dd->entries[idx] = e;
    }
    dd->size = classSize;
    if (!--d->refCount)
        delete d;
    d = dd;

    Q_ASSERT(val != -1);
    return val;
}

void PropertyHash::detach(bool grow, int classSize)
{
    if (d->refCount == 1 && !grow)
        return;

    PropertyHashData *dd = new PropertyHashData(grow ? d->numBits + 1 : d->numBits);
    for (int i = 0; i < d->alloc; ++i) {
        const Entry &e = d->entries[i];
        if (!e.identifier.isValid() || e.index >= static_cast<unsigned>(classSize))
            continue;
        uint idx = e.identifier.id % dd->alloc;
        while (dd->entries[idx].identifier.isValid()) {
            ++idx;
            idx %= dd->alloc;
        }
        dd->entries[idx] = e;
    }
    dd->size = classSize;
    if (!--d->refCount)
        delete d;
    d = dd;
}

namespace Heap {

void InternalClass::init(ExecutionEngine *engine)
{
    Base::init();
    new (&propertyTable) PropertyHash();
    new (&nameMap) SharedInternalClassData<Identifier>();
    new (&propertyData) SharedInternalClassData<PropertyAttributes>();
    new (&transitions) std::vector<Transition>();

    this->engine = engine;
    vtable = QV4::InternalClass::staticVTable();
//    prototype = nullptr;
//    parent = nullptr;
//    size = 0;
    extensible = true;
    isFrozen = false;
    isSealed = false;
    isUsedAsProto = false;
    protoId = engine->newProtoId();

    // Also internal classes need an internal class pointer. Simply make it point to itself
    internalClass.set(engine, this);
}


void InternalClass::init(Heap::InternalClass *other)
{
    Base::init();
    Q_ASSERT(!other->isFrozen);
    new (&propertyTable) PropertyHash(other->propertyTable);
    new (&nameMap) SharedInternalClassData<Identifier>(other->nameMap);
    new (&propertyData) SharedInternalClassData<PropertyAttributes>(other->propertyData);
    new (&transitions) std::vector<Transition>();

    engine = other->engine;
    vtable = other->vtable;
    prototype = other->prototype;
    parent = other;
    size = other->size;
    extensible = other->extensible;
    isSealed = other->isSealed;
    isFrozen = other->isFrozen;
    isUsedAsProto = other->isUsedAsProto;
    protoId = engine->newProtoId();

    internalClass.set(engine, other->internalClass);
}

void InternalClass::destroy()
{
#ifndef QT_NO_DEBUG
    for (const auto &t : transitions) {
        Q_ASSERT(!t.lookup || !t.lookup->isMarked());
    }
#endif
    if (parent && parent->engine && parent->isMarked())
        parent->removeChildEntry(this);

    propertyTable.~PropertyHash();
    nameMap.~SharedInternalClassData<Identifier>();
    propertyData.~SharedInternalClassData<PropertyAttributes>();
    transitions.~vector<Transition>();
    engine = nullptr;
    Base::destroy();
}

static void insertHoleIntoPropertyData(QV4::Object *object, int idx)
{
    Heap::Object *o = object->d();
    ExecutionEngine *v4 = o->internalClass->engine;
    int size = o->internalClass->size;
    for (int i = size - 1; i > idx; --i)
        o->setProperty(v4, i, *o->propertyData(i - 1));
}

static void removeFromPropertyData(QV4::Object *object, int idx, bool accessor = false)
{
    Heap::Object *o = object->d();
    ExecutionEngine *v4 = o->internalClass->engine;
    int size = o->internalClass->size;
    for (int i = idx; i < size; ++i)
        o->setProperty(v4, i, *o->propertyData(i + (accessor ? 2 : 1)));
    o->setProperty(v4, size, Primitive::undefinedValue());
    if (accessor)
        o->setProperty(v4, size + 1, Primitive::undefinedValue());
}

void InternalClass::changeMember(QV4::Object *object, Identifier id, PropertyAttributes data, uint *index)
{
    Q_ASSERT(id.isValid());
    uint idx;
    Heap::InternalClass *oldClass = object->internalClass();
    Heap::InternalClass *newClass = oldClass->changeMember(id, data, &idx);
    if (index)
        *index = idx;

    uint oldSize = oldClass->size;
    object->setInternalClass(newClass);
    // don't use oldClass anymore, it could be GC'ed
    if (newClass->size > oldSize) {
        Q_ASSERT(newClass->size == oldSize + 1);
        insertHoleIntoPropertyData(object, idx);
    } else if (newClass->size < oldSize) {
        Q_ASSERT(newClass->size == oldSize - 1);
        removeFromPropertyData(object, idx + 1);
    }
}

InternalClassTransition &InternalClass::lookupOrInsertTransition(const InternalClassTransition &t)
{
    std::vector<Transition>::iterator it = std::lower_bound(transitions.begin(), transitions.end(), t);
    if (it != transitions.end() && *it == t) {
        return *it;
    } else {
        it = transitions.insert(it, t);
        return *it;
    }
}

static void addDummyEntry(InternalClass *newClass, PropertyHash::Entry e)
{
    // add a dummy entry, since we need two entries for accessors
    newClass->propertyTable.addEntry(e, newClass->size);
    newClass->nameMap.add(newClass->size, Identifier::invalid());
    newClass->propertyData.add(newClass->size, PropertyAttributes());
    ++newClass->size;
}

Heap::InternalClass *InternalClass::changeMember(Identifier identifier, PropertyAttributes data, uint *index)
{
    data.resolve();
    uint idx = find(identifier);
    Q_ASSERT(idx != UINT_MAX);

    if (index)
        *index = idx;

    if (data == propertyData.at(idx))
        return static_cast<Heap::InternalClass *>(this);

    Transition temp = { { identifier }, nullptr, (int)data.flags() };
    Transition &t = lookupOrInsertTransition(temp);
    if (t.lookup)
        return t.lookup;

    // create a new class and add it to the tree
    Heap::InternalClass *newClass = engine->newClass(this);
    if (data.isAccessor() != propertyData.at(idx).isAccessor()) {
        // this changes the layout of the class, so we need to rebuild the data
        newClass->propertyTable = PropertyHash();
        newClass->nameMap = SharedInternalClassData<Identifier>();
        newClass->propertyData = SharedInternalClassData<PropertyAttributes>();
        newClass->size = 0;
        for (uint i = 0; i < size; ++i) {
            Identifier identifier = nameMap.at(i);
            PropertyHash::Entry e = { identifier, newClass->size };
            if (i && !identifier.isValid())
                e.identifier = nameMap.at(i - 1);
            newClass->propertyTable.addEntry(e, newClass->size);
            newClass->nameMap.add(newClass->size, identifier);
            if (i == idx) {
                newClass->propertyData.add(newClass->size, data);
                ++newClass->size;
                if (data.isAccessor())
                    addDummyEntry(newClass, e);
                else
                    ++i;
            } else {
                newClass->propertyData.add(newClass->size, propertyData.at(i));
                ++newClass->size;
            }
        }
    } else {
        newClass->propertyData.set(idx, data);
    }

    t.lookup = newClass;
    Q_ASSERT(t.lookup);
    return newClass;
}

Heap::InternalClass *InternalClass::changePrototypeImpl(Heap::Object *proto)
{
    Scope scope(engine);
    ScopedValue protectThis(scope, this);
    if (proto)
        proto->setUsedAsProto();
    Q_ASSERT(prototype != proto);
    Q_ASSERT(!proto || proto->internalClass->isUsedAsProto);

    Transition temp = { { Identifier::invalid() }, nullptr, Transition::PrototypeChange };
    temp.prototype = proto;

    Transition &t = lookupOrInsertTransition(temp);
    if (t.lookup)
        return t.lookup;

    // create a new class and add it to the tree
    Heap::InternalClass *newClass = engine->newClass(this);
    newClass->prototype = proto;

    t.lookup = newClass;

    return newClass;
}

Heap::InternalClass *InternalClass::changeVTableImpl(const VTable *vt)
{
    Q_ASSERT(vtable != vt);

    Transition temp = { { Identifier::invalid() }, nullptr, Transition::VTableChange };
    temp.vtable = vt;

    Transition &t = lookupOrInsertTransition(temp);
    if (t.lookup)
        return t.lookup;

    // create a new class and add it to the tree
    Heap::InternalClass *newClass = engine->newClass(this);
    newClass->vtable = vt;

    t.lookup = newClass;
    Q_ASSERT(t.lookup);
    Q_ASSERT(newClass->vtable);
    return newClass;
}

Heap::InternalClass *InternalClass::nonExtensible()
{
    if (!extensible)
        return this;

    Transition temp = { { Identifier::invalid() }, nullptr, Transition::NotExtensible};
    Transition &t = lookupOrInsertTransition(temp);
    if (t.lookup)
        return t.lookup;

    Heap::InternalClass *newClass = engine->newClass(this);
    newClass->extensible = false;

    t.lookup = newClass;
    Q_ASSERT(t.lookup);
    return newClass;
}

void InternalClass::addMember(QV4::Object *object, Identifier id, PropertyAttributes data, uint *index)
{
    Q_ASSERT(id.isValid());
    data.resolve();
    if (object->internalClass()->propertyTable.lookup(id) < object->internalClass()->size) {
        changeMember(object, id, data, index);
        return;
    }

    uint idx;
    Heap::InternalClass *newClass = object->internalClass()->addMemberImpl(id, data, &idx);
    if (index)
        *index = idx;

    object->setInternalClass(newClass);
}

Heap::InternalClass *InternalClass::addMember(Identifier identifier, PropertyAttributes data, uint *index)
{
    Q_ASSERT(identifier.isValid());
    data.resolve();

    if (propertyTable.lookup(identifier) < size)
        return changeMember(identifier, data, index);

    return addMemberImpl(identifier, data, index);
}

Heap::InternalClass *InternalClass::addMemberImpl(Identifier identifier, PropertyAttributes data, uint *index)
{
    Transition temp = { { identifier }, nullptr, (int)data.flags() };
    Transition &t = lookupOrInsertTransition(temp);

    if (index)
        *index = size;

    if (t.lookup)
        return t.lookup;

    // create a new class and add it to the tree
    Heap::InternalClass *newClass = engine->newClass(this);
    PropertyHash::Entry e = { identifier, newClass->size };
    newClass->propertyTable.addEntry(e, newClass->size);

    newClass->nameMap.add(newClass->size, identifier);
    newClass->propertyData.add(newClass->size, data);
    ++newClass->size;
    if (data.isAccessor())
        addDummyEntry(newClass, e);

    t.lookup = newClass;
    Q_ASSERT(t.lookup);
    return newClass;
}

void InternalClass::removeChildEntry(InternalClass *child)
{
    Q_ASSERT(engine);
    for (auto &t : transitions) {
        if (t.lookup == child) {
            t.lookup = nullptr;
            return;
        }
    }
    Q_UNREACHABLE();

}

void InternalClass::removeMember(QV4::Object *object, Identifier identifier)
{
    Heap::InternalClass *oldClass = object->internalClass();
    Q_ASSERT(oldClass->propertyTable.lookup(identifier) < oldClass->size);

    Transition temp = { { identifier }, nullptr, Transition::RemoveMember };
    Transition &t = object->internalClass()->lookupOrInsertTransition(temp);

    if (!t.lookup) {
        // create a new class and add it to the tree
        Heap::InternalClass *newClass = oldClass->engine->newClass(oldClass);
        // simply make the entry inaccessible
        int idx = newClass->propertyTable.removeIdentifier(identifier, oldClass->size);
        newClass->nameMap.set(idx, Identifier::invalid());
        newClass->propertyData.set(idx, PropertyAttributes());
        t.lookup = newClass;
        Q_ASSERT(t.lookup);
    }
    object->setInternalClass(t.lookup);

    // we didn't remove the data slot, just made it inaccessible
    Q_ASSERT(object->internalClass()->size == oldClass->size);
}

Heap::InternalClass *InternalClass::sealed()
{
    if (isSealed)
        return this;

    bool alreadySealed = !extensible;
    for (uint i = 0; i < size; ++i) {
        PropertyAttributes attrs = propertyData.at(i);
        if (attrs.isEmpty())
            continue;
        if (attrs.isConfigurable()) {
            alreadySealed = false;
            break;
        }
    }

    if (alreadySealed) {
        isSealed = true;
        return this;
    }

    Transition temp = { { Identifier::invalid() }, nullptr, InternalClassTransition::Sealed };
    Transition &t = lookupOrInsertTransition(temp);

    if (t.lookup) {
        Q_ASSERT(t.lookup && t.lookup->isSealed);
        return t.lookup;
    }

    Heap::InternalClass *s = engine->newClass(this);

    for (uint i = 0; i < size; ++i) {
        PropertyAttributes attrs = propertyData.at(i);
        if (attrs.isEmpty())
            continue;
        attrs.setConfigurable(false);
        s->propertyData.set(i, attrs);
    }
    s->extensible = false;
    s->isSealed = true;

    t.lookup = s;
    return s;
}

Heap::InternalClass *InternalClass::frozen()
{
    if (isFrozen)
        return this;

    bool alreadyFrozen = !extensible;
    for (uint i = 0; i < size; ++i) {
        PropertyAttributes attrs = propertyData.at(i);
        if (attrs.isEmpty())
            continue;
        if ((attrs.isData() && attrs.isWritable()) || attrs.isConfigurable()) {
            alreadyFrozen = false;
            break;
        }
    }

    if (alreadyFrozen) {
        isSealed = true;
        isFrozen = true;
        return this;
    }

    Transition temp = { { Identifier::invalid() }, nullptr, InternalClassTransition::Frozen };
    Transition &t = lookupOrInsertTransition(temp);

    if (t.lookup) {
        Q_ASSERT(t.lookup && t.lookup->isSealed && t.lookup->isFrozen);
        return t.lookup;
    }

    Heap::InternalClass *f = engine->newClass(this);

    for (uint i = 0; i < size; ++i) {
        PropertyAttributes attrs = propertyData.at(i);
        if (attrs.isEmpty())
            continue;
        if (attrs.isData())
            attrs.setWritable(false);
        attrs.setConfigurable(false);
        f->propertyData.set(i, attrs);
    }
    f->extensible = false;
    f->isSealed = true;
    f->isFrozen = true;

    t.lookup = f;
    return f;
}

Heap::InternalClass *InternalClass::propertiesFrozen() const
{
    Scope scope(engine);
    Scoped<QV4::InternalClass> frozen(scope, engine->internalClasses(EngineBase::Class_Empty)->changeVTable(vtable));
    frozen = frozen->changePrototype(prototype);
    for (uint i = 0; i < size; ++i) {
        PropertyAttributes attrs = propertyData.at(i);
        if (attrs.isEmpty())
            continue;
        attrs.setWritable(false);
        attrs.setConfigurable(false);
        frozen = frozen->addMember(nameMap.at(i), attrs);
    }
    return frozen->d();
}

Heap::InternalClass *InternalClass::asProtoClass()
{
    if (isUsedAsProto)
        return this;

    Transition temp = { { Identifier::invalid() }, nullptr, Transition::ProtoClass };
    Transition &t = lookupOrInsertTransition(temp);
    if (t.lookup)
        return t.lookup;

    Heap::InternalClass *newClass = engine->newClass(this);
    newClass->isUsedAsProto = true;

    t.lookup = newClass;
    Q_ASSERT(t.lookup);
    return newClass;
}

static void updateProtoUsage(Heap::Object *o, Heap::InternalClass *ic)
{
    if (ic->prototype == o)
        ic->protoId = ic->engine->newProtoId();
    for (auto &t : ic->transitions) {
        if (t.lookup)
            updateProtoUsage(o, t.lookup);
    }
}


void InternalClass::updateProtoUsage(Heap::Object *o)
{
    Q_ASSERT(isUsedAsProto);
    Heap::InternalClass *ic = engine->internalClasses(EngineBase::Class_Empty);
    Q_ASSERT(!ic->prototype);

    Heap::updateProtoUsage(o, ic);
}

void InternalClass::markObjects(Heap::Base *b, MarkStack *stack)
{
    Heap::InternalClass *ic = static_cast<Heap::InternalClass *>(b);
    if (ic->prototype)
        ic->prototype->mark(stack);
    if (ic->parent)
        ic->parent->mark(stack);

    for (uint i = 0; i < ic->size; ++i) {
        Identifier id = ic->nameMap.at(i);
        if (Heap::Base *b = id.asHeapObject())
            b->mark(stack);
    }
}

}

}

QT_END_NAMESPACE
