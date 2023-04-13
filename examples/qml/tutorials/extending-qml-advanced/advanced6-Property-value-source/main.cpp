// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "birthdayparty.h"
#include "person.h"

#include <QCoreApplication>
#include <QDebug>
#include <QQmlComponent>
#include <QQmlEngine>

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.loadFromModule("People", "Main");
    std::unique_ptr<BirthdayParty> party{ qobject_cast<BirthdayParty *>(component.create()) };

    if (party && party->host()) {
        qInfo() << party->host()->name() << "is having a birthday!";

        if (qobject_cast<Boy *>(party->host()))
            qInfo() << "He is inviting:";
        else
            qInfo() << "She is inviting:";

        for (qsizetype ii = 0; ii < party->guestCount(); ++ii) {
            Person *guest = party->guest(ii);

            QDate rsvpDate;
            QObject *attached = qmlAttachedPropertiesObject<BirthdayParty>(guest, false);
            if (attached)
                rsvpDate = attached->property("rsvp").toDate();

            if (rsvpDate.isNull())
                qInfo() << "   " << guest->name() << "RSVP date: Hasn't RSVP'd";
            else
                qInfo() << "   " << guest->name() << "RSVP date:" << rsvpDate.toString();
        }

        party->startParty();
        return QCoreApplication::exec();
    }

    qWarning() << component.errors();
    return EXIT_FAILURE;
}
