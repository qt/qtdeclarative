/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "person.h"

Person::Person(QObject *parent)
    : QObject(parent), m_name(u"Bart"_qs), m_shoeSize(0)
{
    m_things.append(u"thing"_qs);
    m_things.append(30);
}

QString Person::name() const
{
    return m_name;
}

void Person::setName(const QString &n)
{
    if (n != m_name) {
        m_name = n;
        emit nameChanged();
    }
}

void Person::resetName()
{
    setName(u"Bart"_qs);
}

int Person::shoeSize() const
{
    return m_shoeSize;
}

void Person::setShoeSize(int s)
{
    if (s != m_shoeSize) {
        m_shoeSize = s;
        emit shoeSizeChanged();
    }
}

QVariantList Person::things() const
{
    return m_things;
}

void Person::setThings(const QVariantList &things)
{
    if (m_things == things)
        return;
    m_things = things;
    emit thingsChanged();
}
