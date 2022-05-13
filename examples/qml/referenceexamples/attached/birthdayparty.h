// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef BIRTHDAYPARTY_H
#define BIRTHDAYPARTY_H

#include <QObject>
#include <QDate>
#include <qqml.h>
#include "person.h"

class BirthdayPartyAttached : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QDate rsvp READ rsvp WRITE setRsvp)
    QML_ANONYMOUS
public:
    using QObject::QObject;

    QDate rsvp() const;
    void setRsvp(QDate);

private:
    QDate m_rsvp;
};

class BirthdayParty : public QObject
{
    Q_OBJECT
    Q_PROPERTY(Person *host READ host WRITE setHost)
    Q_PROPERTY(QQmlListProperty<Person> guests READ guests)
    Q_CLASSINFO("DefaultProperty", "guests")
    QML_ELEMENT

//! [declare attached]
    QML_ATTACHED(BirthdayPartyAttached)
//! [declare attached]

public:
    using QObject::QObject;

    Person *host() const;
    void setHost(Person *);

    QQmlListProperty<Person> guests();
    qsizetype guestCount() const;
    Person *guest(qsizetype) const;

    //! [static attached]
    static BirthdayPartyAttached *qmlAttachedProperties(QObject *);
    //! [static attached]
private:
    Person *m_host = nullptr;
    QList<Person *> m_guests;
};

#endif // BIRTHDAYPARTY_H
