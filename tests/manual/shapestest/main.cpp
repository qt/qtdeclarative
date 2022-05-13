// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QGuiApplication>
#include <QSurfaceFormat>
#include <QQuickView>
#include <QQmlEngine>

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);

    QQuickView view;

    QSurfaceFormat fmt;
    fmt.setDepthBufferSize(24);
    fmt.setStencilBufferSize(8);
    if (app.arguments().contains(QStringLiteral("--multisample")))
        fmt.setSamples(4);
    if (app.arguments().contains(QStringLiteral("--coreprofile"))) {
        fmt.setVersion(4, 3);
        fmt.setProfile(QSurfaceFormat::CoreProfile);
    }
    view.setFormat(fmt);

    view.setResizeMode(QQuickView::SizeRootObjectToView);
    view.resize(1024, 768);
    view.setSource(QUrl("qrc:/shapestest/shapestest.qml"));
    view.show();

    return app.exec();
}
