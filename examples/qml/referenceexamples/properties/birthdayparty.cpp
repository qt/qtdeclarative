// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "birthdayparty.h"

// ![0]
Person *BirthdayParty::host() const
{
    return m_host;
}

void BirthdayParty::setHost(Person *c)
{
    m_host = c;
}

QQmlListProperty<Person> BirthdayParty::guests()
{
    return {this, this,
             &BirthdayParty::appendGuest,
             &BirthdayParty::guestCount,
             &BirthdayParty::guest,
             &BirthdayParty::clearGuests,
             &BirthdayParty::replaceGuest,
             &BirthdayParty::removeLastGuest};
}

void BirthdayParty::appendGuest(Person *p)
{
    m_guests.append(p);
}

qsizetype BirthdayParty::guestCount() const
{
    return m_guests.count();
}

Person *BirthdayParty::guest(qsizetype index) const
{
    return m_guests.at(index);
}

void BirthdayParty::clearGuests() {
    m_guests.clear();
}

void BirthdayParty::replaceGuest(qsizetype index, Person *p)
{
    m_guests[index] = p;
}

void BirthdayParty::removeLastGuest()
{
    m_guests.removeLast();
}

// ![0]

void BirthdayParty::appendGuest(QQmlListProperty<Person> *list, Person *p)
{
    reinterpret_cast< BirthdayParty *>(list->data)->appendGuest(p);
}

void BirthdayParty::clearGuests(QQmlListProperty<Person>* list)
{
    reinterpret_cast< BirthdayParty *>(list->data)->clearGuests();
}

void BirthdayParty::replaceGuest(QQmlListProperty<Person> *list, qsizetype i, Person *p)
{
    reinterpret_cast< BirthdayParty* >(list->data)->replaceGuest(i, p);
}

void BirthdayParty::removeLastGuest(QQmlListProperty<Person> *list)
{
    reinterpret_cast< BirthdayParty* >(list->data)->removeLastGuest();
}

Person* BirthdayParty::guest(QQmlListProperty<Person> *list, qsizetype i)
{
    return reinterpret_cast< BirthdayParty* >(list->data)->guest(i);
}

qsizetype BirthdayParty::guestCount(QQmlListProperty<Person> *list)
{
    return reinterpret_cast< BirthdayParty* >(list->data)->guestCount();
}
