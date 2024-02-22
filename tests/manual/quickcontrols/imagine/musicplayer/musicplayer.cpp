// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QSettings>
#include <QIcon>

int main(int argc, char *argv[])
{
    QGuiApplication::setApplicationName("Music Player");
    QGuiApplication::setOrganizationName("QtProject");

    QGuiApplication app(argc, argv);

    QIcon::setThemeName("musicplayer");

    QQmlApplicationEngine engine;
    engine.load(QUrl("qrc:/musicplayer.qml"));
    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
