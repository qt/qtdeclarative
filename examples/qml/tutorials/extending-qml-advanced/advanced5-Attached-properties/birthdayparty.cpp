// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "birthdayparty.h"

QDate BirthdayPartyAttached::rsvp() const
{
    return m_rsvp;
}

void BirthdayPartyAttached::setRsvp(QDate rsvpDate)
{
    if (m_rsvp != rsvpDate) {
        m_rsvp = rsvpDate;
        emit rsvpChanged();
    }
}

Person *BirthdayParty::host() const
{
    return m_host;
}

void BirthdayParty::setHost(Person *host)
{
    if (m_host != host) {
        m_host = host;
        emit hostChanged();
    }
}

QQmlListProperty<Person> BirthdayParty::guests()
{
    return { this,
             this,
             &BirthdayParty::appendGuest,
             &BirthdayParty::guestCount,
             &BirthdayParty::guest,
             &BirthdayParty::clearGuests,
             &BirthdayParty::replaceGuest,
             &BirthdayParty::removeLastGuest };
}

void BirthdayParty::appendGuest(Person *guest)
{
    m_guests.append(guest);
    emit guestsChanged();
}

qsizetype BirthdayParty::guestCount() const
{
    return m_guests.count();
}

Person *BirthdayParty::guest(qsizetype index) const
{
    return m_guests.at(index);
}

void BirthdayParty::clearGuests()
{
    if (!m_guests.empty()) {
        m_guests.clear();
        emit guestsChanged();
    }
}

void BirthdayParty::replaceGuest(qsizetype index, Person *guest)
{
    if (m_guests.size() > index) {
        m_guests[index] = guest;
        emit guestsChanged();
    }
}

void BirthdayParty::removeLastGuest()
{
    if (!m_guests.empty()) {
        m_guests.removeLast();
        emit guestsChanged();
    }
}

void BirthdayParty::appendGuest(QQmlListProperty<Person> *list, Person *guest)
{
    static_cast<BirthdayParty *>(list->data)->appendGuest(guest);
}

void BirthdayParty::clearGuests(QQmlListProperty<Person> *list)
{
    static_cast<BirthdayParty *>(list->data)->clearGuests();
}

void BirthdayParty::replaceGuest(QQmlListProperty<Person> *list, qsizetype index, Person *guest)
{
    static_cast<BirthdayParty *>(list->data)->replaceGuest(index, guest);
}

void BirthdayParty::removeLastGuest(QQmlListProperty<Person> *list)
{
    static_cast<BirthdayParty *>(list->data)->removeLastGuest();
}

Person *BirthdayParty::guest(QQmlListProperty<Person> *list, qsizetype index)
{
    return static_cast<BirthdayParty *>(list->data)->guest(index);
}

qsizetype BirthdayParty::guestCount(QQmlListProperty<Person> *list)
{
    return static_cast<BirthdayParty *>(list->data)->guestCount();
}

BirthdayPartyAttached *BirthdayParty::qmlAttachedProperties(QObject *object)
{
    return new BirthdayPartyAttached(object);
}
