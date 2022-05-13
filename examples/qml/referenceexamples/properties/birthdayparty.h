// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef BIRTHDAYPARTY_H
#define BIRTHDAYPARTY_H

#include <QObject>
#include <QList>
#include <QQmlListProperty>
#include "person.h"

// ![0]
class BirthdayParty : public QObject
{
    Q_OBJECT
// ![0]
// ![1]
    Q_PROPERTY(Person *host READ host WRITE setHost)
// ![1]
// ![2]
    Q_PROPERTY(QQmlListProperty<Person> guests READ guests)
// ![2]
// ![3]
    QML_ELEMENT
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

private:
    static void appendGuest(QQmlListProperty<Person> *, Person *);
    static qsizetype guestCount(QQmlListProperty<Person> *);
    static Person* guest(QQmlListProperty<Person> *, qsizetype);
    static void clearGuests(QQmlListProperty<Person> *);
    static void replaceGuest(QQmlListProperty<Person> *, qsizetype, Person *);
    static void removeLastGuest(QQmlListProperty<Person> *);

    Person *m_host = nullptr;
    QList<Person *> m_guests;
};
// ![3]

#endif // BIRTHDAYPARTY_H
