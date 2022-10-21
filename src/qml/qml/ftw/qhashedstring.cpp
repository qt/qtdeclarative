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
    QStringView view {m_data, m_length};
    return view.endsWith(s);
}

bool QHashedStringRef::startsWith(const QString &s) const
{
    QStringView view {m_data, m_length};
    return view.startsWith(s);
}

int QHashedStringRef::indexOf(const QChar &c, int from) const
{
    QStringView view {m_data, m_length};
    return view.indexOf(c, from);
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
