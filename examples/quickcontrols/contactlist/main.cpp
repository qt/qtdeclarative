// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QGuiApplication>
#include <QQmlApplicationEngine>

#include "contactmodel.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    qmlRegisterType<ContactModel>("Backend", 1, 0, "ContactModel");

    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/contactlist.qml")));

    return app.exec();
}
