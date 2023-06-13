// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>

#include "mainwindow.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    // this example and QQuickWidget are only functional when rendering with OpenGL
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);

    QCoreApplication::setApplicationName("Qt QQuickView/QQuickWidget Comparison Example");
    QCoreApplication::setOrganizationName("QtProject");
    QCoreApplication::setApplicationVersion(QT_VERSION_STR);
    QCommandLineParser parser;
    parser.setApplicationDescription(QCoreApplication::applicationName());
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption noRenderAlphaOption("no-render-alpha", "Do not render Alpha");
    parser.addOption(noRenderAlphaOption);
    QCommandLineOption transparentOption("transparent", "Transparent window");
    parser.addOption(transparentOption);

    parser.process(app);

    const bool transparency = parser.isSet(transparentOption);
    MainWindow widgetWindow(transparency, parser.isSet(noRenderAlphaOption));
    if (transparency) {
        widgetWindow.setAttribute(Qt::WA_TranslucentBackground);
        widgetWindow.setAttribute(Qt::WA_NoSystemBackground, false);
    }

    widgetWindow.resize(1024, 768);
    widgetWindow.show();

    return app.exec();
}
