// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QGuiApplication>
#include <QQuickView>
#include <QSurfaceFormat>

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);

    // On macOS, request a core profile context in the unlikely case of using OpenGL.
#ifdef Q_OS_MACOS
    QSurfaceFormat format = QSurfaceFormat::defaultFormat();
    format.setMajorVersion(4);
    format.setMinorVersion(1);
    format.setProfile(QSurfaceFormat::CoreProfile);
    QSurfaceFormat::setDefaultFormat(format);
#endif

    QQuickView view;

    view.setResizeMode(QQuickView::SizeRootObjectToView);
    view.setSource(QUrl("qrc:///scenegraph/customrendernode/main.qml"));
    view.setColor(Qt::black);
    view.show();

    QString api;
    switch (view.graphicsApi()) {
    case QSGRendererInterface::OpenGL:
        api = "RHI OpenGL";
        break;
    case QSGRendererInterface::Direct3D11:
        api = "RHI Direct 3D 11";
        break;
    case QSGRendererInterface::Direct3D12:
        api = "RHI Direct 3D 12";
        break;
    case QSGRendererInterface::Vulkan:
        api = "RHI Vulkan";
        break;
    case QSGRendererInterface::Metal:
        api = "RHI Metal";
        break;
    case QSGRendererInterface::Null:
        api = "RHI Null";
        break;
    default:
        api = "unknown";
        break;
    }

    view.setTitle(QStringLiteral("Custom QSGRenderNode - ") + api);

    return app.exec();
}
