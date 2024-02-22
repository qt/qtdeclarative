// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("docImagesDir", QString(DOC_IMAGES_DIR));
    engine.load(QUrl("qrc:/styles-cover-flow.qml"));
    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
