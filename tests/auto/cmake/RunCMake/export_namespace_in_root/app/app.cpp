// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QDebug>
#include <QDirIterator>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);


    QDirIterator dirIterator(":/", QDirIterator::Subdirectories);
    while (dirIterator.hasNext()) {
        qDebug() << dirIterator.next();
    }
    QQmlApplicationEngine engine;
    engine.loadFromModule("ExampleProjectApp", "Main");

    return app.exec();
}
