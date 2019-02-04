/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "qstringhash_p.h"

QT_BEGIN_NAMESPACE

/*
    A QHash has initially around pow(2, MinNumBits) buckets. For
    example, if MinNumBits is 4, it has 17 buckets.
*/
static const int MinNumBits = 4;

/*
    The prime_deltas array is a table of selected prime values, even
    though it doesn't look like one. The primes we are using are 1,
    2, 5, 11, 17, 37, 67, 131, 257, ..., i.e. primes in the immediate
    surrounding of a power of two.

    The primeForNumBits() function returns the prime associated to a
    power of two. For example, primeForNumBits(8) returns 257.
*/

static const uchar prime_deltas[] = {
    0,  0,  1,  3,  1,  5,  3,  3,  1,  9,  7,  5,  3,  9, 25,  3,
    1, 21,  3, 21,  7, 15,  9,  5,  3, 29, 15,  0,  0,  0,  0,  0
};

static inline int primeForNumBits(int numBits)
{
    return (1 << numBits) + prime_deltas[numBits];
}

void QStringHashData::rehashToSize(int size)
{
    short bits = qMax(MinNumBits, (int)numBits);
    while (primeForNumBits(bits) < size) bits++;

    if (bits > numBits)
        rehashToBits(bits);
}

void QStringHashData::rehashToBits(short bits)
{
    numBits = qMax(MinNumBits, (int)bits);

    int nb = primeForNumBits(numBits);
    if (nb == numBuckets && buckets)
        return;

#ifdef QSTRINGHASH_LINK_DEBUG
    if (linkCount)
        qFatal("QStringHash: Illegal attempt to rehash a linked hash.");
#endif

    QStringHashNode **newBuckets = new QStringHashNode *[nb];
    ::memset(newBuckets, 0, sizeof(QStringHashNode *) * nb);

    // Preserve the existing order within buckets so that items with the
    // same key will retain the same find/findNext order
    for (int i = 0; i < numBuckets; ++i) {
        QStringHashNode *bucket = buckets[i];
        if (bucket)
            rehashNode(newBuckets, nb, bucket);
    }

    delete [] buckets;
    buckets = newBuckets;
    numBuckets = nb;
}

void QStringHashData::rehashNode(QStringHashNode **newBuckets, int nb, QStringHashNode *node)
{
    QStringHashNode *next = node->next.data();
    if (next)
        rehashNode(newBuckets, nb, next);

    int bucket = node->hash % nb;
    node->next = newBuckets[bucket];
    newBuckets[bucket] = node;
}

// Copy of QString's qMemCompare
bool QHashedString::compare(const QChar *lhs, const QChar *rhs, int length)
{
    Q_ASSERT(lhs && rhs);
    const quint16 *a = (const quint16 *)lhs;
    const quint16 *b = (const quint16 *)rhs;

    if (a == b || !length)
        return true;

    union {
        const quint16 *w;
        const quint32 *d;
        quintptr value;
    } sa, sb;
    sa.w = a;
    sb.w = b;

    // check alignment
    if ((sa.value & 2) == (sb.value & 2)) {
        // both addresses have the same alignment
        if (sa.value & 2) {
            // both addresses are not aligned to 4-bytes boundaries
            // compare the first character
            if (*sa.w != *sb.w)
                return false;
            --length;
            ++sa.w;
            ++sb.w;

            // now both addresses are 4-bytes aligned
        }

        // both addresses are 4-bytes aligned
        // do a fast 32-bit comparison
        const quint32 *e = sa.d + (length >> 1);
        for ( ; sa.d != e; ++sa.d, ++sb.d) {
            if (*sa.d != *sb.d)
                return false;
        }

        // do we have a tail?
        return (length & 1) ? *sa.w == *sb.w : true;
    } else {
        // one of the addresses isn't 4-byte aligned but the other is
        const quint16 *e = sa.w + length;
        for ( ; sa.w != e; ++sa.w, ++sb.w) {
            if (*sa.w != *sb.w)
                return false;
        }
    }
    return true;
}

QT_END_NAMESPACE
