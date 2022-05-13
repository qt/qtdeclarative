// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef BIRTHDAYPARTY_H
#define BIRTHDAYPARTY_H

#include <QObject>
#include <QDate>
#include <QDebug>
#include <qqml.h>
#include "person.h"

class BirthdayPartyAttached : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QDate rsvp READ rsvp WRITE setRsvp NOTIFY rsvpChanged)
    QML_ANONYMOUS
public:
    using QObject::QObject;

    QDate rsvp() const;
    void setRsvp(QDate);

signals:
    void rsvpChanged();

private:
    QDate m_rsvp;
};

class BirthdayParty : public QObject
{
    Q_OBJECT
// ![0]
    Q_PROPERTY(Person *host READ host WRITE setHost NOTIFY hostChanged)
// ![0]
    Q_PROPERTY(QQmlListProperty<Person> guests READ guests)
    Q_PROPERTY(QString announcement READ announcement WRITE setAnnouncement)
    Q_CLASSINFO("DefaultProperty", "guests")
    QML_ELEMENT
    QML_ATTACHED(BirthdayPartyAttached)
public:
    using QObject::QObject;

    Person *host() const;
    void setHost(Person *);

    QQmlListProperty<Person> guests();
    qsizetype guestCount() const;
    Person *guest(qsizetype) const;

    QString announcement() const;
    void setAnnouncement(const QString &);

    static BirthdayPartyAttached *qmlAttachedProperties(QObject *);

    void startParty();
signals:
    void partyStarted(QTime time);
    void hostChanged();

private:
    Person *m_host = nullptr;
    QList<Person *> m_guests;
};

#endif // BIRTHDAYPARTY_H
