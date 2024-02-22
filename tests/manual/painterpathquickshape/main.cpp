// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QFontDatabase>

#include "svgpathloader.h"
#include "debugpaintitem.h"
#include "debugvisualizationcontroller.h"

int main(int argc, char *argv[])
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
    QGuiApplication app(argc, argv);
    app.setOrganizationName("QtProject");

    qmlRegisterType<DebugPaintItem>("io.qt", 1, 0, "DebugPaintItem");
    qmlRegisterType<SvgPathLoader>("io.qt", 1, 0, "SvgPathLoader");
    qmlRegisterType<DebugVisualizationController>("io.qt", 1, 0, "DebugVisualizationController");

    QFontDatabase::addApplicationFont(":/Graziano.ttf");
    QQmlApplicationEngine engine;
    const QUrl url(QStringLiteral("qrc:/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);
    engine.load(url);

    return app.exec();
}
