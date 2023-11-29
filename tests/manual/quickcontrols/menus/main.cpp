// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>

int main(int argc, char *argv[])
{
    QGuiApplication::setApplicationName("menus");
    QGuiApplication::setOrganizationName("QtProject");

    QGuiApplication app(argc, argv);

    qputenv("QT_QUICK_CONTROLS_USE_NATIVE_MENUS", "1");

    QQmlApplicationEngine engine;
    engine.setInitialProperties({{ "currentStyle", QQuickStyle::name() }});
    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));

    return app.exec();
}

