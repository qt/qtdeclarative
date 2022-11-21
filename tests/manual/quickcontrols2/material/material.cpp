// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QSettings>

int main(int argc, char *argv[])
{
    QGuiApplication::setApplicationName("material");
    QGuiApplication::setOrganizationName("QtProject");

    QGuiApplication app(argc, argv);

    const char *variantEnvVar = "QT_QUICK_CONTROLS_MATERIAL_VARIANT";
    if (!qEnvironmentVariableIsSet(variantEnvVar)) {
        QSettings settings;
        const char *variant = "variant";
        if (settings.contains(variant))
            qputenv(variantEnvVar, settings.value(variant).toByteArray());
    }

    QQmlApplicationEngine engine;
    const QUrl url(QStringLiteral("qrc:/material.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);
    engine.load(url);

    return app.exec();
}
