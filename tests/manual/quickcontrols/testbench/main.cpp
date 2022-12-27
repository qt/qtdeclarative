// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QDebug>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QSettings>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QtQuickControls2/private/qquickstyle_p.h>

#include "assetfixer.h"
#include "clipboard.h"
#include "directoryvalidator.h"

int main(int argc, char *argv[])
{
    QGuiApplication::setApplicationName("testbench");
    QGuiApplication::setOrganizationName("QtProject");

    QGuiApplication app(argc, argv);

    QSettings settings;
    QString style = QQuickStyle::name();
    if (!style.isEmpty() && !QQuickStylePrivate::isUsingDefaultStyle())
        settings.setValue("style", style);
    else
        QQuickStyle::setStyle(settings.value("style").isValid() ? settings.value("style").toString() : "Imagine");

    if (QFontDatabase::addApplicationFont(":/fonts/fontello.ttf") == -1) {
        qWarning() << "Failed to load fontawesome font";
    }

    QQmlApplicationEngine engine;

    qmlRegisterType<AssetFixer>("Backend", 1, 0, "AssetFixer");
    qmlRegisterType<Clipboard>("Backend", 1, 0, "Clipboard");
    qmlRegisterType<DirectoryValidator>("Backend", 1, 0, "DirectoryValidator");

    engine.rootContext()->setContextProperty("availableStyles", QQuickStylePrivate::builtInStyles());

    engine.load(QUrl(QStringLiteral("qrc:/testbench.qml")));

    return app.exec();
}

