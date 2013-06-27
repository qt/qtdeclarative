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
#ifndef QV4IDENTIFIER_H
#define QV4IDENTIFIER_H

#include "qv4string_p.h"
#include "qv4engine_p.h"
#include <limits.h>

QT_BEGIN_NAMESPACE

namespace QV4 {

struct IdentifierTable;

struct Identifier
{
    QString string;
    uint hashValue;
};

inline uint hash(const QV4::Identifier *id)
{
    quintptr h = (quintptr)id;
    if (sizeof(quintptr) == sizeof(uint))
        return h ^ (h >> 8);
    else
        return (uint)(h ^ (h >> 8) ^ (h >> 32));
}



struct IdentifierIntHashData;
struct IdentifierIntHash
{
    struct Entry {
        const Identifier *identifier;
        int value;
    };

    IdentifierIntHashData *d;

    IdentifierIntHash() : d(0) {}
    IdentifierIntHash(ExecutionEngine *engine);
    inline IdentifierIntHash(const IdentifierIntHash &other);
    inline ~IdentifierIntHash();
    inline IdentifierIntHash &operator=(const IdentifierIntHash &other);

    bool isEmpty() const { return !d; }
    // ###
    void reserve(int) {}

    inline int count() const;
    void add(const QString &str, int value);

    int value(const QString &);
    int value(String *str);

    QString findId(int value) const;

private:
    void addEntry(const Identifier *i, uint value);
    int lookup(const Identifier *identifier) const;
};

struct IdentifierIntHashData
{
    IdentifierIntHashData(int numBits);
    ~IdentifierIntHashData() {
        free(entries);
    }

    QBasicAtomicInt refCount;
    int alloc;
    int size;
    int numBits;
    IdentifierTable *identifierTable;
    IdentifierIntHash::Entry *entries;
};

inline IdentifierIntHash::IdentifierIntHash(const IdentifierIntHash &other)
{
    d = other.d;
    if (d)
        d->refCount.ref();
}

inline IdentifierIntHash::~IdentifierIntHash()
{
    if (d && !d->refCount.deref())
        delete d;
}

IdentifierIntHash &IdentifierIntHash::operator=(const IdentifierIntHash &other)
{
    if (other.d)
        other.d->refCount.ref();
    if (d && !d->refCount.deref())
        delete d;
    d = other.d;
    return *this;
}

inline int IdentifierIntHash::count() const
{
    return d ? d->size : 0;
}


struct IdentifierTable
{
    ExecutionEngine *engine;

    int alloc;
    int size;
    int numBits;
    String **entries;

    void addEntry(String *str);

public:

    IdentifierTable(ExecutionEngine *engine);
    ~IdentifierTable();

    String *insertString(const QString &s);

    Identifier *identifier(String *str);
    Identifier *identifier(const QString &s);
    Identifier *identifier(const char *s, int len);

    void mark() {
        for (int i = 0; i < alloc; ++i)
            if (entries[i])
                entries[i]->mark();
    }
};

}

QT_END_NAMESPACE

#endif
