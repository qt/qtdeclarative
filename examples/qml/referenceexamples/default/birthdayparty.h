// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef BIRTHDAYPARTY_H
#define BIRTHDAYPARTY_H

#include <QObject>
#include <QQmlListProperty>
#include "person.h"

// ![0]
class BirthdayParty : public QObject
{
    Q_OBJECT
    Q_PROPERTY(Person *host READ host WRITE setHost)
    Q_PROPERTY(QQmlListProperty<Person> guests READ guests)
    Q_CLASSINFO("DefaultProperty", "guests")
    QML_ELEMENT
public:
    using QObject::QObject;

    Person *host() const;
    void setHost(Person *);

    QQmlListProperty<Person> guests();
    qsizetype guestCount() const;
    Person *guest(qsizetype) const;

private:
    Person *m_host = nullptr;
    QList<Person *> m_guests;
};
// ![0]

#endif // BIRTHDAYPARTY_H
