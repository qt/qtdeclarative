// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QGuiApplication>
#include <QQmlApplicationEngine>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QGuiApplication::setOrganizationName("QtProject");
    QGuiApplication::setApplicationName("File System Explorer");

    QQmlApplicationEngine engine;
    engine.loadFromModule("FileSystemModule", "Main");
    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
