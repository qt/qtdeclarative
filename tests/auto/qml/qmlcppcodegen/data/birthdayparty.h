/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef BIRTHDAYPARTY_H
#define BIRTHDAYPARTY_H

#include <QObject>
#include <QQmlListProperty>
#include <QProperty>
#include <QBindable>
#include <QDateTime>
#include "person.h"

class BirthdayPartyAttached : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QDateTime rsvp READ rsvp WRITE setRsvp BINDABLE rsvpBindable)
    QML_ANONYMOUS
public:
    BirthdayPartyAttached(QObject *parent);

    QDateTime rsvp() const;
    void setRsvp(const QDateTime &rsvp);
    QBindable<QDateTime> rsvpBindable();

private:
    QProperty<QDateTime> m_rsvp;
};

class BirthDayPartyExtended : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int eee READ eee WRITE setEee NOTIFY eeeChanged)
public:

    BirthDayPartyExtended(QObject *parent) : QObject(parent) {}

    int eee() const { return m_eee; }
    void setEee(int eee)
    {
        if (eee != m_eee) {
            m_eee = eee;
            emit eeeChanged();
        }
    }

signals:
    void eeeChanged();

private:
    int m_eee = 25;
};

class BirthdayParty : public QObject
{
    Q_OBJECT
    Q_PROPERTY(Person *host READ host WRITE setHost NOTIFY hostChanged FINAL)
    Q_PROPERTY(QQmlListProperty<Person> guests READ guests)
    QML_ELEMENT
    QML_ATTACHED(BirthdayPartyAttached)
    QML_EXTENDED(BirthDayPartyExtended)
public:
    BirthdayParty(QObject *parent = nullptr);

    Person *host() const;
    void setHost(Person *);

    QQmlListProperty<Person> guests();
    int guestCount() const;
    Person *guest(int) const;

    Q_INVOKABLE void invite(const QString &name);
    static BirthdayPartyAttached *qmlAttachedProperties(QObject *object);

signals:
    void hostChanged();

private:
    Person *m_host;
    QList<Person *> m_guests;
};

class NastyBase : public QObject {
public:
    NastyBase(QObject *parent) : QObject(parent) {}
};

class Nasty : public NastyBase
{
    Q_OBJECT
    QML_ELEMENT
    QML_ATTACHED(Nasty)

public:
    Nasty(QObject *parent = nullptr) : NastyBase(parent) {}
    static Nasty *qmlAttachedProperties(QObject *object) { return new Nasty(object); }
};

#endif // BIRTHDAYPARTY_H
