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

#include <private/qv4identifierhash_p.h>
#include <private/qv4identifiertable_p.h>
#include <private/qv4string_p.h>
#include <private/qv4identifierhashdata_p.h>
#include <private/qprimefornumbits_p.h>

QT_BEGIN_NAMESPACE

namespace QV4 {

IdentifierHash::IdentifierHash(ExecutionEngine *engine)
{
    d = new IdentifierHashData(engine->identifierTable, 3);
    Q_ASSERT(!isEmpty());
}

void IdentifierHash::detach()
{
    if (!d || d->refCount.loadAcquire() == 1)
        return;
    IdentifierHashData *newData = new IdentifierHashData(d);
    if (d && !d->refCount.deref())
        delete d;
    d = newData;
}

inline
IdentifierHashEntry *IdentifierHash::addEntry(PropertyKey identifier)
{
    Q_ASSERT(identifier.isStringOrSymbol());

    // fill up to max 50%
    bool grow = (d->alloc <= d->size*2);

    if (grow) {
        ++d->numBits;
        int newAlloc = qPrimeForNumBits(d->numBits);
        IdentifierHashEntry *newEntries = (IdentifierHashEntry *)malloc(newAlloc * sizeof(IdentifierHashEntry));
        memset(newEntries, 0, newAlloc*sizeof(IdentifierHashEntry));
        for (int i = 0; i < d->alloc; ++i) {
            const IdentifierHashEntry &e = d->entries[i];
            if (!e.identifier.isValid())
                continue;
            uint idx = e.identifier.id() % newAlloc;
            while (newEntries[idx].identifier.isValid()) {
                ++idx;
                idx %= newAlloc;
            }
            newEntries[idx] = e;
        }
        free(d->entries);
        d->entries = newEntries;
        d->alloc = newAlloc;
    }

    uint idx = identifier.id() % d->alloc;
    while (d->entries[idx].identifier.isValid()) {
        Q_ASSERT(d->entries[idx].identifier != identifier);
        ++idx;
        idx %= d->alloc;
    }
    d->entries[idx].identifier = identifier;
    ++d->size;
    return d->entries + idx;
}

inline
const IdentifierHashEntry *IdentifierHash::lookup(PropertyKey identifier) const
{
    if (!d || !identifier.isStringOrSymbol())
        return nullptr;
    Q_ASSERT(d->entries);

    uint idx = identifier.id() % d->alloc;
    while (1) {
        if (!d->entries[idx].identifier.isValid())
            return nullptr;
        if (d->entries[idx].identifier == identifier)
            return d->entries + idx;
        ++idx;
        idx %= d->alloc;
    }
}

inline
const IdentifierHashEntry *IdentifierHash::lookup(const QString &str) const
{
    if (!d)
        return nullptr;

    PropertyKey id = d->identifierTable->asPropertyKey(str);
    return lookup(id);
}

inline
const IdentifierHashEntry *IdentifierHash::lookup(String *str) const
{
    if (!d)
        return nullptr;
    PropertyKey id = d->identifierTable->asPropertyKey(str);
    if (id.isValid())
        return lookup(id);
    return lookup(str->toQString());
}

inline
const PropertyKey IdentifierHash::toIdentifier(const QString &str) const
{
    Q_ASSERT(d);
    return d->identifierTable->asPropertyKey(str);
}

inline
const PropertyKey IdentifierHash::toIdentifier(Heap::String *str) const
{
    Q_ASSERT(d);
    return d->identifierTable->asPropertyKey(str);
}

QString QV4::IdentifierHash::findId(int value) const
{
    IdentifierHashEntry *e = d->entries;
    IdentifierHashEntry *end = e + d->alloc;
    while (e < end) {
        if (e->identifier.isValid() && e->value == value)
            return e->identifier.toQString();
        ++e;
    }
    return QString();
}

QV4::IdentifierHash::IdentifierHash(const IdentifierHash &other)
{
    d = other.d;
    if (d)
        d->refCount.ref();
}

QV4::IdentifierHash::~IdentifierHash()
{
    if (d && !d->refCount.deref())
        delete d;
}

IdentifierHash &QV4::IdentifierHash::operator=(const IdentifierHash &other)
{
    if (other.d)
        other.d->refCount.ref();
    if (d && !d->refCount.deref())
        delete d;
    d = other.d;
    return *this;
}

int QV4::IdentifierHash::count() const
{
    return d ? d->size : 0;
}

void QV4::IdentifierHash::add(const QString &str, int value)
{
    IdentifierHashEntry *e = addEntry(toIdentifier(str));
    e->value = value;
}

void QV4::IdentifierHash::add(Heap::String *str, int value)
{
    IdentifierHashEntry *e = addEntry(toIdentifier(str));
    e->value = value;
}

int QV4::IdentifierHash::value(const QString &str) const
{
    const IdentifierHashEntry *e = lookup(str);
    return e ? e->value : -1;
}

int QV4::IdentifierHash::value(String *str) const
{
    const IdentifierHashEntry *e = lookup(str);
    return e ? e->value : -1;
}


}

QT_END_NAMESPACE
