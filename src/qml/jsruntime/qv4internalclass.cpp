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

using namespace QV4;

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

    uint idx = entry.identifier->hashValue % d->alloc;
    while (d->entries[idx].identifier) {
        ++idx;
        idx %= d->alloc;
    }
    d->entries[idx] = entry;
    ++d->size;
}

int PropertyHash::removeIdentifier(Identifier *identifier, int classSize)
{
    detach(false, classSize);
    uint idx = identifier->hashValue % d->alloc;
    while (1) {
        if (d->entries[idx].identifier == identifier) {
            int val = d->entries[idx].index;
            d->entries[idx] = { nullptr, 0 };
            return val;
        }

        ++idx;
        idx %= d->alloc;
    }
    Q_UNREACHABLE();
}

void PropertyHash::detach(bool grow, int classSize)
{
    if (d->refCount == 1 && !grow)
        return;

    PropertyHashData *dd = new PropertyHashData(grow ? d->numBits + 1 : d->numBits);
    for (int i = 0; i < d->alloc; ++i) {
        const Entry &e = d->entries[i];
        if (!e.identifier || e.index >= static_cast<unsigned>(classSize))
            continue;
        uint idx = e.identifier->hashValue % dd->alloc;
        while (dd->entries[idx].identifier) {
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


InternalClass::InternalClass(ExecutionEngine *engine)
    : engine(engine)
    , vtable(nullptr)
    , prototype(nullptr)
    , parent(nullptr)
    , m_sealed(nullptr)
    , m_frozen(nullptr)
    , size(0)
    , extensible(true)
{
    id = engine->newInternalClassId();
}


InternalClass::InternalClass(QV4::InternalClass *other)
    : QQmlJS::Managed()
    , engine(other->engine)
    , vtable(other->vtable)
    , prototype(other->prototype)
    , parent(other)
    , propertyTable(other->propertyTable)
    , nameMap(other->nameMap)
    , propertyData(other->propertyData)
    , m_sealed(nullptr)
    , m_frozen(nullptr)
    , size(other->size)
    , extensible(other->extensible)
    , isUsedAsProto(other->isUsedAsProto)
{
    id = engine->newInternalClassId();
}

static void insertHoleIntoPropertyData(Object *object, int idx)
{
    Heap::Object *o = object->d();
    ExecutionEngine *v4 = o->internalClass->engine;
    int size = o->internalClass->size;
    for (int i = size - 1; i > idx; --i)
        o->setProperty(v4, i, *o->propertyData(i - 1));
}

static void removeFromPropertyData(Object *object, int idx, bool accessor = false)
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

void InternalClass::changeMember(Object *object, String *string, PropertyAttributes data, uint *index)
{
    uint idx;
    InternalClass *oldClass = object->internalClass();
    InternalClass *newClass = oldClass->changeMember(string->identifier(), data, &idx);
    if (index)
        *index = idx;

    object->setInternalClass(newClass);
    if (newClass->size > oldClass->size) {
        Q_ASSERT(newClass->size == oldClass->size + 1);
        insertHoleIntoPropertyData(object, idx);
    } else if (newClass->size < oldClass->size) {
        Q_ASSERT(newClass->size == oldClass->size - 1);
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
    newClass->nameMap.add(newClass->size, nullptr);
    newClass->propertyData.add(newClass->size, PropertyAttributes());
    ++newClass->size;
}

InternalClass *InternalClass::changeMember(Identifier *identifier, PropertyAttributes data, uint *index)
{
    data.resolve();
    uint idx = find(identifier);
    Q_ASSERT(idx != UINT_MAX);

    if (index)
        *index = idx;

    if (data == propertyData.at(idx))
        return this;

    Transition temp = { { identifier }, nullptr, (int)data.flags() };
    Transition &t = lookupOrInsertTransition(temp);
    if (t.lookup)
        return t.lookup;

    // create a new class and add it to the tree
    InternalClass *newClass = engine->newClass(this);
    if (data.isAccessor() != propertyData.at(idx).isAccessor()) {
        // this changes the layout of the class, so we need to rebuild the data
        newClass->propertyTable = PropertyHash();
        newClass->nameMap = SharedInternalClassData<Identifier *>();
        newClass->propertyData = SharedInternalClassData<PropertyAttributes>();
        newClass->size = 0;
        for (uint i = 0; i < size; ++i) {
            Identifier *identifier = nameMap.at(i);
            PropertyHash::Entry e = { identifier, newClass->size };
            if (!identifier)
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

InternalClass *InternalClass::changePrototypeImpl(Heap::Object *proto)
{
    if (proto)
        proto->setUsedAsProto();
    Q_ASSERT(prototype != proto);
    Q_ASSERT(!proto || proto->internalClass->isUsedAsProto);

    Transition temp = { { nullptr }, nullptr, Transition::PrototypeChange };
    temp.prototype = proto;

    Transition &t = lookupOrInsertTransition(temp);
    if (t.lookup)
        return t.lookup;

    // create a new class and add it to the tree
    InternalClass *newClass = engine->newClass(this);
    newClass->prototype = proto;

    t.lookup = newClass;

    return newClass;
}

InternalClass *InternalClass::changeVTableImpl(const VTable *vt)
{
    Q_ASSERT(vtable != vt);

    Transition temp = { { nullptr }, nullptr, Transition::VTableChange };
    temp.vtable = vt;

    Transition &t = lookupOrInsertTransition(temp);
    if (t.lookup)
        return t.lookup;

    // create a new class and add it to the tree
    InternalClass *newClass = engine->newClass(this);
    newClass->vtable = vt;

    t.lookup = newClass;
    Q_ASSERT(t.lookup);
    Q_ASSERT(newClass->vtable);
    return newClass;
}

InternalClass *InternalClass::nonExtensible()
{
    if (!extensible)
        return this;

    Transition temp = { { nullptr }, nullptr, Transition::NotExtensible};
    Transition &t = lookupOrInsertTransition(temp);
    if (t.lookup)
        return t.lookup;

    InternalClass *newClass = engine->newClass(this);
    newClass->extensible = false;

    t.lookup = newClass;
    Q_ASSERT(t.lookup);
    return newClass;
}

void InternalClass::addMember(Object *object, String *string, PropertyAttributes data, uint *index)
{
    data.resolve();
    object->internalClass()->engine->identifierTable->identifier(string);
    if (object->internalClass()->propertyTable.lookup(string->d()->identifier) < object->internalClass()->size) {
        changeMember(object, string, data, index);
        return;
    }

    uint idx;
    InternalClass *newClass = object->internalClass()->addMemberImpl(string->identifier(), data, &idx);
    if (index)
        *index = idx;

    object->setInternalClass(newClass);
}

InternalClass *InternalClass::addMember(String *string, PropertyAttributes data, uint *index)
{
    engine->identifierTable->identifier(string);
    return addMember(string->identifier(), data, index);
}

InternalClass *InternalClass::addMember(Identifier *identifier, PropertyAttributes data, uint *index)
{
    data.resolve();

    if (propertyTable.lookup(identifier) < size)
        return changeMember(identifier, data, index);

    return addMemberImpl(identifier, data, index);
}

InternalClass *InternalClass::addMemberImpl(Identifier *identifier, PropertyAttributes data, uint *index)
{
    Transition temp = { { identifier }, nullptr, (int)data.flags() };
    Transition &t = lookupOrInsertTransition(temp);

    if (index)
        *index = size;

    if (t.lookup)
        return t.lookup;

    // create a new class and add it to the tree
    InternalClass *newClass = engine->newClass(this);
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

void InternalClass::removeMember(Object *object, Identifier *id)
{
    InternalClass *oldClass = object->internalClass();
    Q_ASSERT(oldClass->propertyTable.lookup(id) < oldClass->size);

    Transition temp = { { id }, nullptr, Transition::RemoveMember };
    Transition &t = object->internalClass()->lookupOrInsertTransition(temp);

    if (!t.lookup) {
        // create a new class and add it to the tree
        InternalClass *newClass = oldClass->engine->newClass(oldClass);
        // simply make the entry inaccessible
        int idx = newClass->propertyTable.removeIdentifier(id, oldClass->size);
        newClass->nameMap.set(idx, nullptr);
        newClass->propertyData.set(idx, PropertyAttributes());
        t.lookup = newClass;
        Q_ASSERT(t.lookup);
    }
    object->setInternalClass(t.lookup);

    // we didn't remove the data slot, just made it inaccessible
    Q_ASSERT(object->internalClass()->size == oldClass->size);
}

uint InternalClass::find(const String *string)
{
    engine->identifierTable->identifier(string);
    const Identifier *id = string->d()->identifier;

    uint index = propertyTable.lookup(id);
    if (index < size)
        return index;

    return UINT_MAX;
}

InternalClass *InternalClass::sealed()
{
    if (m_sealed)
        return m_sealed;

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
        m_sealed = this;
        return this;
    }

    m_sealed = engine->newClass(this);

    for (uint i = 0; i < size; ++i) {
        PropertyAttributes attrs = propertyData.at(i);
        if (attrs.isEmpty())
            continue;
        attrs.setConfigurable(false);
        m_sealed->propertyData.set(i, attrs);
    }
    m_sealed->extensible = false;

    m_sealed->m_sealed = m_sealed;
    return m_sealed;
}

InternalClass *InternalClass::frozen()
{
    if (m_frozen)
        return m_frozen;

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
        m_frozen = this;
        m_sealed = this;
        return this;
    }

    m_frozen = engine->newClass(this);

    for (uint i = 0; i < size; ++i) {
        PropertyAttributes attrs = propertyData.at(i);
        if (attrs.isEmpty())
            continue;
        if (attrs.isData())
            attrs.setWritable(false);
        attrs.setConfigurable(false);
        m_frozen->propertyData.set(i, attrs);
    }
    m_frozen->extensible = false;

    m_frozen->m_frozen = m_frozen;
    m_frozen->m_sealed = m_frozen;
    return m_frozen;
}

InternalClass *InternalClass::propertiesFrozen() const
{
    InternalClass *frozen = engine->internalClasses[EngineBase::Class_Empty]->changeVTable(vtable);
    frozen = frozen->changePrototype(prototype);
    for (uint i = 0; i < size; ++i) {
        PropertyAttributes attrs = propertyData.at(i);
        if (attrs.isEmpty())
            continue;
        attrs.setWritable(false);
        attrs.setConfigurable(false);
        frozen = frozen->addMember(nameMap.at(i), attrs);
    }
    return frozen;
}

InternalClass *InternalClass::asProtoClass()
{
    if (isUsedAsProto)
        return this;

    Transition temp = { { nullptr }, nullptr, Transition::ProtoClass };
    Transition &t = lookupOrInsertTransition(temp);
    if (t.lookup)
        return t.lookup;

    InternalClass *newClass = engine->newClass(this);
    newClass->isUsedAsProto = true;

    t.lookup = newClass;
    Q_ASSERT(t.lookup);
    return newClass;
}

void InternalClass::destroy()
{
    std::vector<InternalClass *> destroyStack;
    destroyStack.reserve(64);
    destroyStack.push_back(this);

    while (!destroyStack.empty()) {
        InternalClass *next = destroyStack.back();
        destroyStack.pop_back();
        if (!next->engine)
            continue;
        next->engine = nullptr;
        next->propertyTable.~PropertyHash();
        next->nameMap.~SharedInternalClassData<Identifier *>();
        next->propertyData.~SharedInternalClassData<PropertyAttributes>();
        if (next->m_sealed)
            destroyStack.push_back(next->m_sealed);
        if (next->m_frozen)
            destroyStack.push_back(next->m_frozen);

        for (size_t i = 0; i < next->transitions.size(); ++i) {
            Q_ASSERT(next->transitions.at(i).lookup);
            destroyStack.push_back(next->transitions.at(i).lookup);
        }

        next->transitions.~vector<Transition>();
    }
}

static void updateProtoUsage(Heap::Object *o, InternalClass *ic)
{
    if (ic->prototype == o)
        ic->id = ic->engine->newInternalClassId();
    for (auto &t : ic->transitions) {
        Q_ASSERT(t.lookup);
        updateProtoUsage(o, t.lookup);
    }
}


void InternalClass::updateProtoUsage(Heap::Object *o)
{
    Q_ASSERT(isUsedAsProto);
    InternalClass *ic = engine->internalClasses[EngineBase::Class_Empty];
    Q_ASSERT(!ic->prototype);

    ::updateProtoUsage(o, ic);
}

void InternalClass::updateInternalClassIdRecursive()
{
    id = engine->newInternalClassId();
    for (auto &t : transitions) {
        Q_ASSERT(t.lookup);
        t.lookup->updateInternalClassIdRecursive();
    }
}


static void markChildren(MarkStack *markStack, InternalClass *ic)
{
    if (ic->prototype)
        ic->prototype->mark(markStack);

    for (auto &t : ic->transitions) {
        Q_ASSERT(t.lookup);
        markChildren(markStack, t.lookup);
    }
}


void InternalClassPool::markObjects(MarkStack *markStack)
{
    InternalClass *ic = markStack->engine->internalClasses[EngineBase::Class_Empty];
    Q_ASSERT(!ic->prototype);
    ::markChildren(markStack, ic);
}

QT_END_NAMESPACE
