/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qhashedstring_p.h"

// This is a reimplementation of V8's string hash algorithm.  It is significantly
// faster to do it here than call into V8, but it adds the maintainence burden of
// ensuring that the two hashes are identical.  We Q_ASSERT() that the two return
// the same value.  If these asserts start to fail, the hash code needs to be 
// synced with V8.
namespace String {
    static const int kMaxArrayIndexSize = 10;
    static const int kMaxHashCalcLength = 16383;
    static const int kNofHashBitFields = 2;
    static const int kHashShift = kNofHashBitFields;
    static const int kIsNotArrayIndexMask = 1 << 1;
    static const int kArrayIndexValueBits = 24;
    static const int kArrayIndexHashLengthShift = kArrayIndexValueBits + kNofHashBitFields;
    static const int kMaxCachedArrayIndexLength = 7;
};

template <typename schar>
uint32_t calculateHash(const schar* chars, int length) {
    if (length > String::kMaxHashCalcLength) {
        // V8 trivial hash
        return (length << String::kHashShift) | String::kIsNotArrayIndexMask;
    }

    uint32_t raw_running_hash = 0;
    uint32_t array_index = 0;
    bool is_array_index = (0 < length && length <= String::kMaxArrayIndexSize);
    bool is_first_char = true;

    int ii = 0;
    for (;is_array_index && ii < length; ++ii) {
        quint32 c = *chars++;

        raw_running_hash += c;
        raw_running_hash += (raw_running_hash << 10);
        raw_running_hash ^= (raw_running_hash >> 6);

        if (c < '0' || c > '9') {
            is_array_index = false;
        } else {
            int d = c - '0';
            if (is_first_char) {
                is_first_char = false;
                if (c == '0' && length > 1) {
                    is_array_index = false;
                    continue;
                }
            }
            if (array_index > 429496729U - ((d + 2) >> 3)) {
                is_array_index = false;
            } else {
                array_index = array_index * 10 + d;
            }
        }
    }

    for (;ii < length; ++ii) {
        raw_running_hash += *chars++;
        raw_running_hash += (raw_running_hash << 10);
        raw_running_hash ^= (raw_running_hash >> 6);
    }

    if (is_array_index) {
        array_index <<= String::kHashShift;
        array_index |= length << String::kArrayIndexHashLengthShift;
        return array_index;
    } else {
        raw_running_hash += (raw_running_hash << 3);
        raw_running_hash ^= (raw_running_hash >> 11);
        raw_running_hash += (raw_running_hash << 15);
        if (raw_running_hash == 0) {
            raw_running_hash = 27;
        }

        return (raw_running_hash << String::kHashShift) | String::kIsNotArrayIndexMask;
    }
}

inline quint32 stringHash(const QChar* data, int length)
{
    quint32 rv = calculateHash<quint16>((quint16*)data, length) >> String::kHashShift;
    Q_ASSERT(rv == v8::String::ComputeHash((uint16_t*)data, length));
    return rv;
}

inline quint32 stringHash(const char *data, int length)
{
    quint32 rv = calculateHash<quint8>((quint8*)data, length) >> String::kHashShift;
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

void QStringHashData::rehashToSize(int size, IteratorData first,
                                   IteratorData (*Iterate)(const IteratorData &),
                                   QStringHashNode *skip)
{
    short bits = qMax(MinNumBits, (int)numBits);
    while (primeForNumBits(bits) < size) bits++;

    if (bits > numBits)
        rehashToBits(bits, first, Iterate, skip);
}

void QStringHashData::rehashToBits(short bits, IteratorData first,
                                   IteratorData (*Iterate)(const IteratorData &),
                                   QStringHashNode *skip)
{
    numBits = qMax(MinNumBits, (int)bits);

    int nb = primeForNumBits(numBits);
    if (nb == numBuckets && buckets)
        return;

    numBuckets = nb;

#ifdef QSTRINGHASH_LINK_DEBUG
    if (linkCount)
        qFatal("QStringHash: Illegal attempt to rehash a linked hash.");
#endif

    delete []  buckets;
    buckets = new QStringHashNode *[numBuckets]; 
    ::memset(buckets, 0, sizeof(QStringHashNode *) * numBuckets);

    IteratorData nodeList = first;
    while (nodeList.n) {
        if (nodeList.n != skip) {
            int bucket = nodeList.n->hash % numBuckets;
            nodeList.n->next = buckets[bucket];
            buckets[bucket] = nodeList.n;
        }

        nodeList = Iterate(nodeList);
    }
}

// Copy of QString's qMemCompare
bool QHashedString::compare(const QChar *lhs, const QChar *rhs, int length)
{
    Q_ASSERT(lhs && rhs);
    const quint16 *a = (const quint16 *)lhs;
    const quint16 *b = (const quint16 *)rhs;

    if (a == b || !length)
        return true;

    register union {
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
        register const quint32 *e = sa.d + (length >> 1);
        for ( ; sa.d != e; ++sa.d, ++sb.d) {
            if (*sa.d != *sb.d)
                return false;
        }

        // do we have a tail?
        return (length & 1) ? *sa.w == *sb.w : true;
    } else {
        // one of the addresses isn't 4-byte aligned but the other is
        register const quint16 *e = sa.w + length;
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

