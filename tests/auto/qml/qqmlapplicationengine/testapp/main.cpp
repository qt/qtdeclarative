// Copyright (C) 2016 Research In Motion.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QCoreApplication>
#include <QQmlApplicationEngine>

int main (int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QQmlApplicationEngine e(QUrl(QString("qrc:///") + argv[1]));
    return app.exec();
}
