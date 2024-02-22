// Copyright (C) 2016 Research In Motion.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtQml>
#include <QGuiApplication>
#include "testdriver.h"

int main (int argc, char* argv[])
{
    QGuiApplication app(argc, argv);
    int i = -1;
    if (argc > 1)
        i = atoi(argv[1]);
    if (i < 1)
        i = -1;
    TestDriver td(QUrl::fromLocalFile("main.qml"), i);
    return app.exec();
}
