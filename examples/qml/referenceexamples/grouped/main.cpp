// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QCoreApplication>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QDebug>
#include "birthdayparty.h"
#include "person.h"

int main(int argc, char ** argv)
{
    QCoreApplication app(argc, argv);

    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl("qrc:example.qml"));
    auto *party = qobject_cast<BirthdayParty *>(component.create());

    if (party && party->host()) {
        qInfo() << party->host()->name() << "is having a birthday!";

        if (qobject_cast<Boy *>(party->host()))
            qInfo() << "He is inviting:";
        else
            qInfo() << "She is inviting:";

        Person *bestShoe = nullptr;
        for (qsizetype ii = 0; ii < party->guestCount(); ++ii) {
            Person *guest = party->guest(ii);
            qInfo() << "   " << guest->name();

            if (!bestShoe || bestShoe->shoe()->price() < guest->shoe()->price())
                bestShoe = guest;
        }
        if (bestShoe)
            qInfo() << bestShoe->name() << "is wearing the best shoes!";

        return EXIT_SUCCESS;
    }

    qWarning() << component.errors();
    return EXIT_FAILURE;
}
