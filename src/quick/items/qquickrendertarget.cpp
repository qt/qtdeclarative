// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickrendertarget_p.h"
#include <rhi/qrhi.h>
#include <QtQuick/private/qquickitem_p.h>
#include <QtQuick/private/qquickwindow_p.h>
#include <QtQuick/private/qsgrhisupport_p.h>

QT_BEGIN_NAMESPACE

/*!
    \class QQuickRenderTarget
    \since 6.0
    \inmodule QtQuick

    \brief The QQuickRenderTarget class provides an opaque container for native
    graphics resources specifying a render target, and associated metadata.

    \sa QQuickWindow::setRenderTarget(), QQuickGraphicsDevice
*/

QQuickRenderTargetPrivate::QQuickRenderTargetPrivate()
    : ref(1)
{
}

QQuickRenderTargetPrivate::QQuickRenderTargetPrivate(const QQuickRenderTargetPrivate &other)
    : ref(1),
      type(other.type),
      pixelSize(other.pixelSize),
      devicePixelRatio(other.devicePixelRatio),
      sampleCount(other.sampleCount),
      u(other.u),
      mirrorVertically(other.mirrorVertically)
{
}

/*!
    Constructs a default QQuickRenderTarget that does not reference any native
    objects.
 */
QQuickRenderTarget::QQuickRenderTarget()
    : d(new QQuickRenderTargetPrivate)
{
}

/*!
    \internal
 */
void QQuickRenderTarget::detach()
{
    qAtomicDetach(d);
}

/*!
    \internal
 */
QQuickRenderTarget::QQuickRenderTarget(const QQuickRenderTarget &other)
    : d(other.d)
{
    d->ref.ref();
}

/*!
    \internal
 */
QQuickRenderTarget &QQuickRenderTarget::operator=(const QQuickRenderTarget &other)
{
    qAtomicAssign(d, other.d);
    return *this;
}

/*!
    Destructor.
 */
QQuickRenderTarget::~QQuickRenderTarget()
{
    if (!d->ref.deref())
        delete d;
}

/*!
    \return true if this QQuickRenderTarget is default constructed, referencing
    no native objects.
 */
bool QQuickRenderTarget::isNull() const
{
    return d->type == QQuickRenderTargetPrivate::Type::Null;
}

/*!
    \return the device pixel ratio for the render target. This is the ratio
    between \e{device pixels} and \e{device independent pixels}.

    The default device pixel ratio is 1.0.

    \since 6.3

    \sa setDevicePixelRatio()
*/
qreal QQuickRenderTarget::devicePixelRatio() const
{
    return d->devicePixelRatio;
}

/*!
    Sets the device pixel ratio for this render target to \a ratio. This is
    the ratio between \e{device pixels} and \e{device independent pixels}.

    Note that the specified device pixel ratio value will be ignored if
    QQuickRenderControl::renderWindow() is re-implemented to return a valid
    QWindow.

    \since 6.3

    \sa devicePixelRatio()
*/
void QQuickRenderTarget::setDevicePixelRatio(qreal ratio)
{
    if (d->devicePixelRatio == ratio)
        return;

    detach();
    d->devicePixelRatio = ratio;
}

/*!
    \return Returns whether the render target is mirrored vertically.

    The default value is \c {false}.

    \since 6.4

    \sa setMirrorVertically()
*/
bool QQuickRenderTarget::mirrorVertically() const
{
    return d->mirrorVertically;
}


/*!
    Sets the size of the render target contents should be mirrored vertically to
    \a enable when drawing. This allows easy integration of third-party rendering
    code that does not follow the standard expectations.

    \note This function should not be used when using the \c software backend.

    \since 6.4

    \sa mirrorVertically()
 */
void QQuickRenderTarget::setMirrorVertically(bool enable)
{
    if (d->mirrorVertically == enable)
        return;

    detach();
    d->mirrorVertically = enable;
}

/*!
    \return a new QQuickRenderTarget referencing an OpenGL texture object
    specified by \a textureId.

    \a format specifies the native internal format of the
    texture. Only texture formats that are supported by Qt's rendering
    infrastructure should be used.

    \a pixelSize specifies the size of the image, in pixels. Currently only 2D
    textures are supported.

    \a sampleCount specific the number of samples. 0 or 1 means no
    multisampling, while a value like 4 or 8 states that the native object is a
    multisample texture.

    The texture is used as the first color attachment of the render target used
    by the Qt Quick scenegraph. A depth-stencil buffer, if applicable, is
    created and used automatically.

    The OpenGL object name \a textureId must be a valid name in the rendering
    context used by the Qt Quick scenegraph.

    \note the resulting QQuickRenderTarget does not own any native resources,
    it merely contains references and the associated metadata of the size and
    sample count. It is the caller's responsibility to ensure that the native
    resource exists as long as necessary.

    \since 6.4

    \sa QQuickWindow::setRenderTarget(), QQuickRenderControl
 */
#if QT_CONFIG(opengl) || defined(Q_QDOC)
QQuickRenderTarget QQuickRenderTarget::fromOpenGLTexture(uint textureId, uint format,
                                                         const QSize &pixelSize, int sampleCount)
{
    QQuickRenderTarget rt;
    QQuickRenderTargetPrivate *d = QQuickRenderTargetPrivate::get(&rt);

    if (!textureId) {
        qWarning("QQuickRenderTarget: textureId is invalid");
        return rt;
    }

    if (pixelSize.isEmpty()) {
        qWarning("QQuickRenderTarget: Cannot create with empty size");
        return rt;
    }

    d->type = QQuickRenderTargetPrivate::Type::NativeTexture;
    d->pixelSize = pixelSize;
    d->sampleCount = qMax(1, sampleCount);

    auto rhiFormat = QSGRhiSupport::toRhiTextureFormatFromGL(format);
    d->u.nativeTexture = { textureId, 0, uint(rhiFormat), 0 };

    return rt;
}

/*!
    \overload

    \return a new QQuickRenderTarget referencing an OpenGL texture
    object specified by \a textureId. The texture is assumed to have a
    format of GL_RGBA (GL_RGBA8).

    \a pixelSize specifies the size of the image, in pixels. Currently
    only 2D textures are supported.

    \a sampleCount specific the number of samples. 0 or 1 means no
    multisampling, while a value like 4 or 8 states that the native
    object is a multisample texture.

    The texture is used as the first color attachment of the render target used
    by the Qt Quick scenegraph. A depth-stencil buffer, if applicable, is
    created and used automatically.

    The OpenGL object name \a textureId must be a valid name in the rendering
    context used by the Qt Quick scenegraph.

    \note the resulting QQuickRenderTarget does not own any native resources,
    it merely contains references and the associated metadata of the size and
    sample count. It is the caller's responsibility to ensure that the native
    resource exists as long as necessary.

    \sa QQuickWindow::setRenderTarget(), QQuickRenderControl
*/
QQuickRenderTarget QQuickRenderTarget::fromOpenGLTexture(uint textureId, const QSize &pixelSize, int sampleCount)
{
    return fromOpenGLTexture(textureId, 0, pixelSize, sampleCount);
}

/*!
    \return a new QQuickRenderTarget referencing an OpenGL renderbuffer object
    specified by \a renderbufferId.

    The renderbuffer will be used as the color attachment for the internal
    framebuffer object. This function is provided to allow targeting
    renderbuffers that are created by the application with some external buffer
    underneath, such as an EGLImageKHR. Once the application has called
    \l{https://www.khronos.org/registry/OpenGL/extensions/OES/OES_EGL_image.txt}{glEGLImageTargetRenderbufferStorageOES},
    the renderbuffer can be passed to this function.

    \a pixelSize specifies the size of the image, in pixels.

    \a sampleCount specific the number of samples. 0 or 1 means no
    multisampling, while a value like 4 or 8 states that the native object is a
    multisample renderbuffer.

    \note the resulting QQuickRenderTarget does not own any native resources,
    it merely contains references and the associated metadata of the size and
    sample count. It is the caller's responsibility to ensure that the native
    resource exists as long as necessary.

    \since 6.2

    \sa QQuickWindow::setRenderTarget(), QQuickRenderControl
 */
QQuickRenderTarget QQuickRenderTarget::fromOpenGLRenderBuffer(uint renderbufferId, const QSize &pixelSize, int sampleCount)
{
    QQuickRenderTarget rt;
    QQuickRenderTargetPrivate *d = QQuickRenderTargetPrivate::get(&rt);

    if (!renderbufferId) {
        qWarning("QQuickRenderTarget: renderbufferId is invalid");
        return rt;
    }

    if (pixelSize.isEmpty()) {
        qWarning("QQuickRenderTarget: Cannot create with empty size");
        return rt;
    }

    d->type = QQuickRenderTargetPrivate::Type::NativeRenderbuffer;
    d->pixelSize = pixelSize;
    d->sampleCount = qMax(1, sampleCount);
    d->u.nativeRenderbufferObject = renderbufferId;

    return rt;
}
#endif

/*!
    \return a new QQuickRenderTarget referencing a D3D11 texture object
    specified by \a texture.

    \a format specifies the DXGI_FORMAT of the texture. Only texture formats
    that are supported by Qt's rendering infrastructure should be used.

    \a pixelSize specifies the size of the image, in pixels. Currently only 2D
    textures are supported.

    \a sampleCount specific the number of samples. 0 or 1 means no
    multisampling, while a value like 4 or 8 states that the native object is a
    multisample texture.

    The texture is used as the first color attachment of the render target used
    by the Qt Quick scenegraph. A depth-stencil buffer, if applicable, is
    created and used automatically.

    \note the resulting QQuickRenderTarget does not own any native resources,
    it merely contains references and the associated metadata of the size and
    sample count. It is the caller's responsibility to ensure that the native
    resource exists as long as necessary.

    \since 6.4

    \sa QQuickWindow::setRenderTarget(), QQuickRenderControl
 */
#if defined(Q_OS_WIN) || defined(Q_QDOC)
QQuickRenderTarget QQuickRenderTarget::fromD3D11Texture(void *texture, uint format,
                                                        const QSize &pixelSize, int sampleCount)
{
    QQuickRenderTarget rt;
    QQuickRenderTargetPrivate *d = QQuickRenderTargetPrivate::get(&rt);

    if (!texture) {
        qWarning("QQuickRenderTarget: texture is null");
        return rt;
    }

    if (pixelSize.isEmpty()) {
        qWarning("QQuickRenderTarget: Cannot create with empty size");
        return rt;
    }

    d->type = QQuickRenderTargetPrivate::Type::NativeTexture;
    d->pixelSize = pixelSize;
    d->sampleCount = qMax(1, sampleCount);

    QRhiTexture::Flags flags;
    auto rhiFormat = QSGRhiSupport::toRhiTextureFormatFromDXGI(format, &flags);
    d->u.nativeTexture = { quint64(texture), 0, uint(rhiFormat), uint(flags) };

    return rt;
}

/*!
    \overload

    \return a new QQuickRenderTarget referencing a D3D11 texture
    object specified by \a texture. The texture is assumed to have a
    format of DXGI_FORMAT_R8G8B8A8_UNORM.

    \a pixelSize specifies the size of the image, in pixels. Currently only 2D
    textures are supported.

    \a sampleCount specific the number of samples. 0 or 1 means no
    multisampling, while a value like 4 or 8 states that the native object is a
    multisample texture.

    The texture is used as the first color attachment of the render target used
    by the Qt Quick scenegraph. A depth-stencil buffer, if applicable, is
    created and used automatically.

    \note the resulting QQuickRenderTarget does not own any native resources,
    it merely contains references and the associated metadata of the size and
    sample count. It is the caller's responsibility to ensure that the native
    resource exists as long as necessary.

    \sa QQuickWindow::setRenderTarget(), QQuickRenderControl
*/
QQuickRenderTarget QQuickRenderTarget::fromD3D11Texture(void *texture, const QSize &pixelSize, int sampleCount)
{
    return fromD3D11Texture(texture, 0 /* DXGI_FORMAT_UNKNOWN */, pixelSize, sampleCount);
}

/*!
    \return a new QQuickRenderTarget referencing a D3D12 texture object
    specified by \a texture.

    \a resourceState must a valid bitmask with bits from D3D12_RESOURCE_STATES,
    specifying the resource's current state.

    \a format specifies the DXGI_FORMAT of the texture. Only texture formats
    that are supported by Qt's rendering infrastructure should be used.

    \a pixelSize specifies the size of the image, in pixels. Currently only 2D
    textures are supported.

    \a sampleCount specific the number of samples. 0 or 1 means no
    multisampling, while a value like 4 or 8 states that the native object is a
    multisample texture.

    The texture is used as the first color attachment of the render target used
    by the Qt Quick scenegraph. A depth-stencil buffer, if applicable, is
    created and used automatically.

    \note the resulting QQuickRenderTarget does not own any native resources,
    it merely contains references and the associated metadata of the size and
    sample count. It is the caller's responsibility to ensure that the native
    resource exists as long as necessary.

    \since 6.6

    \sa QQuickWindow::setRenderTarget(), QQuickRenderControl
 */
QQuickRenderTarget QQuickRenderTarget::fromD3D12Texture(void *texture,
                                                        int resourceState,
                                                        uint format,
                                                        const QSize &pixelSize,
                                                        int sampleCount)
{
    QQuickRenderTarget rt;
    QQuickRenderTargetPrivate *d = QQuickRenderTargetPrivate::get(&rt);

    if (!texture) {
        qWarning("QQuickRenderTarget: texture is null");
        return rt;
    }

    if (pixelSize.isEmpty()) {
        qWarning("QQuickRenderTarget: Cannot create with empty size");
        return rt;
    }

    d->type = QQuickRenderTargetPrivate::Type::NativeTexture;
    d->pixelSize = pixelSize;
    d->sampleCount = qMax(1, sampleCount);

    QRhiTexture::Flags flags;
    auto rhiFormat = QSGRhiSupport::toRhiTextureFormatFromDXGI(format, &flags);
    d->u.nativeTexture = { quint64(texture), resourceState, uint(rhiFormat), uint(flags) };

    return rt;
}
#endif

/*!
    \return a new QQuickRenderTarget referencing a Metal texture object
    specified by \a texture.

    \a format specifies the MTLPixelFormat of the texture. Only texture formats
    that are supported by Qt's rendering infrastructure should be used.

    \a pixelSize specifies the size of the image, in pixels. Currently only 2D
    textures are supported.

    \a sampleCount specific the number of samples. 0 or 1 means no
    multisampling, while a value like 4 or 8 states that the native object is a
    multisample texture.

    The texture is used as the first color attachment of the render target used
    by the Qt Quick scenegraph. A depth-stencil buffer, if applicable, is
    created and used automatically.

    \note the resulting QQuickRenderTarget does not own any native resources,
    it merely contains references and the associated metadata of the size and
    sample count. It is the caller's responsibility to ensure that the native
    resource exists as long as necessary.

    \since 6.4

    \sa QQuickWindow::setRenderTarget(), QQuickRenderControl
 */
#if defined(Q_OS_MACOS) || defined(Q_OS_IOS) || defined(Q_QDOC)
QQuickRenderTarget QQuickRenderTarget::fromMetalTexture(MTLTexture *texture, uint format,
                                                        const QSize &pixelSize, int sampleCount)
{
    QQuickRenderTarget rt;
    QQuickRenderTargetPrivate *d = QQuickRenderTargetPrivate::get(&rt);

    if (!texture) {
        qWarning("QQuickRenderTarget: texture is null");
        return rt;
    }

    if (pixelSize.isEmpty()) {
        qWarning("QQuickRenderTarget: Cannot create with empty size");
        return rt;
    }

    d->type = QQuickRenderTargetPrivate::Type::NativeTexture;
    d->pixelSize = pixelSize;
    d->sampleCount = qMax(1, sampleCount);

    QRhiTexture::Flags flags;
    auto rhiFormat = QSGRhiSupport::toRhiTextureFormatFromMetal(format, &flags);
    d->u.nativeTexture = { quint64(texture), 0, uint(rhiFormat), uint(flags) };

    return rt;
}

/*!
    \overload

    \return a new QQuickRenderTarget referencing a Metal texture object
    specified by \a texture. The texture is assumed to have a format of
    MTLPixelFormatRGBA8Unorm.

    \a pixelSize specifies the size of the image, in pixels. Currently only 2D
    textures are supported.

    \a sampleCount specific the number of samples. 0 or 1 means no
    multisampling, while a value like 4 or 8 states that the native object is a
    multisample texture.

    The texture is used as the first color attachment of the render target used
    by the Qt Quick scenegraph. A depth-stencil buffer, if applicable, is
    created and used automatically.

    \note the resulting QQuickRenderTarget does not own any native resources,
    it merely contains references and the associated metadata of the size and
    sample count. It is the caller's responsibility to ensure that the native
    resource exists as long as necessary.

    \sa QQuickWindow::setRenderTarget(), QQuickRenderControl
*/
QQuickRenderTarget QQuickRenderTarget::fromMetalTexture(MTLTexture *texture, const QSize &pixelSize, int sampleCount)
{
    return fromMetalTexture(texture, 0 /* MTLPixelFormatInvalid */, pixelSize, sampleCount);
}
#endif

/*!
    \return a new QQuickRenderTarget referencing a Vulkan image object
    specified by \a image. The current \a layout of the image must be provided
    as well.

    \a format specifies the VkFormat of the image. Only image formats that are
    supported by Qt's rendering infrastructure should be used.

    \a pixelSize specifies the size of the image, in pixels. Currently only 2D
    textures are supported.

    \a sampleCount specific the number of samples. 0 or 1 means no
    multisampling, while a value like 4 or 8 states that the native object is a
    multisample texture.

    The image is used as the first color attachment of the render target used
    by the Qt Quick scenegraph. A depth-stencil buffer, if applicable, is
    created and used automatically.

    \note the resulting QQuickRenderTarget does not own any native resources,
    it merely contains references and the associated metadata of the size and
    sample count. It is the caller's responsibility to ensure that the native
    resource exists as long as necessary.

    \since 6.4

    \sa QQuickWindow::setRenderTarget(), QQuickRenderControl
 */
#if QT_CONFIG(vulkan) || defined(Q_QDOC)
QQuickRenderTarget QQuickRenderTarget::fromVulkanImage(VkImage image, VkImageLayout layout, VkFormat format,
                                                       const QSize &pixelSize, int sampleCount)
{
    QQuickRenderTarget rt;
    QQuickRenderTargetPrivate *d = QQuickRenderTargetPrivate::get(&rt);

    if (image == VK_NULL_HANDLE) {
        qWarning("QQuickRenderTarget: image is invalid");
        return rt;
    }

    if (pixelSize.isEmpty()) {
        qWarning("QQuickRenderTarget: Cannot create with empty size");
        return rt;
    }

    d->type = QQuickRenderTargetPrivate::Type::NativeTexture;
    d->pixelSize = pixelSize;
    d->sampleCount = qMax(1, sampleCount);

    QRhiTexture::Flags flags;
    auto rhiFormat = QSGRhiSupport::toRhiTextureFormatFromVulkan(format, &flags);
    d->u.nativeTexture = { quint64(image), layout, uint(rhiFormat), uint(flags) };

    return rt;
}

/*!
    \overload

    \return a new QQuickRenderTarget referencing a Vulkan image object specified
    by \a image. The image is assumed to have a format of
    VK_FORMAT_R8G8B8A8_UNORM.

    \a pixelSize specifies the size of the image, in pixels. Currently only 2D
    textures are supported.

    \a sampleCount specific the number of samples. 0 or 1 means no
    multisampling, while a value like 4 or 8 states that the native object is a
    multisample texture.

    The texture is used as the first color attachment of the render target used
    by the Qt Quick scenegraph. A depth-stencil buffer, if applicable, is
    created and used automatically.

    \note the resulting QQuickRenderTarget does not own any native resources,
    it merely contains references and the associated metadata of the size and
    sample count. It is the caller's responsibility to ensure that the native
    resource exists as long as necessary.

    \sa QQuickWindow::setRenderTarget(), QQuickRenderControl
*/
QQuickRenderTarget QQuickRenderTarget::fromVulkanImage(VkImage image, VkImageLayout layout, const QSize &pixelSize, int sampleCount)
{
    return fromVulkanImage(image, layout, VK_FORMAT_UNDEFINED, pixelSize, sampleCount);
}
#endif

/*!
    \return a new QQuickRenderTarget referencing an existing \a renderTarget.

    \a renderTarget will in most cases be a QRhiTextureRenderTarget, which
    allows directing the Qt Quick scene's rendering into a QRhiTexture.

    \note the resulting QQuickRenderTarget does not own \a renderTarget and any
    underlying native resources, it merely contains references and the
    associated metadata of the size and sample count. It is the caller's
    responsibility to ensure that the referenced resources exists as long as
    necessary.

    \since 6.6

    \sa QQuickWindow::setRenderTarget(), QQuickRenderControl
*/
QQuickRenderTarget QQuickRenderTarget::fromRhiRenderTarget(QRhiRenderTarget *renderTarget)
{
    QQuickRenderTarget rt;
    QQuickRenderTargetPrivate *d = QQuickRenderTargetPrivate::get(&rt);

    if (!renderTarget) {
        qWarning("QQuickRenderTarget: Needs a valid QRhiRenderTarget");
        return rt;
    }

    d->type = QQuickRenderTargetPrivate::Type::RhiRenderTarget;
    d->pixelSize = renderTarget->pixelSize();
    d->sampleCount = renderTarget->sampleCount();
    d->u.rhiRt = renderTarget;

    return rt;
}

/*!
    \return a new QQuickRenderTarget referencing a paint device object
    specified by \a device.

    This option of redirecting rendering to a QPaintDevice is available only
    when running with the \c software backend of Qt Quick.

    \note The QQuickRenderTarget does not take ownship of \a device, it is the
    caller's responsibility to ensure the object exists as long as necessary.

    \since 6.4

    \sa QQuickWindow::setRenderTarget(), QQuickRenderControl
 */
QQuickRenderTarget QQuickRenderTarget::fromPaintDevice(QPaintDevice *device)
{
    QQuickRenderTarget rt;
    QQuickRenderTargetPrivate *d = QQuickRenderTargetPrivate::get(&rt);

    d->type = QQuickRenderTargetPrivate::Type::PaintDevice;
    d->pixelSize = QSize(device->width(), device->height());
    d->u.paintDevice = device;

    return rt;
}

/*!
    \fn bool QQuickRenderTarget::operator==(const QQuickRenderTarget &a, const QQuickRenderTarget &b) noexcept
    \return true if \a a and \a b refer to the same set of native objects and
    have matching associated data (size, sample count).
*/
/*!
    \fn bool QQuickRenderTarget::operator!=(const QQuickRenderTarget &a, const QQuickRenderTarget &b) noexcept

    \return true if \a a and \a b refer to a different set of native objects,
    or the associated data (size, sample count) does not match.
*/

/*!
    \internal
*/
bool QQuickRenderTarget::isEqual(const QQuickRenderTarget &other) const noexcept
{
    if (d->type != other.d->type
            || d->pixelSize != other.d->pixelSize
            || d->devicePixelRatio != other.d->devicePixelRatio
            || d->sampleCount != other.d->sampleCount
            || d->mirrorVertically != other.d->mirrorVertically)
    {
        return false;
    }

    switch (d->type) {
    case QQuickRenderTargetPrivate::Type::Null:
        break;
    case QQuickRenderTargetPrivate::Type::NativeTexture:
        if (d->u.nativeTexture.object != other.d->u.nativeTexture.object
                || d->u.nativeTexture.layoutOrState != other.d->u.nativeTexture.layoutOrState
                || d->u.nativeTexture.rhiFormat != other.d->u.nativeTexture.rhiFormat
                || d->u.nativeTexture.rhiFlags != other.d->u.nativeTexture.rhiFlags)
            return false;
        break;
    case QQuickRenderTargetPrivate::Type::NativeRenderbuffer:
        if (d->u.nativeRenderbufferObject != other.d->u.nativeRenderbufferObject)
            return false;
        break;
    case QQuickRenderTargetPrivate::Type::RhiRenderTarget:
        if (d->u.rhiRt != other.d->u.rhiRt)
            return false;
        break;
    case QQuickRenderTargetPrivate::Type::PaintDevice:
        if (d->u.paintDevice != other.d->u.paintDevice)
            return false;
        break;
    default:
        break;
    }

    return true;
}

static bool createRhiRenderTarget(const QRhiColorAttachment &colorAttachment,
                                  const QSize &pixelSize,
                                  int sampleCount,
                                  QRhi *rhi,
                                  QQuickWindowRenderTarget *dst)
{
    std::unique_ptr<QRhiRenderBuffer> depthStencil(rhi->newRenderBuffer(QRhiRenderBuffer::DepthStencil, pixelSize, sampleCount));
    if (!depthStencil->create()) {
        qWarning("Failed to build depth-stencil buffer for QQuickRenderTarget");
        return false;
    }

    QRhiTextureRenderTargetDescription rtDesc(colorAttachment);
    rtDesc.setDepthStencilBuffer(depthStencil.get());
    std::unique_ptr<QRhiTextureRenderTarget> rt(rhi->newTextureRenderTarget(rtDesc));
    std::unique_ptr<QRhiRenderPassDescriptor> rp(rt->newCompatibleRenderPassDescriptor());
    rt->setRenderPassDescriptor(rp.get());

    if (!rt->create()) {
        qWarning("Failed to build texture render target for QQuickRenderTarget");
        return false;
    }

    dst->renderTarget = rt.release();
    dst->rpDesc = rp.release();
    dst->depthStencil = depthStencil.release();
    dst->owns = true; // ownership of the native resource itself is not transferred but the QRhi objects are on us now

    return true;
}

bool QQuickRenderTargetPrivate::resolve(QRhi *rhi, QQuickWindowRenderTarget *dst)
{
    switch (type) {
    case Type::Null:
        dst->renderTarget = nullptr;
        dst->paintDevice = nullptr;
        dst->owns = false;
        return true;

    case Type::NativeTexture:
    {
        const auto format = u.nativeTexture.rhiFormat == QRhiTexture::UnknownFormat ? QRhiTexture::RGBA8
                                                                                    : QRhiTexture::Format(u.nativeTexture.rhiFormat);
        const auto flags = QRhiTexture::RenderTarget | QRhiTexture::Flags(u.nativeTexture.rhiFlags);
        std::unique_ptr<QRhiTexture> texture(rhi->newTexture(format, pixelSize, sampleCount, flags));
        if (!texture->createFrom({ u.nativeTexture.object, u.nativeTexture.layoutOrState })) {
            qWarning("Failed to build wrapper texture for QQuickRenderTarget");
            return false;
        }
        QRhiColorAttachment att(texture.get());
        if (!createRhiRenderTarget(att, pixelSize, sampleCount, rhi, dst))
            return false;
        dst->texture = texture.release();
    }
        return true;

    case Type::NativeRenderbuffer:
    {
        std::unique_ptr<QRhiRenderBuffer> renderbuffer(rhi->newRenderBuffer(QRhiRenderBuffer::Color, pixelSize, sampleCount));
        if (!renderbuffer->createFrom({ u.nativeRenderbufferObject })) {
            qWarning("Failed to build wrapper renderbuffer for QQuickRenderTarget");
            return false;
        }
        QRhiColorAttachment att(renderbuffer.get());
        if (!createRhiRenderTarget(att, pixelSize, sampleCount, rhi, dst))
            return false;
        dst->renderBuffer = renderbuffer.release();
    }
        return true;

    case Type::RhiRenderTarget:
        dst->renderTarget = u.rhiRt;
        dst->rpDesc = u.rhiRt->renderPassDescriptor(); // just for QQuickWindowRenderTarget::reset()
        dst->owns = false;
        return true;
    case Type::PaintDevice:
        dst->paintDevice = u.paintDevice;
        dst->owns = false;
        return true;

    default:
        break;
    }
    return false;
}

QT_END_NAMESPACE
