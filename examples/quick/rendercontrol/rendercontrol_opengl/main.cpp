// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QGuiApplication>
#include <QQuickWindow>
#include "window_singlethreaded.h"

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);

    // only functional when Qt Quick is also using OpenGL
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);

    WindowSingleThreaded window;
    window.resize(1024, 768);
    window.show();

    return app.exec();
}
