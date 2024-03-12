// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

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
    return m_guests.size();
}

Person *BirthdayParty::guest(int index) const
{
    return m_guests.at(index);
}

QStringList BirthdayParty::guestNames() const
{
    QStringList names;
    for (Person *guest: m_guests)
        names.append(guest->name());
    return names;
}

QVariantList BirthdayParty::stuffs() const
{
    return QVariantList({
            QVariant::fromValue(objectName()),
            QVariant::fromValue(m_guests.size()),
            QVariant::fromValue(m_host)
    });
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
