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

#include <qv4internalclass_p.h>
#include <qv4string_p.h>
#include <qv4engine_p.h>
#include <qv4identifier_p.h>
#include "qv4object_p.h"

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
    : refCount(Q_BASIC_ATOMIC_INITIALIZER(1))
    , numBits(numBits)
    , size(0)
{
    alloc = primeForNumBits(numBits);
    entries = (PropertyHash::Entry *)malloc(alloc*sizeof(PropertyHash::Entry));
    for (uint i = 0; i < alloc; ++i) {
        entries[i].identifier = UINT_MAX;
        entries[i].index = UINT_MAX;
    }
}

void PropertyHash::addEntry(const PropertyHash::Entry &entry, int classSize)
{
    // fill up to max 50%
    bool grow = (d->alloc <= d->size*2);

    if (classSize < d->size || grow) {
        PropertyHashData *dd = new PropertyHashData(grow ? d->numBits + 1 : d->numBits);
        for (uint i = 0; i < d->alloc; ++i) {
            const Entry &e = d->entries[i];
            if (e.identifier == UINT_MAX || e.index >= classSize)
                continue;
            uint idx = e.identifier % dd->alloc;
            while (dd->entries[idx].identifier != UINT_MAX) {
                ++idx;
                idx %= dd->alloc;
            }
            dd->entries[idx] = e;
        }
        dd->size = classSize;
        assert(d->refCount.load() > 1);
        d->refCount.deref();
        d = dd;
    }

    uint idx = entry.identifier % d->alloc;
    while (d->entries[idx].identifier != UINT_MAX) {
        ++idx;
        idx %= d->alloc;
    }
    d->entries[idx] = entry;
    ++d->size;
}

uint PropertyHash::lookup(uint identifier) const
{
    assert(d->entries);

    uint idx = identifier % d->alloc;
    while (1) {
        if (d->entries[idx].identifier == identifier)
            return d->entries[idx].index;
        if (d->entries[idx].identifier == UINT_MAX)
            return UINT_MAX;
        ++idx;
        idx %= d->alloc;
    }
}


InternalClass::InternalClass(const QV4::InternalClass &other)
    : engine(other.engine)
    , propertyTable(other.propertyTable)
    , nameMap(other.nameMap)
    , propertyData(other.propertyData)
    , transitions()
    , m_sealed(0)
    , m_frozen(0)
    , size(other.size)
{
}

// ### Should we build this up from the empty class to avoid duplication?
InternalClass *InternalClass::changeMember(String *string, PropertyAttributes data, uint *index)
{
//    qDebug() << "InternalClass::changeMember()" << string->toQString() << hex << (uint)data.m_all;
    data.resolve();
    uint idx = find(string);
    if (index)
        *index = idx;

    assert(idx != UINT_MAX);

    if (data == propertyData[idx])
        return this;

    uint tid = string->identifier | (data.flags() << 27);

    QHash<int, InternalClass *>::const_iterator tit = transitions.constFind(tid);
    if (tit != transitions.constEnd())
        return tit.value();

    // create a new class and add it to the tree
    InternalClass *newClass = engine->newClass(*this);
    newClass->propertyData[idx] = data;
    return newClass;

}

InternalClass *InternalClass::addMember(String *string, PropertyAttributes data, uint *index)
{
//    qDebug() << "InternalClass::addMember()" << string->toQString() << size << hex << (uint)data.m_all << data.type();
    data.resolve();
    engine->identifierCache->toIdentifier(string);

    if (propertyTable.lookup(string->identifier) < size)
        return changeMember(string, data, index);

    uint id = string->identifier | (data.flags() << 27);
    QHash<int, InternalClass *>::const_iterator tit = transitions.constFind(id);

    if (index)
        *index = size;
    if (tit != transitions.constEnd())
        return tit.value();

    // create a new class and add it to the tree
    InternalClass *newClass = engine->newClass(*this);
    PropertyHash::Entry e = { string->identifier, size };
    newClass->propertyTable.addEntry(e, size);

    // The incoming string can come from anywhere, so make sure to
    // store a string in the nameMap that's guaranteed to get
    // marked properly during GC.
    String *name = engine->newIdentifier(string->toQString());
    newClass->nameMap.append(name);

    newClass->propertyData.append(data);
    ++newClass->size;
    transitions.insert(id, newClass);
    return newClass;
}

void InternalClass::removeMember(Object *object, uint id)
{
    int propIdx = propertyTable.lookup(id);
    assert(propIdx < size);

    int toRemove = - (int)id;
    QHash<int, InternalClass *>::const_iterator tit = transitions.constFind(toRemove);

    if (tit != transitions.constEnd()) {
        object->internalClass = tit.value();
        return;
    }

    // create a new class and add it to the tree
    object->internalClass = engine->emptyClass;
    for (int i = 0; i < nameMap.size(); ++i) {
        if (i == propIdx)
            continue;
        object->internalClass = object->internalClass->addMember(nameMap.at(i), propertyData.at(i));
    }

    transitions.insert(toRemove, object->internalClass);
}

uint InternalClass::find(String *string)
{
    engine->identifierCache->toIdentifier(string);
    uint id = string->identifier;

    uint index = propertyTable.lookup(id);
    if (index < size)
        return index;

    return UINT_MAX;
}

InternalClass *InternalClass::sealed()
{
    if (m_sealed)
        return m_sealed;

    m_sealed = engine->emptyClass;
    for (int i = 0; i < nameMap.size(); ++i) {
        PropertyAttributes attrs = propertyData.at(i);
        attrs.setConfigurable(false);
        m_sealed = m_sealed->addMember(nameMap.at(i), attrs);
    }

    m_sealed->m_sealed = m_sealed;
    return m_sealed;
}

InternalClass *InternalClass::frozen()
{
    if (m_frozen)
        return m_frozen;

    m_frozen = engine->emptyClass;
    for (int i = 0; i < nameMap.size(); ++i) {
        PropertyAttributes attrs = propertyData.at(i);
        attrs.setWritable(false);
        attrs.setConfigurable(false);
        m_frozen = m_frozen->addMember(nameMap.at(i), attrs);
    }

    m_frozen->m_frozen = m_frozen;
    return m_frozen;
}

void InternalClass::destroy()
{
    if (!engine)
        return;
    engine = 0;

    // Free the memory of the hashes/vectors by calling clear(), which
    // re-assigns them to the shared null instance. Therefore Internalclass
    // doesn't need a destructor to be called.
    propertyTable.~PropertyHash();
    nameMap.clear();
    propertyData.clear();

    if (m_sealed)
        m_sealed->destroy();

    if (m_frozen)
        m_frozen->destroy();

    for (QHash<int, InternalClass *>::ConstIterator it = transitions.begin(), end = transitions.end();
         it != end; ++it)
        it.value()->destroy();

    transitions.clear();
}

QT_END_NAMESPACE
