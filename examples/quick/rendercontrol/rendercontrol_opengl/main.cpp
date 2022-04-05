// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QGuiApplication>
#include <QQuickWindow>
#include <QCommandLineParser>
#include "window_singlethreaded.h"
#include "window_multithreaded.h"

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);

    // only functional when Qt Quick is also using OpenGL
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);

    QCoreApplication::setApplicationName("Qt Render Control Example");
    QCoreApplication::setOrganizationName("QtProject");
    QCoreApplication::setApplicationVersion(QT_VERSION_STR);
    QCommandLineParser parser;
    parser.setApplicationDescription(QCoreApplication::applicationName());
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption threadedOption("threaded", "Threaded Rendering");
    parser.addOption(threadedOption);

    parser.process(app);

    QScopedPointer<QWindow> window;
    if (parser.isSet(threadedOption)) {
        qWarning("Using separate Qt Quick render thread");
        window.reset(new WindowMultiThreaded);
    } else {
        qWarning("Using single-threaded rendering");
        window.reset(new WindowSingleThreaded);
    }

    window->resize(1024, 768);
    window->show();

    return app.exec();
}
