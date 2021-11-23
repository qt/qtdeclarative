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

#include "birthdayparty.h"

BirthdayParty::BirthdayParty(QObject *parent)
    : QObject(parent), m_host(nullptr)
{
}

Person *BirthdayParty::host() const
{
    return m_host;
}

void BirthdayParty::setHost(Person *c)
{
    if (c != m_host) {
        m_host = c;
        emit hostChanged();
    }
}

QQmlListProperty<Person> BirthdayParty::guests()
{
    return {this, &m_guests};
}

int BirthdayParty::guestCount() const
{
    return m_guests.count();
}

Person *BirthdayParty::guest(int index) const
{
    return m_guests.at(index);
}

void BirthdayParty::invite(const QString &name)
{
    auto *person = new Person(this);
    person->setName(name);
    m_guests.append(person);
}

BirthdayPartyAttached *BirthdayParty::qmlAttachedProperties(QObject *object)
{
    return new BirthdayPartyAttached(object);
}

BirthdayPartyAttached::BirthdayPartyAttached(QObject *parent)
    : QObject(parent), m_rsvp(QDateTime(QDate(1911, 3, 4), QTime()))
{
}

QDateTime BirthdayPartyAttached::rsvp() const
{
    return m_rsvp.value();
}

void BirthdayPartyAttached::setRsvp(const QDateTime &rsvp)
{
    m_rsvp.setValue(rsvp);
}

QBindable<QDateTime> BirthdayPartyAttached::rsvpBindable()
{
    return QBindable<QDateTime>(&m_rsvp);
}
