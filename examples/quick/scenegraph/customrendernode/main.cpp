// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QGuiApplication>
#include <QtQuick/QQuickView>
#include <QSurfaceFormat>

int main(int argc, char **argv)
{
#ifdef Q_OS_MACOS
    QSurfaceFormat format = QSurfaceFormat::defaultFormat();
    format.setMajorVersion(4);
    format.setMinorVersion(1);
    format.setProfile(QSurfaceFormat::CoreProfile);
    QSurfaceFormat::setDefaultFormat(format);
#endif

    QGuiApplication app(argc, argv);

    QQuickView view;

    view.setResizeMode(QQuickView::SizeRootObjectToView);
    view.setSource(QUrl("qrc:///scenegraph/customrendernode/main.qml"));
    view.setColor(QColor(0, 0, 0));
    view.show();

    QString api;
    switch (view.graphicsApi()) {
    case QSGRendererInterface::GraphicsApi::OpenGLRhi:
        api = "RHI OpenGL";
        break;
    case QSGRendererInterface::GraphicsApi::Direct3D11Rhi:
        api = "RHI Direct3D";
        break;
    case QSGRendererInterface::GraphicsApi::VulkanRhi:
        api = "RHI Vulkan";
        break;
    case QSGRendererInterface::GraphicsApi::MetalRhi:
        api = "RHI Metal";
        break;
    case QSGRendererInterface::GraphicsApi::NullRhi:
        api = "RHI Null";
        break;
    default:
        api = "unknown";
        break;
    }

    view.setTitle(QStringLiteral("Custom QSGRenderNode - ") + api);

    return QGuiApplication::exec();
}
