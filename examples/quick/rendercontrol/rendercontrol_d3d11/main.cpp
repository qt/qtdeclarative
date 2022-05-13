// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

// This example is only functional on Windows with Direct 3D 11.

#include <QGuiApplication>
#include <QQuickWindow>
#include "engine.h"
#include "window.h"

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);

    QCoreApplication::setApplicationName("Qt Render Control D3D11 Example");
    QCoreApplication::setOrganizationName("QtProject");
    QCoreApplication::setApplicationVersion(QT_VERSION_STR);

    // only functional when Qt Quick is also using D3D11
    QQuickWindow::setGraphicsApi(QSGRendererInterface::Direct3D11);

    Engine engine;
    if (!engine.create())
        qFatal("Failed to initialize D3D (this example requires D3D 11.1 and DXGI 1.3)");

    Window window(&engine);

    window.resize(1024, 768);
    window.show();

    return app.exec();
}
