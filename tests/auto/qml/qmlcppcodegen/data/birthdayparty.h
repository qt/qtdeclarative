// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

struct Foozle {
    Q_GADGET
    int foo = 1;
};

class BirthdayParty : public QObject
{
    Q_OBJECT
    Q_PROPERTY(Person *host READ host WRITE setHost NOTIFY hostChanged FINAL)
    Q_PROPERTY(QQmlListProperty<Person> guests READ guests)
    Q_PROPERTY(QStringList guestNames READ guestNames FINAL)
    Q_PROPERTY(QVariantList stuffs READ stuffs FINAL)
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

    QStringList guestNames() const;
    QVariantList stuffs() const;

    Q_INVOKABLE void invite(const QString &name);
    static BirthdayPartyAttached *qmlAttachedProperties(QObject *object);

signals:
    void hostChanged();
    void partyStarted(Foozle foozle);

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
