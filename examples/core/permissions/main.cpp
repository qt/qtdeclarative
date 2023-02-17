// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QGuiApplication>
#include <QQmlApplicationEngine>

int main(int argc, char **argv)
{
    QT_USE_NAMESPACE
    QGuiApplication::setApplicationName("Permissions");
    QGuiApplication::setOrganizationName("QtProject");

    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;
    engine.loadFromModule("PermissionsExample", "Main");
    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
