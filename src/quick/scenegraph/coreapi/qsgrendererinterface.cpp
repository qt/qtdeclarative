// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsgrendererinterface.h"

QT_BEGIN_NAMESPACE

/*!
    \class QSGRendererInterface
    \brief An interface providing access to some of the graphics API specific internals
    of the scenegraph.
    \inmodule QtQuick
    \since 5.8

    Renderer interfaces allow accessing graphics API specific functionality in
    the scenegraph. Such internals are not typically exposed. However, when
    integrating custom rendering via QSGRenderNode for example, it may become
    necessary to query certain values, for instance the graphics device (e.g.
    the Direct3D or Vulkan device) that is used by the scenegraph.

    QSGRendererInterface's functions have varying availability. API and
    language queries, such as, graphicsApi() or shaderType() are always
    available, meaning it is sufficient to construct a QQuickWindow or
    QQuickView, and the graphics API or shading language in use can be queried
    right after via QQuickWindow::rendererInterface(). This guarantees that
    utilities like the GraphicsInfo QML type are able to report the correct
    values as early as possible, without having conditional property values -
    depending on for instance shaderType() - evaluate to unexpected values.

    Engine-specific accessors, like getResource(), are however available only
    after the scenegraph is initialized. Additionally, there may be
    backend-specific limitations on when such functions can be called. The only
    way that is guaranteed to succeed is calling them when the rendering of a
    node (i.e. the preparation of the command list for the next frame) is
    active. In practice this typically means QSGRenderNode::render().
 */

/*!
    \enum QSGRendererInterface::GraphicsApi
    \value Unknown An unknown graphics API is in use
    \value Software The Qt Quick 2D Renderer is in use
    \value OpenVG OpenVG via EGL
    \value [since 5.14] OpenGL OpenGL ES 2.0 or higher via a graphics abstraction layer.
    \value [since 5.14] Direct3D11 Direct3D 11 via a graphics abstraction layer.
    \value [since 6.6] Direct3D12 Direct3D 12 via a graphics abstraction layer.
    \value [since 5.14] Vulkan Vulkan 1.0 via a graphics abstraction layer.
    \value [since 5.14] Metal Metal via a graphics abstraction layer.
    \value [since 5.14] Null Null (no output) via a graphics abstraction layer.
    \omitvalue OpenGLRhi
    \omitvalue Direct3D11Rhi
    \omitvalue VulkanRhi
    \omitvalue MetalRhi
    \omitvalue NullRhi
  */

/*!
    \enum QSGRendererInterface::Resource

    \value DeviceResource The resource is a pointer to the graphics device,
    when applicable. For example, a \c{VkDevice *}, \c{MTLDevice *} or
    \c{ID3D11Device *}. Note that with Vulkan the returned value is a pointer
    to the VkDevice, not the handle itself. This is because Vulkan handles may
    not be pointers, and may use a different size from the architecture's
    pointer size so merely casting to/from \c{void *} is wrong.

    \value CommandQueueResource The resource is a pointer to the graphics
    command queue used by the scenegraph, when applicable. For example, a
    \c{VkQueue *} or \c{MTLCommandQueue *}. Note that with Vulkan the returned
    value is a pointer to the VkQueue, not the handle itself.

    \value CommandListResource The resource is a pointer to the command list or
    buffer used by the scenegraph, when applicable. For example, a
    \c{VkCommandBuffer *} or \c{MTLCommandBuffer *}. This object has limited
    validity, and is only valid while the scene graph is preparing the next
    frame. Note that with Vulkan the returned value is a pointer to the
    VkCommandBuffer, not the handle itself.

    \value PainterResource The resource is a pointer to the active QPainter
    used by the scenegraph, when running with the software backend.

    \value [since 5.14] RhiResource The resource is a pointer to the QRhi instance used by
    the scenegraph, when applicable.

    \value [since 6.0] RhiSwapchainResource The resource is a pointer to a QRhiSwapchain
    instance that is associated with the window. The value is null when the
    window is used in combination with QQuickRenderControl.

    \value [since 6.0] RhiRedirectCommandBuffer The resource is a pointer to a
    QRhiCommandBuffer instance that is associated with the window and its
    QQuickRenderControl. The value is null when the window is not associated
    with a QQuickRenderControl.

    \value [since 6.0] RhiRedirectRenderTarget The resource is a pointer to a
    QRhiTextureRenderTarget instance that is associated with the window and its
    QQuickRenderControl. The value is null when the window is not associated
    with a QQuickRenderControl. Note that the value always reflects the main
    texture render target and it does not depend on the Qt Quick scene, meaning
    it does not take any additional texture-targeting render passes generated
    by ShaderEffect or QQuickItem layers into account.

    \value [since 5.14] PhysicalDeviceResource The resource is a pointer to the pysical
    device object used by the scenegraph, when applicable. For example, a
    \c{VkPhysicalDevice *}. Note that with Vulkan the returned value is a
    pointer to the VkPhysicalDevice, not the handle itself.

    \value [since 5.14] OpenGLContextResource The resource is a pointer to the
    QOpenGLContext used by the scenegraph (on the render thread), when
    applicable.

    \value [since 5.14] DeviceContextResource The resource is a pointer to the device
    context used by the scenegraph, when applicable. For example, a
    \c{ID3D11DeviceContext *}.

    \value [since 5.14] CommandEncoderResource The resource is a pointer to the currently
    active render command encoder object used by the scenegraph, when
    applicable. For example, a \c{MTLRenderCommandEncoder *}. This object has
    limited validity, and is only valid while the scene graph is recording a
    render pass for the next frame.

    \value [since 5.14] VulkanInstanceResource The resource is a pointer to the
    QVulkanInstance used by the scenegraph, when applicable.

    \value [since 5.14] RenderPassResource The resource is a pointer to the main render pass
    used by the scenegraph, describing the color and depth/stecil attachments
    and how they are used. For example, a \c{VkRenderPass *}. Note that the
    value always reflects the main render target (either the on-screen window
    or the texture QQuickRenderControl redirects to) and it does not depend on
    the Qt Quick scene, meaning it does not take any additional
    texture-targeting render passes generated by ShaderEffect or QQuickItem
    layers into account.

    \value [since 6.4] RedirectPaintDevice The resource is a pointer to QPaintDevice instance
    that is associated with the window and its QQuickRenderControl. The value is
    null when the window is not associated with a QQuickRenderControl.

    \value [since 6.6] GraphicsQueueFamilyIndexResource The resource is a pointer to the
    graphics queue family index used by the scenegraph, when applicable. With
    Vulkan, this is a pointer to a \c uint32_t index value.

    \value [since 6.6] GraphicsQueueIndexResource The resource is a pointer to the graphics
    queue index (uint32_t) used by the scenegraph, when applicable. With
    Vulkan, this is a pointer to a \c uint32_t index value, which in practice
    is the index of the VkQueue reported for \c CommandQueueResource.
 */

/*!
    \enum QSGRendererInterface::ShaderType
    \value UnknownShadingLanguage Not yet known due to no window and scenegraph associated
    \value GLSL GLSL or GLSL ES
    \value HLSL HLSL
    \value [since 5.14] RhiShader Consumes QShader instances containing shader variants for
    multiple target languages and intermediate formats.
 */

/*!
    \enum QSGRendererInterface::ShaderCompilationType
    \value RuntimeCompilation Runtime compilation of shader source code is supported
    \value OfflineCompilation Pre-compiled bytecode supported
 */

/*!
    \enum QSGRendererInterface::ShaderSourceType

    \value ShaderSourceString Shader source can be provided as a string in
    the corresponding properties of ShaderEffect

    \value ShaderSourceFile Local or resource files containing shader source
    code are supported

    \value ShaderByteCode Local or resource files containing shader bytecode are
    supported
 */

/*!
    \enum QSGRendererInterface::RenderMode

    \value RenderMode2D Normal 2D rendering
    \value RenderMode2DNoDepthBuffer Normal 2D rendering with depth buffer disabled
    \value RenderMode3D Scene is rendered as part of a 3D graph
 */
QSGRendererInterface::~QSGRendererInterface()
{
}

/*!
    \fn QSGRendererInterface::GraphicsApi QSGRendererInterface::graphicsApi() const

    Returns the graphics API that is in use by the Qt Quick scenegraph.

    \note This function can be called on any thread.
 */

/*!
    Queries a graphics \a resource in \a window. Returns null when the resource in question is
    not supported or not available.

    When successful, the returned pointer is either a direct pointer to an
    interface, or a pointer to an opaque handle that needs to be dereferenced
    first (for example, \c{VkDevice dev = *static_cast<VkDevice
    *>(result)}). The latter is necessary since such handles may have sizes
    different from a pointer.

    \note The ownership of the returned pointer is never transferred to the caller.

    \note This function must only be called on the render thread.
 */
void *QSGRendererInterface::getResource(QQuickWindow *window, Resource resource) const
{
    Q_UNUSED(window);
    Q_UNUSED(resource);
    return nullptr;
}

/*!
    Queries a graphics resource. \a resource is a backend-specific key. This
    allows supporting any future resources that are not listed in the
    Resource enum.

    \note The ownership of the returned pointer is never transferred to the caller.

    \note This function must only be called on the render thread.
 */
void *QSGRendererInterface::getResource(QQuickWindow *window, const char *resource) const
{
    Q_UNUSED(window);
    Q_UNUSED(resource);
    return nullptr;
}

/*!
    \return true if \a api is based on a graphics abstraction layer (QRhi)
    instead of directly calling the native graphics API.

    \note This function can be called on any thread.

    \since 5.14
 */
bool QSGRendererInterface::isApiRhiBased(GraphicsApi api)
{
    switch (api) {
    case OpenGL:
    case Direct3D11:
    case Direct3D12:
    case Vulkan:
    case Metal:
    case Null:
        return true;
    default:
        return false;
    }
}

/*!
    \fn QSGRendererInterface::ShaderType QSGRendererInterface::shaderType() const

    \return the shading language supported by the Qt Quick backend the
    application is using.

    \note This function can be called on any thread.

    \sa QtQuick::GraphicsInfo
 */

/*!
    \fn QSGRendererInterface::ShaderCompilationTypes QSGRendererInterface::shaderCompilationType() const

    \return a bitmask of the shader compilation approaches supported by the Qt
    Quick backend the application is using.

    \note This function can be called on any thread.

    \sa QtQuick::GraphicsInfo
 */

/*!
    \fn QSGRendererInterface::ShaderSourceTypes QSGRendererInterface::shaderSourceType() const

    \return a bitmask of the supported ways of providing shader sources in ShaderEffect items.

    \note This function can be called on any thread.

    \sa QtQuick::GraphicsInfo
 */

QT_END_NAMESPACE
