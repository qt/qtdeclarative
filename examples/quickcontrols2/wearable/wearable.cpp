// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QIcon>

int main(int argc, char *argv[])
{
    QCoreApplication::setApplicationName("Wearable");
    QCoreApplication::setOrganizationName("QtProject");

    QGuiApplication app(argc, argv);

    //! [style]
    QQuickStyle::setStyle(QStringLiteral("qrc:/qml/Style"));
    //! [style]

    //! [icons]
    QIcon::setThemeName(QStringLiteral("wearable"));
    //! [icons]

    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/wearable.qml")));

    return app.exec();
}
