/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

#ifndef QSGRENDERERINTERFACE_H
#define QSGRENDERERINTERFACE_H

#include <QtQuick/qsgnode.h>

QT_BEGIN_NAMESPACE

class QQuickWindow;

class Q_QUICK_EXPORT QSGRendererInterface
{
public:
    enum GraphicsApi {
        Unknown,
        Software,
        OpenVG,
        OpenGL,
        Direct3D11,
        Vulkan,
        Metal,
        Null,

        OpenGLRhi = OpenGL,
        Direct3D11Rhi = Direct3D11,
        VulkanRhi = Vulkan,
        MetalRhi = Metal,
        NullRhi = Null
    };

    enum Resource {
        DeviceResource,
        CommandQueueResource,
        CommandListResource,
        PainterResource,
        RhiResource,
        RhiSwapchainResource,
        RhiRedirectCommandBuffer,
        RhiRedirectRenderTarget,
        PhysicalDeviceResource,
        OpenGLContextResource,
        DeviceContextResource,
        CommandEncoderResource,
        VulkanInstanceResource,
        RenderPassResource
    };

    enum ShaderType {
        UnknownShadingLanguage,
        GLSL,
        HLSL,
        RhiShader
    };

    enum ShaderCompilationType {
        RuntimeCompilation = 0x01,
        OfflineCompilation = 0x02
    };
    Q_DECLARE_FLAGS(ShaderCompilationTypes, ShaderCompilationType)

    enum ShaderSourceType {
        ShaderSourceString = 0x01,
        ShaderSourceFile = 0x02,
        ShaderByteCode = 0x04
    };
    Q_DECLARE_FLAGS(ShaderSourceTypes, ShaderSourceType)

    enum RenderMode {
        RenderMode2D,
        RenderMode2DNoDepthBuffer,
        RenderMode3D
    };

    virtual ~QSGRendererInterface();

    virtual GraphicsApi graphicsApi() const = 0;

    virtual void *getResource(QQuickWindow *window, Resource resource) const;
    virtual void *getResource(QQuickWindow *window, const char *resource) const;

    virtual ShaderType shaderType() const = 0;
    virtual ShaderCompilationTypes shaderCompilationType() const = 0;
    virtual ShaderSourceTypes shaderSourceType() const = 0;

    static bool isApiRhiBased(GraphicsApi api);
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QSGRendererInterface::ShaderCompilationTypes)
Q_DECLARE_OPERATORS_FOR_FLAGS(QSGRendererInterface::ShaderSourceTypes)

QT_END_NAMESPACE

#endif
