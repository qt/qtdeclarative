// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef BIRTHDAYPARTY_H
#define BIRTHDAYPARTY_H

#include "person.h"
#include "ThirdPartyDisplay.h"

#include <QDate>
#include <QDebug>
#include <QObject>
#include <qqml.h>

#include <memory>

class BirthdayPartyAttached : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QDate rsvp READ rsvp WRITE setRsvp NOTIFY rsvpChanged FINAL)
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
    Q_PROPERTY(Person *host READ host WRITE setHost NOTIFY hostChanged FINAL)
    Q_PROPERTY(QQmlListProperty<Person> guests READ guests NOTIFY guestsChanged FINAL)
    Q_PROPERTY(QString announcement READ announcement WRITE setAnnouncement NOTIFY announcementChanged FINAL)
    Q_PROPERTY(ThirdPartyDisplay *display READ display WRITE setDisplay NOTIFY displayChanged FINAL)
    Q_CLASSINFO("DefaultProperty", "guests")
    QML_ELEMENT
    QML_ATTACHED(BirthdayPartyAttached)
public:
    using QObject::QObject;

    Person *host() const;
    void setHost(Person *);

    QQmlListProperty<Person> guests();
    void appendGuest(Person *);
    qsizetype guestCount() const;
    Person *guest(qsizetype) const;
    void clearGuests();
    void replaceGuest(qsizetype, Person *);
    void removeLastGuest();

    QString announcement() const;
    void setAnnouncement(const QString &);

    ThirdPartyDisplay *display() const;
    void setDisplay(ThirdPartyDisplay *);

    static BirthdayPartyAttached *qmlAttachedProperties(QObject *);

    void startParty();

signals:
    void hostChanged();
    void guestsChanged();
    void partyStarted(QDateTime time);
    void announcementChanged();
    void displayChanged();

private:
    static void appendGuest(QQmlListProperty<Person> *, Person *);
    static qsizetype guestCount(QQmlListProperty<Person> *);
    static Person *guest(QQmlListProperty<Person> *, qsizetype);
    static void clearGuests(QQmlListProperty<Person> *);
    static void replaceGuest(QQmlListProperty<Person> *, qsizetype, Person *);
    static void removeLastGuest(QQmlListProperty<Person> *);

    Person *m_host = nullptr;
    QList<Person *> m_guests;
    QString m_announcement;
    ThirdPartyDisplay *m_display = nullptr;
};

#endif // BIRTHDAYPARTY_H
