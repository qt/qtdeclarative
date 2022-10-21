/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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
