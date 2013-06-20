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

#include "qhashedstring_p.h"
#include <private/qcalculatehash_p.h>

inline quint32 stringHash(const QChar* data, int length)
{
    quint32 rv = calculateHash<quint16>((quint16*)data, length) >> HashedString::kHashShift;
    Q_ASSERT(rv == v8::String::ComputeHash((uint16_t*)data, length));
    return rv;
}

inline quint32 stringHash(const char *data, int length)
{
    quint32 rv = calculateHash<quint8>((quint8*)data, length) >> HashedString::kHashShift;
    Q_ASSERT(rv == v8::String::ComputeHash((char *)data, length));
    return rv;
}

void QHashedString::computeHash() const
{
    m_hash = stringHash(constData(), length());
}

void QHashedStringRef::computeHash() const
{
    m_hash = stringHash(m_data, m_length);
}

void QHashedCStringRef::computeHash() const
{
    m_hash = stringHash(m_data, m_length);
}

/*
    A QHash has initially around pow(2, MinNumBits) buckets. For
    example, if MinNumBits is 4, it has 17 buckets.
*/
const int MinNumBits = 4;

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

// Unicode stuff
static inline bool isUnicodeNonCharacter(uint ucs4)
{
    // Unicode has a couple of "non-characters" that one can use internally,
    // but are not allowed to be used for text interchange.
    //
    // Those are the last two entries each Unicode Plane (U+FFFE, U+FFFF,
    // U+1FFFE, U+1FFFF, etc.) as well as the entries between U+FDD0 and
    // U+FDEF (inclusive)

    return (ucs4 & 0xfffe) == 0xfffe
            || (ucs4 - 0xfdd0U) < 16;
}

static int utf8LengthFromUtf16(const QChar *uc, int len)
{
    int length = 0;

    int surrogate_high = -1;

    const QChar *ch = uc;
    int invalid = 0;

    const QChar *end = ch + len;
    while (ch < end) {
        uint u = ch->unicode();
        if (surrogate_high >= 0) {
            if (u >= 0xdc00 && u < 0xe000) {
                u = (surrogate_high - 0xd800)*0x400 + (u - 0xdc00) + 0x10000;
                surrogate_high = -1;
            } else {
                // high surrogate without low
                ++ch;
                ++invalid;
                surrogate_high = -1;
                continue;
            }
        } else if (u >= 0xdc00 && u < 0xe000) {
            // low surrogate without high
            ++ch;
            ++invalid;
            continue;
        } else if (u >= 0xd800 && u < 0xdc00) {
            surrogate_high = u;
            ++ch;
            continue;
        }

        if (u < 0x80) {
            ++length;
        } else {
            if (u < 0x0800) {
                ++length;
            } else {
                // is it one of the Unicode non-characters?
                if (isUnicodeNonCharacter(u)) {
                    ++length;
                    ++ch;
                    ++invalid;
                    continue;
                }

                if (u > 0xffff) {
                    ++length;
                    ++length;
                } else {
                    ++length;
                }
                ++length;
            }
            ++length;
        }
        ++ch;
    }

    return length;
}

// Writes the utf8 version of uc to output.  uc is of length len.
// There must be at least utf8LengthFromUtf16(uc, len) bytes in output.
// A null terminator is not written.
static void utf8FromUtf16(char *output, const QChar *uc, int len)
{
    uchar replacement = '?';
    int surrogate_high = -1;

    uchar* cursor = (uchar*)output;
    const QChar *ch = uc;
    int invalid = 0;

    const QChar *end = ch + len;
    while (ch < end) {
        uint u = ch->unicode();
        if (surrogate_high >= 0) {
            if (u >= 0xdc00 && u < 0xe000) {
                u = (surrogate_high - 0xd800)*0x400 + (u - 0xdc00) + 0x10000;
                surrogate_high = -1;
            } else {
                // high surrogate without low
                *cursor = replacement;
                ++ch;
                ++invalid;
                surrogate_high = -1;
                continue;
            }
        } else if (u >= 0xdc00 && u < 0xe000) {
            // low surrogate without high
            *cursor = replacement;
            ++ch;
            ++invalid;
            continue;
        } else if (u >= 0xd800 && u < 0xdc00) {
            surrogate_high = u;
            ++ch;
            continue;
        }

        if (u < 0x80) {
            *cursor++ = (uchar)u;
        } else {
            if (u < 0x0800) {
                *cursor++ = 0xc0 | ((uchar) (u >> 6));
            } else {
                // is it one of the Unicode non-characters?
                if (isUnicodeNonCharacter(u)) {
                    *cursor++ = replacement;
                    ++ch;
                    ++invalid;
                    continue;
                }

                if (u > 0xffff) {
                    *cursor++ = 0xf0 | ((uchar) (u >> 18));
                    *cursor++ = 0x80 | (((uchar) (u >> 12)) & 0x3f);
                } else {
                    *cursor++ = 0xe0 | (((uchar) (u >> 12)) & 0x3f);
                }
                *cursor++ = 0x80 | (((uchar) (u >> 6)) & 0x3f);
            }
            *cursor++ = 0x80 | ((uchar) (u&0x3f));
        }
        ++ch;
    }
}

void QHashedStringRef::computeUtf8Length() const
{
    if (m_length) 
        m_utf8length = utf8LengthFromUtf16(m_data, m_length);
    else
        m_utf8length = 0;
}

QHashedStringRef QHashedStringRef::mid(int offset, int length) const
{
    Q_ASSERT(offset < m_length);
    return QHashedStringRef(m_data + offset, 
                            (length == -1 || (offset + length) > m_length)?(m_length - offset):length);
}

bool QHashedStringRef::endsWith(const QString &s) const
{
    return s.length() < m_length && 
           QHashedString::compare(s.constData(), m_data + m_length - s.length(), s.length());
}

bool QHashedStringRef::startsWith(const QString &s) const
{
    return s.length() < m_length && 
           QHashedString::compare(s.constData(), m_data, s.length());
}

static int findChar(const QChar *str, int len, QChar ch, int from)
{
    const ushort *s = (const ushort *)str;
    ushort c = ch.unicode();
    if (from < 0)
        from = qMax(from + len, 0);
    if (from < len) {
        const ushort *n = s + from - 1;
        const ushort *e = s + len;
        while (++n != e)
            if (*n == c)
                return  n - s;
    }
    return -1;
}

int QHashedStringRef::indexOf(const QChar &c, int from) const
{
    return findChar(m_data, m_length, c, from);
}

QString QHashedStringRef::toString() const
{
    if (m_length == 0)
        return QString();
    return QString(m_data, m_length);
}

QByteArray QHashedStringRef::toUtf8() const
{
    if (m_length == 0)
        return QByteArray();

    QByteArray result;
    result.resize(utf8length());
    writeUtf8(result.data());
    return result;
}

void QHashedStringRef::writeUtf8(char *output) const
{
    if (m_length) {
        int ulen = utf8length();
        if (ulen == m_length) {
            // Must be a latin1 string
            uchar *o = (uchar *)output;
            const QChar *c = m_data;
            while (ulen--) 
                *o++ = (uchar)((*c++).unicode());
        } else {
            utf8FromUtf16(output, m_data, m_length);
        }
    }
}

QString QHashedCStringRef::toUtf16() const
{
    if (m_length == 0)
        return QString();

    QString rv;
    rv.resize(m_length);
    writeUtf16((uint16_t*)rv.data());
    return rv;
}

