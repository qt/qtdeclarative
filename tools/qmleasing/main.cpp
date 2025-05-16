// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#include "mainwindow.h"

#include <QApplication>

int main(int argc, char ** argv)
{
    QApplication app(argc, argv);
    QCoreApplication::setApplicationVersion(QLatin1String(QT_VERSION_STR));

    MainWindow mainWindow;
    mainWindow.show();
    mainWindow.showQuickView();

    return app.exec();
}
