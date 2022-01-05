/******************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt JavaScript to C++ compiler.
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
******************************************************************************/

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
