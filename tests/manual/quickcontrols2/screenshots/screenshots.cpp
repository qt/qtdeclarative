// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QFileInfo>
#include <QDir>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;
    const QUrl snippetsDir = QUrl::fromLocalFile(SNIPPETS_DIR);
    Q_ASSERT(snippetsDir.isValid() && QFileInfo(snippetsDir.toLocalFile()).exists());
    engine.rootContext()->setContextProperty("snippetsDir", snippetsDir);
    engine.rootContext()->setContextProperty("screenshotsDir", QUrl::fromLocalFile(QDir::currentPath()));
    engine.rootContext()->setContextProperty("screenshotsDirStr", QDir::currentPath());
    engine.load(QUrl("qrc:/screenshots.qml"));
    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
