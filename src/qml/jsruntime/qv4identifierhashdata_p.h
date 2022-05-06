/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/
#ifndef QV4IDENTIFIERHASHDATA_H
#define QV4IDENTIFIERHASHDATA_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qv4global_p.h>
#include <private/qv4propertykey_p.h>
#include <private/qv4identifiertable_p.h>
#include <QtCore/qatomic.h>

QT_BEGIN_NAMESPACE

namespace QV4 {

struct IdentifierHashEntry {
    PropertyKey identifier;
    int value;
};

struct IdentifierHashData
{
    IdentifierHashData(IdentifierTable *table, int numBits)
        : size(0)
        , numBits(numBits)
        , identifierTable(table)
    {
        refCount.storeRelaxed(1);
        alloc = qPrimeForNumBits(numBits);
        entries = (IdentifierHashEntry *)malloc(alloc*sizeof(IdentifierHashEntry));
        memset(entries, 0, alloc*sizeof(IdentifierHashEntry));
        identifierTable->addIdentifierHash(this);
    }

    explicit IdentifierHashData(IdentifierHashData *other)
        : size(other->size)
        , numBits(other->numBits)
        , identifierTable(other->identifierTable)
    {
        refCount.storeRelaxed(1);
        alloc = other->alloc;
        entries = (IdentifierHashEntry *)malloc(alloc*sizeof(IdentifierHashEntry));
        memcpy(entries, other->entries, alloc*sizeof(IdentifierHashEntry));
        identifierTable->addIdentifierHash(this);
    }

    ~IdentifierHashData() {
        free(entries);
        if (identifierTable)
            identifierTable->removeIdentifierHash(this);
    }

    void markObjects(MarkStack *markStack) const
    {
        IdentifierHashEntry *e = entries;
        IdentifierHashEntry *end = e + alloc;
        while (e < end) {
            if (Heap::Base *o = e->identifier.asStringOrSymbol())
                o->mark(markStack);
            ++e;
        }
    }

    QBasicAtomicInt refCount;
    int alloc;
    int size;
    int numBits;
    IdentifierTable *identifierTable;
    IdentifierHashEntry *entries;
};

} // namespace QV4

QT_END_NAMESPACE

#endif // QV4IDENTIFIERHASHDATA_P_H
