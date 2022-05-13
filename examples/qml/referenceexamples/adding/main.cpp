// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QCoreApplication>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QDebug>
#include "person.h"

int main(int argc, char ** argv)
{
    QCoreApplication app(argc, argv);

    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl("qrc:example.qml"));
    auto *person = qobject_cast<Person *>(component.create());
    if (!person) {
        qWarning() << component.errors();
        return EXIT_FAILURE;
    }

     qInfo() << "The person's name is" << person->name()
         << "\nThey wear a" << person->shoeSize() << "sized shoe";

    return EXIT_SUCCESS;
}
