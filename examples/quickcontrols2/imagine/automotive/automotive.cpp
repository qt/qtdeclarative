// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QSettings>
#include <QQuickStyle>
#include <QIcon>

int main(int argc, char *argv[])
{
    QGuiApplication::setApplicationName("Automotive");
    QGuiApplication::setOrganizationName("QtProject");

    QGuiApplication app(argc, argv);

    QIcon::setThemeName("automotive");

    QQmlApplicationEngine engine;
    engine.load(QUrl("qrc:/qml/automotive.qml"));
    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
