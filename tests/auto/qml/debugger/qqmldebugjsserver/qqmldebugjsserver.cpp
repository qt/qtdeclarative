// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtGui/qguiapplication.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlapplicationengine.h>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QQmlEngine someWeirdEngine; // add another engine to cause some trouble

    QQmlApplicationEngine engine;
    engine.load(QUrl::fromLocalFile(QLatin1String(argv[argc - 1])));

    return app.exec();
}

