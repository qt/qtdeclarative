// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant
#include "qv4identifiertable_p.h"
#include "qv4symbol_p.h"
#include <private/qv4identifierhashdata_p.h>
#include <private/qprimefornumbits_p.h>

QT_BEGIN_NAMESPACE

namespace QV4 {

IdentifierTable::IdentifierTable(ExecutionEngine *engine, int numBits)
    : engine(engine)
    , size(0)
    , numBits(numBits)
{
    alloc = qPrimeForNumBits(numBits);
    entriesByHash = (Heap::StringOrSymbol **)malloc(alloc*sizeof(Heap::StringOrSymbol *));
    entriesById = (Heap::StringOrSymbol **)malloc(alloc*sizeof(Heap::StringOrSymbol *));
    memset(entriesByHash, 0, alloc*sizeof(Heap::String *));
    memset(entriesById, 0, alloc*sizeof(Heap::String *));
}

IdentifierTable::~IdentifierTable()
{
    free(entriesByHash);
    free(entriesById);
    for (const auto &h : std::as_const(idHashes))
        h->identifierTable = nullptr;
}

void IdentifierTable::addEntry(Heap::StringOrSymbol *str)
{
    uint hash = str->hashValue();

    if (str->subtype == Heap::String::StringType_ArrayIndex)
        return;

    str->identifier = PropertyKey::fromStringOrSymbol(engine, str);

    bool grow = (alloc <= size*2);

    if (grow) {
        ++numBits;
        int newAlloc = qPrimeForNumBits(numBits);
        Heap::StringOrSymbol **newEntries = (Heap::StringOrSymbol **)malloc(newAlloc*sizeof(Heap::String *));
        memset(newEntries, 0, newAlloc*sizeof(Heap::StringOrSymbol *));
        for (uint i = 0; i < alloc; ++i) {
            Heap::StringOrSymbol *e = entriesByHash[i];
            if (!e)
                continue;
            uint idx = e->stringHash % newAlloc;
            while (newEntries[idx]) {
                ++idx;
                idx %= newAlloc;
            }
            newEntries[idx] = e;
        }
        free(entriesByHash);
        entriesByHash = newEntries;

        newEntries = (Heap::StringOrSymbol **)malloc(newAlloc*sizeof(Heap::String *));
        memset(newEntries, 0, newAlloc*sizeof(Heap::StringOrSymbol *));
        for (uint i = 0; i < alloc; ++i) {
            Heap::StringOrSymbol *e = entriesById[i];
            if (!e)
                continue;
            uint idx = e->identifier.id() % newAlloc;
            while (newEntries[idx]) {
                ++idx;
                idx %= newAlloc;
            }
            newEntries[idx] = e;
        }
        free(entriesById);
        entriesById = newEntries;

        alloc = newAlloc;
    }

    uint idx = hash % alloc;
    while (entriesByHash[idx]) {
        ++idx;
        idx %= alloc;
    }
    entriesByHash[idx] = str;

    idx = str->identifier.id() % alloc;
    while (entriesById[idx]) {
        ++idx;
        idx %= alloc;
    }
    entriesById[idx] = str;

    ++size;
}



Heap::String *IdentifierTable::insertString(const QString &s)
{
    uint subtype;
    uint hash = String::createHashValue(s.constData(), s.size(), &subtype);
    if (subtype == Heap::String::StringType_ArrayIndex) {
        Heap::String *str = engine->newString(s);
        str->stringHash = hash;
        str->subtype = subtype;
        str->identifier = PropertyKey::fromArrayIndex(hash);
        return str;
    }
    return resolveStringEntry(s, hash, subtype);
}

Heap::String *IdentifierTable::resolveStringEntry(const QString &s, uint hash, uint subtype)
{
    uint idx = hash % alloc;
    while (Heap::StringOrSymbol *e = entriesByHash[idx]) {
        if (e->stringHash == hash && e->toQString() == s)
            return static_cast<Heap::String *>(e);
        ++idx;
        idx %= alloc;
    }

    Heap::String *str = engine->newString(s);
    str->stringHash = hash;
    str->subtype = subtype;
    addEntry(str);
    return str;
}

Heap::Symbol *IdentifierTable::insertSymbol(const QString &s)
{
    Q_ASSERT(s.at(0) == QLatin1Char('@'));

    uint subtype;
    uint hash = String::createHashValue(s.constData(), s.size(), &subtype);
    uint idx = hash % alloc;
    while (Heap::StringOrSymbol *e = entriesByHash[idx]) {
        if (e->stringHash == hash && e->toQString() == s)
            return static_cast<Heap::Symbol *>(e);
        ++idx;
        idx %= alloc;
    }

    Heap::Symbol *str = Symbol::create(engine, s);
    str->stringHash = hash;
    str->subtype = subtype;
    addEntry(str);
    return str;

}


PropertyKey IdentifierTable::asPropertyKeyImpl(const Heap::String *str)
{
    if (str->identifier.isValid())
        return str->identifier;
    uint hash = str->hashValue();
    if (str->subtype == Heap::String::StringType_ArrayIndex) {
        str->identifier = PropertyKey::fromArrayIndex(hash);
        return str->identifier;
    }

    uint idx = hash % alloc;
    while (Heap::StringOrSymbol *e = entriesByHash[idx]) {
        if (e->stringHash == hash && e->toQString() == str->toQString()) {
            str->identifier = e->identifier;
            QV4::WriteBarrier::markCustom(engine, [&](QV4::MarkStack *stack) {
                e->identifier.asStringOrSymbol()->mark(stack);
            });
            return e->identifier;
        }
        ++idx;
        idx %= alloc;
    }

    addEntry(const_cast<QV4::Heap::String *>(str));
    return str->identifier;
}

Heap::StringOrSymbol *IdentifierTable::resolveId(PropertyKey i) const
{
    if (i.isArrayIndex())
        return engine->newString(QString::number(i.asArrayIndex()));
    if (!i.isValid())
        return nullptr;

    uint idx = i.id() % alloc;
    while (1) {
        Heap::StringOrSymbol *e = entriesById[idx];
        if (!e || e->identifier == i)
            return e;
        ++idx;
        idx %= alloc;
    }
}

Heap::String *IdentifierTable::stringForId(PropertyKey i) const
{
    Heap::StringOrSymbol *s = resolveId(i);
    Q_ASSERT(s && s->internalClass->vtable->isString);
    return static_cast<Heap::String *>(s);
}

Heap::Symbol *IdentifierTable::symbolForId(PropertyKey i) const
{
    Heap::StringOrSymbol *s = resolveId(i);
    Q_ASSERT(!s || !s->internalClass->vtable->isString);
    return static_cast<Heap::Symbol *>(s);
}

void IdentifierTable::markObjects(MarkStack *markStack)
{
    for (const auto &h : idHashes)
        h->markObjects(markStack);
}

void IdentifierTable::sweep()
{
    int freed = 0;

    Heap::StringOrSymbol **newTable = (Heap::StringOrSymbol **)malloc(alloc*sizeof(Heap::String *));
    memset(newTable, 0, alloc*sizeof(Heap::StringOrSymbol *));
    memset(entriesById, 0, alloc*sizeof(Heap::StringOrSymbol *));
    for (uint i = 0; i < alloc; ++i) {
        Heap::StringOrSymbol *e = entriesByHash[i];
        if (!e)
            continue;
        if (!e->isMarked()) {
            ++freed;
            continue;
        }
        uint idx = e->hashValue() % alloc;
        while (newTable[idx]) {
            ++idx;
            if (idx == alloc)
                idx = 0;
        }
        newTable[idx] = e;

        idx = e->identifier.id() % alloc;
        while (entriesById[idx]) {
            ++idx;
            if (idx == alloc)
                idx = 0;
        }
        entriesById[idx] = e;
    }
    free(entriesByHash);
    entriesByHash = newTable;

    size -= freed;
}

PropertyKey IdentifierTable::asPropertyKey(const QString &s,
                                           IdentifierTable::KeyConversionBehavior conversionBehvior)
{
    uint subtype;
    uint hash = String::createHashValue(s.constData(), s.size(), &subtype);
    if (subtype == Heap::String::StringType_ArrayIndex) {
        if (Q_UNLIKELY(conversionBehvior == ForceConversionToId))
            hash = String::createHashValueDisallowingArrayIndex(s.constData(), s.size(), &subtype);
        else
            return PropertyKey::fromArrayIndex(hash);
    }
    return resolveStringEntry(s, hash, subtype)->identifier;
}

}

QT_END_NAMESPACE
