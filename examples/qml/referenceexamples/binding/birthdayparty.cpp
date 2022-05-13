// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "birthdayparty.h"

QDate BirthdayPartyAttached::rsvp() const
{
    return m_rsvp;
}

void BirthdayPartyAttached::setRsvp(QDate d)
{
    if (d != m_rsvp) {
        m_rsvp = d;
        emit rsvpChanged();
    }
}

Person *BirthdayParty::host() const
{
    return m_host;
}

void BirthdayParty::setHost(Person *c)
{
    if (c == m_host) return;
    m_host = c;
    emit hostChanged();
}

QQmlListProperty<Person> BirthdayParty::guests()
{
    return QQmlListProperty<Person>(this, &m_guests);
}

qsizetype BirthdayParty::guestCount() const
{
    return m_guests.count();
}

Person *BirthdayParty::guest(qsizetype index) const
{
    return m_guests.at(index);
}

void BirthdayParty::startParty()
{
    QTime time = QTime::currentTime();
    emit partyStarted(time);
}

QString BirthdayParty::announcement() const
{
    return QString();
}

void BirthdayParty::setAnnouncement(const QString &speak)
{
    qWarning().noquote() << speak;
}

BirthdayPartyAttached *BirthdayParty::qmlAttachedProperties(QObject *object)
{
    return new BirthdayPartyAttached(object);
}

