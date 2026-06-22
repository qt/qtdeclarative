// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "person.h"

using namespace Qt::StringLiterals;

Person::Person(QObject *parent)
: QObject(parent), m_shoeSize(0)
{
    m_things.append(u"thing"_s);
    m_things.append(30);
}

QString Person::name() const
{
    return m_name;
}

void Person::setName(const QString &n)
{
    m_name = n;
}

int Person::shoeSize() const
{
    return m_shoeSize;
}

void Person::setShoeSize(int s)
{
    m_shoeSize = s;
}


QString Person::nick() const
{
    return m_nick;
}

void Person::setNick(const QString &nick)
{
    if (m_nick == nick)
        return;
    m_nick = nick;
    emit nickChanged();
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
