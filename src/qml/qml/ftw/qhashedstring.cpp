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

#include "qhashedstring_p.h"

QT_BEGIN_NAMESPACE

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

QHashedStringRef QHashedStringRef::mid(int offset, int length) const
{
    Q_ASSERT(offset < m_length);
    return QHashedStringRef(m_data + offset,
                            (length == -1 || (offset + length) > m_length)?(m_length - offset):length);
}

QVector<QHashedStringRef> QHashedStringRef::split(const QChar sep) const
{
    QVector<QHashedStringRef> ret;
    auto curLength = 0;
    auto curOffset = m_data;
    for (int offset = 0; offset < m_length; ++offset) {
        if (*(m_data + offset) == sep) {
            ret.push_back({curOffset, curLength});
            curOffset = m_data + offset + 1;
            curLength = 0;
        } else {
            ++curLength;
        }
    }
    if (curLength > 0)
        ret.push_back({curOffset, curLength});
    return ret;
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

QString QHashedCStringRef::toUtf16() const
{
    if (m_length == 0)
        return QString();

    QString rv;
    rv.resize(m_length);
    writeUtf16((quint16*)rv.data());
    return rv;
}

QT_END_NAMESPACE
