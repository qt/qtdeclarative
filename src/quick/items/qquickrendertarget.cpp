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
      customDepthTexture(other.customDepthTexture),
      mirrorVertically(other.mirrorVertically),
      multisampleResolve(other.multisampleResolve)
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
    \return the currently set depth texture or, in most cases, \nullptr.

    The value is only non-null when setDepthTexture() was called.

    \since 6.8
 */
QRhiTexture *QQuickRenderTarget::depthTexture() const
{
    return d->customDepthTexture;
}

/*!
    Requests using the given \a texture as the depth or depth-stencil buffer.
    Ownership of \a texture is not taken.

    The request is only taken into account when relevant. For example, calling
    this function has no effect with fromRhiRenderTarget(), fromPaintDevice(),
    or fromOpenGLRenderBuffer().

    Normally a depth-stencil buffer is created automatically, transparently to
    the user of QQuickRenderTarget. Therefore, there is no need to call this
    function in most cases when working with QQuickRenderTarget. In special
    circumstances, it can however become essential to be able to provide a
    texture to render depth (or depth and stencil) data into, instead of letting
    Qt Quick create its own intermediate textures or buffers. An example of this
    is \l{https://www.khronos.org/openxr/}{OpenXR} and its extensions such as
    \l{https://registry.khronos.org/OpenXR/specs/1.0/html/xrspec.html#XR_KHR_composition_layer_depth}{XR_KHR_composition_layer_depth}.
    In order to "submit the depth buffer" to the XR compositor, one has to, in
    practice, retrieve an already created depth (depth-stencil) texture from
    OpenXR (from the XrSwapchain) and use that texture as the render target for
    depth data. That would not be possible without this function.

    \note The \a texture is always expected to be a non-multisample 2D texture
    or texture array (for multiview). If MSAA is involved, the samples are
    resolved into \a texture at the end of the render pass, regardless of having
    the MultisampleResolve flag set or not. MSAA is only supported for depth
    (depth-stencil) textures when the underlying 3D API supports this, and this
    support is not universally available. See \l{QRhi::ResolveDepthStencil}{the
    relevant QRhi feature flag} for details. When this is not supported and
    multisampling is requested in combination with a custom depth texture, \a
    texture is not going to be touched during rendering and a warning is
    printed.

    \since 6.8

    \note When it comes to OpenGL and OpenGL ES, using depth textures is not
    functional on OpenGL ES 2.0 and requires at least OpenGL ES 3.0. Multisample
    (MSAA) support is not available without at least OpenGL ES 3.1, or OpenGL
    3.0 on desktop.
 */
void QQuickRenderTarget::setDepthTexture(QRhiTexture *texture)
{
    if (d->customDepthTexture == texture)
        return;

    detach();
    d->customDepthTexture = texture;
}

/*!
    \enum QQuickRenderTarget::Flag
    Flags for the static QQuickRenderTarget constructor functions.

    \value MultisampleResolve Indicates that the \c sampleCount argument is not
    the number of samples for the provided texture (and that the texture is
    still a non-multisample texture), but rather the desired samples for
    multisample antialiasing. Triggers automatically creating and managing an
    intermediate multisample texture (or texture array) as the color buffer,
    transparently to the application. The samples are resolved into the provided
    texture at the end of the render pass automatically. When this flag is not
    set, and the \c sampleCount argument is greater than 1, it implies the
    provided texture is multisample. The flag has no effect is the
    \c sampleCount is 1 (indicating that multisampling is not involved).

    \since 6.8
*/

/*!
    \return a new QQuickRenderTarget referencing an OpenGL texture object
    specified by \a textureId.

    \a format specifies the native internal format of the
    texture. Only texture formats that are supported by Qt's rendering
    infrastructure should be used.

    \a pixelSize specifies the size of the image, in pixels. Currently only 2D
    textures are supported.

    \a sampleCount specifies the number of samples. 0 or 1 means no
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

    QRhiTexture::Flags formatFlags;
    QRhiTexture::Format rhiFormat = QSGRhiSupport::toRhiTextureFormatFromGL(format, &formatFlags);
    d->u.nativeTexture = { textureId, 0, uint(rhiFormat), uint(formatFlags), uint(rhiFormat), uint(formatFlags) };

    return rt;
}

/*!
    \overload

    \return a new QQuickRenderTarget referencing an OpenGL texture
    object specified by \a textureId. The texture is assumed to have a
    format of GL_RGBA (GL_RGBA8).

    \a pixelSize specifies the size of the image, in pixels. Currently
    only 2D textures are supported.

    \a sampleCount specifies the number of samples. 0 or 1 means no
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
    \overload

    \return a new QQuickRenderTarget referencing an OpenGL 2D texture or texture
    array object specified by \a textureId.

    \a format specifies the native internal format of the texture. Only texture
    formats that are supported by Qt's rendering infrastructure should be used.

    \a pixelSize specifies the size of the image, in pixels. Currently only 2D
    textures and 2D texture arrays are supported.

    \a sampleCount specifies the number of samples. 0 or 1 means no
    multisampling, while a value like 4 or 8 states that the native object is a
    multisample texture, except when \a flags contains \l MultisampleResolve. In
    that case, \a textureId is assumed to be a non-multisample 2D texture or 2D
    texture array, and \a sampleCount defines the number of samples desired. The
    resulting QQuickRenderTarget will use an intermediate, automatically created
    multisample texture (or texture array) as its color attachment, and will
    resolve the samples into \a textureId. This is the recommended approach to
    perform MSAA when the native OpenGL texture is not already multisample.

    When \a arraySize is greater than 1, it implies multiview rendering
    (\l{https://registry.khronos.org/OpenGL/extensions/OVR/OVR_multiview.txt}{GL_OVR_multiview},
    \l QRhiColorAttachment::setMultiViewCount()), which can be relevant with
    VR/AR especially. In this case \a arraySize is the number of views,
    typically \c 2. See \l QSGMaterial::viewCount() for details on enabling
    multiview rendering within the Qt Quick scenegraph.

    A depth-stencil buffer, if applicable, is created and used automatically.
    When the color buffer is multisample, the depth-stencil buffer will
    automatically be multisample too. For multiview rendering, the depth-stencil
    texture will automatically be made an array with a matching \a arraySize.

    The OpenGL object name \a textureId must be a valid 2D texture name in the
    rendering context used by the Qt Quick scenegraph. When \a arraySize is
    greater than 1, \a textureId must be a valid 2D texture array name.

    \note the resulting QQuickRenderTarget does not own any native resources, it
    merely contains references and the associated metadata of the size and
    sample count. It is the caller's responsibility to ensure that the native
    resource exists as long as necessary.

    \since 6.8

    \note The implementation of this overload is not compatible with OpenGL ES
    2.0 or 3.0, and requires OpenGL ES 3.1 at minimum. (or OpenGL 3.0 on
    desktop)

    \sa QQuickWindow::setRenderTarget(), QQuickRenderControl, fromOpenGLTexture()
 */
QQuickRenderTarget QQuickRenderTarget::fromOpenGLTexture(uint textureId, uint format, QSize pixelSize, int sampleCount, int arraySize, Flags flags)
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

    QRhiTexture::Flags formatFlags;
    QRhiTexture::Format rhiFormat = QSGRhiSupport::toRhiTextureFormatFromGL(format, &formatFlags);

    d->pixelSize = pixelSize;
    d->sampleCount = qMax(1, sampleCount);
    d->multisampleResolve = flags.testFlag(Flag::MultisampleResolve);

    if (arraySize <= 1) {
        d->type = QQuickRenderTargetPrivate::Type::NativeTexture;
        d->u.nativeTexture = { textureId, 0, uint(rhiFormat), uint(formatFlags), uint(rhiFormat), uint(formatFlags) };
    } else {
        d->type = QQuickRenderTargetPrivate::Type::NativeTextureArray;
        d->u.nativeTextureArray = { textureId, 0, arraySize, uint(rhiFormat), uint(formatFlags), uint(rhiFormat), uint(formatFlags) };
    }

    return rt;
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

    \a sampleCount specifies the number of samples. 0 or 1 means no
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

    \a sampleCount specifies the number of samples. 0 or 1 means no
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

    QRhiTexture::Flags formatFlags;
    QRhiTexture::Format rhiFormat = QSGRhiSupport::toRhiTextureFormatFromDXGI(format, &formatFlags);
    d->u.nativeTexture = { quint64(texture), 0, uint(rhiFormat), uint(formatFlags), uint(rhiFormat), uint(formatFlags) };

    return rt;
}

/*!
    \overload

    \return a new QQuickRenderTarget referencing a D3D11 texture
    object specified by \a texture. The texture is assumed to have a
    format of DXGI_FORMAT_R8G8B8A8_UNORM.

    \a pixelSize specifies the size of the image, in pixels. Currently only 2D
    textures are supported.

    \a sampleCount specifies the number of samples. 0 or 1 means no
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
    \overload

    \return a new QQuickRenderTarget referencing a D3D11 texture object
    specified by \a texture.

    \a format specifies the DXGI_FORMAT of the texture. Only texture formats
    that are supported by Qt's rendering infrastructure should be used.

    \a pixelSize specifies the size of the image, in pixels. Currently only 2D
    textures are supported.

    \a sampleCount specifies the number of samples. 0 or 1 means no
    multisampling, while a value like 4 or 8 states that the native object is a
    multisample texture, except when \a flags contains \l MultisampleResolve. In
    that case, \a texture is assumed to be a non-multisample 2D texture and \a
    sampleCount defines the number of samples desired. The resulting
    QQuickRenderTarget will use an intermediate, automatically created
    multisample texture as its color attachment, and will resolve the samples
    into \a texture. This is the recommended approach to perform MSAA when the
    native texture is not already multisample.

    The texture is used as the first color attachment of the render target used
    by the Qt Quick scenegraph. A depth-stencil buffer, if applicable, is
    created and used automatically. When the color buffer is multisample, the
    depth-stencil buffer will automatically be multisample too.

    \note the resulting QQuickRenderTarget does not own any native resources, it
    merely contains references and the associated metadata of the size and
    sample count. It is the caller's responsibility to ensure that the native
    resource exists as long as necessary.

    \since 6.8

    \sa QQuickWindow::setRenderTarget(), QQuickRenderControl, fromD3D11Texture()
 */
QQuickRenderTarget QQuickRenderTarget::fromD3D11Texture(void *texture, uint format, QSize pixelSize, int sampleCount, Flags flags)
{
    QQuickRenderTarget rt = fromD3D11Texture(texture, format, pixelSize, sampleCount);
    QQuickRenderTargetPrivate::get(&rt)->multisampleResolve = flags.testFlag(Flag::MultisampleResolve);
    return rt;
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

    \a sampleCount specifies the number of samples. 0 or 1 means no
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

    QRhiTexture::Flags formatFlags;
    QRhiTexture::Format rhiFormat = QSGRhiSupport::toRhiTextureFormatFromDXGI(format, &formatFlags);
    d->u.nativeTexture = { quint64(texture), resourceState, uint(rhiFormat), uint(formatFlags), uint(rhiFormat), uint(formatFlags) };

    return rt;
}

/*!
    \overload

    \return a new QQuickRenderTarget referencing a D3D12 2D texture or 2D
    texture array object specified by \a texture.

    \a resourceState must a valid bitmask with bits from D3D12_RESOURCE_STATES,
    specifying the resource's current state.

    \a format specifies the DXGI_FORMAT of the texture. Only texture formats
    that are supported by Qt's rendering infrastructure should be used.

    \a viewFormat is the DXGI_FORMAT used for the render target view (RTV).
    Often the same as \a format. Functional only when
    \l{https://microsoft.github.io/DirectX-Specs/d3d/RelaxedCasting.html}{relaxed
    format casting} is supported by the driver, the argument is ignored otherwise.
    In practice support is expected to be always available on Windows 10 1703
    and newer.

    \a pixelSize specifies the size of the image, in pixels. Currently only 2D
    textures and 2D texture arrays are supported.

    \a sampleCount specifies the number of samples. 0 or 1 means no
    multisampling, while a value like 4 or 8 states that the native object is a
    multisample texture, except when \a flags contains \l MultisampleResolve. In
    that case, \a texture is assumed to be a non-multisample 2D texture or 2D
    texture array, and \a sampleCount defines the number of samples desired. The
    resulting QQuickRenderTarget will use an intermediate, automatically created
    multisample texture (or texture array) as its color attachment, and will
    resolve the samples into \a texture. This is the recommended approach to
    perform MSAA when the native D3D12 texture is not already multisample.

    The number of array elements (layers) is given in \a arraySize. When greater
    than 1, it implies multiview rendering
    (\l{https://microsoft.github.io/DirectX-Specs/d3d/ViewInstancing.html}{view
    instancing}), which can be relevant with VR/AR especially. \a arraySize is
    the number of views, typically \c 2. See \l QSGMaterial::viewCount() for
    details on enabling multiview rendering within the Qt Quick scenegraph.

    The texture is used as the first color attachment of the render target used
    by the Qt Quick scenegraph.  A depth-stencil buffer, if applicable, is
    created and used automatically. When the color buffer is multisample, the
    depth-stencil buffer will automatically be multisample too. For multiview
    rendering, the depth-stencil texture will automatically be made an array
    with a matching \a arraySize.

    \note the resulting QQuickRenderTarget does not own any native resources, it
    merely contains references and the associated metadata of the size and
    sample count. It is the caller's responsibility to ensure that the native
    resource exists as long as necessary.

    \since 6.8

    \sa QQuickWindow::setRenderTarget(), QQuickRenderControl
 */
QQuickRenderTarget QQuickRenderTarget::fromD3D12Texture(void *texture,
                                                        int resourceState,
                                                        uint format,
                                                        uint viewFormat,
                                                        QSize pixelSize,
                                                        int sampleCount,
                                                        int arraySize,
                                                        Flags flags)
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

    QRhiTexture::Flags formatFlags;
    QRhiTexture::Format rhiFormat = QSGRhiSupport::toRhiTextureFormatFromDXGI(format, &formatFlags);
    QRhiTexture::Flags viewFormatFlags;
    QRhiTexture::Format rhiViewFormat = QSGRhiSupport::toRhiTextureFormatFromDXGI(viewFormat, &viewFormatFlags);

    d->pixelSize = pixelSize;
    d->sampleCount = qMax(1, sampleCount);
    d->multisampleResolve = flags.testFlag(Flag::MultisampleResolve);

    if (arraySize <= 1) {
        d->type = QQuickRenderTargetPrivate::Type::NativeTexture;
        d->u.nativeTexture = { quint64(texture), resourceState, uint(rhiFormat), uint(formatFlags), uint(rhiViewFormat), uint(viewFormatFlags) };
    } else {
        d->type = QQuickRenderTargetPrivate::Type::NativeTextureArray;
        d->u.nativeTextureArray = { quint64(texture), resourceState, arraySize, uint(rhiFormat), uint(formatFlags), uint(rhiViewFormat), uint(viewFormatFlags) };
    }

    return rt;
}

#endif // Q_OS_WIN

/*!
    \return a new QQuickRenderTarget referencing a Metal texture object
    specified by \a texture.

    \a format specifies the MTLPixelFormat of the texture. Only texture formats
    that are supported by Qt's rendering infrastructure should be used.

    \a pixelSize specifies the size of the image, in pixels. Currently only 2D
    textures are supported.

    \a sampleCount specifies the number of samples. 0 or 1 means no
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
#if QT_CONFIG(metal) || defined(Q_QDOC)
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

    QRhiTexture::Flags formatFlags;
    QRhiTexture::Format rhiFormat = QSGRhiSupport::toRhiTextureFormatFromMetal(format, &formatFlags);
    d->u.nativeTexture = { quint64(texture), 0, uint(rhiFormat), uint(formatFlags), uint(rhiFormat), uint(formatFlags) };

    return rt;
}

/*!
    \overload

    \return a new QQuickRenderTarget referencing a Metal texture object
    specified by \a texture. The texture is assumed to have a format of
    MTLPixelFormatRGBA8Unorm.

    \a pixelSize specifies the size of the image, in pixels. Currently only 2D
    textures are supported.

    \a sampleCount specifies the number of samples. 0 or 1 means no
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

/*!
    \overload

    \return a new QQuickRenderTarget referencing a Metal 2D texture or 2D
    texture array given in \a texture.

    \a format specifies the MTLPixelFormat of the texture. Only texture formats
    that are supported by Qt's rendering infrastructure should be used.

    \a viewFormat is usually set to the same value as \a format. In some cases,
    such as when rendering into a texture with a \c{_SRGB} format and the
    implicit linear->sRGB conversion on shader writes is not wanted, the value
    can be different. Note however that the value may be ignored by Qt, when at
    run time QRhi reports that the \l{QRhi::TextureViewFormat} feature is
    unsupported.

    \a pixelSize specifies the size of the image, in pixels. Currently only 2D
    textures and 2D texture arrays are supported.

    \a sampleCount specifies the number of samples. 0 or 1 means no
    multisampling, while a value like 4 or 8 states that the native object is a
    multisample texture, except when \a flags contains \l MultisampleResolve. In
    that case, \a texture is assumed to be a non-multisample 2D texture or 2D
    texture array, and \a sampleCount defines the number of samples desired. The
    resulting QQuickRenderTarget will use an intermediate, automatically created
    multisample texture (or texture array) as its color attachment, and will
    resolve the samples into \a texture. This is the recommended approach to
    perform MSAA when the native Metal texture is not already multisample.

    The number of array elements (layers) is given in \a arraySize. When greater
    than 1, it implies multiview rendering, which can be relevant with VR/AR
    especially. \a arraySize is the number of views, typically \c 2. See
    \l QSGMaterial::viewCount() for details on enabling multiview rendering within
    the Qt Quick scenegraph.

    The texture is used as the first color attachment of the render target used
    by the Qt Quick scenegraph.  A depth-stencil buffer, if applicable, is
    created and used automatically. When the color buffer is multisample, the
    depth-stencil buffer will automatically be multisample too. For multiview
    rendering, the depth-stencil texture will automatically be made an array
    with a matching \a arraySize.

    \note the resulting QQuickRenderTarget does not own any native resources, it
    merely contains references and the associated metadata of the size and
    sample count. It is the caller's responsibility to ensure that the native
    resource exists as long as necessary.

    \since 6.8

    \sa QQuickWindow::setRenderTarget(), QQuickRenderControl
 */
QQuickRenderTarget QQuickRenderTarget::fromMetalTexture(MTLTexture *texture, uint format, uint viewFormat,
                                                        QSize pixelSize, int sampleCount, int arraySize, Flags flags)
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

    QRhiTexture::Flags formatFlags;
    QRhiTexture::Format rhiFormat = QSGRhiSupport::toRhiTextureFormatFromMetal(format, &formatFlags);
    QRhiTexture::Flags viewFormatFlags;
    QRhiTexture::Format rhiViewFormat = QSGRhiSupport::toRhiTextureFormatFromMetal(viewFormat, &viewFormatFlags);

    d->pixelSize = pixelSize;
    d->sampleCount = qMax(1, sampleCount);
    d->multisampleResolve = flags.testFlag(Flag::MultisampleResolve);

    if (arraySize <= 1) {
        d->type = QQuickRenderTargetPrivate::Type::NativeTexture;
        d->u.nativeTexture = { quint64(texture), 0, uint(rhiFormat), uint(formatFlags), uint(rhiViewFormat), uint(viewFormatFlags) };
    } else {
        d->type = QQuickRenderTargetPrivate::Type::NativeTextureArray;
        d->u.nativeTextureArray = { quint64(texture), 0, arraySize, uint(rhiFormat), uint(formatFlags), uint(rhiViewFormat), uint(viewFormatFlags) };
    }

    return rt;
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

    \a sampleCount specifies the number of samples. 0 or 1 means no
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
QQuickRenderTarget QQuickRenderTarget::fromVulkanImage(VkImage image, VkImageLayout layout, VkFormat format, const QSize &pixelSize, int sampleCount)
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

    QRhiTexture::Flags formatFlags;
    QRhiTexture::Format rhiFormat = QSGRhiSupport::toRhiTextureFormatFromVulkan(format, &formatFlags);
    d->u.nativeTexture = { quint64(image), layout, uint(rhiFormat), uint(formatFlags), uint(rhiFormat), uint(formatFlags) };

    return rt;
}

/*!
    \overload

    \return a new QQuickRenderTarget referencing a Vulkan image object specified
    by \a image. The image is assumed to have a format of
    VK_FORMAT_R8G8B8A8_UNORM.

    \a pixelSize specifies the size of the image, in pixels. Currently only 2D
    textures are supported.

    \a sampleCount specifies the number of samples. 0 or 1 means no
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

/*!
    \overload

    \return a new QQuickRenderTarget referencing a Vulkan image object
    specified by \a image. The current \a layout of the image must be provided
    as well. The image must be either a 2D texture or 2D texture array.

    \a format specifies the VkFormat of the image. Only image formats that are
    supported by Qt's rendering infrastructure should be used.

    \a viewFormat is usually set to the same value as \a format. In some cases,
    such as when rendering into a texture with a \c{_SRGB} format and the
    implicit linear->sRGB conversion on shader writes is not wanted, the value
    can be different. (for example, a \a format of \c VK_FORMAT_R8G8B8A8_SRGB
    and \a viewFormat of \c VK_FORMAT_R8G8B8A8_UNORM).

    \a pixelSize specifies the size of the image, in pixels. Currently only 2D
    textures are supported.

    \a sampleCount specifies the number of samples. 0 or 1 means no
    multisampling, while a value like 4 or 8 states that the native object is a
    multisample texture, except when \a flags contains \l MultisampleResolve. In
    that case, \a image is assumed to be a non-multisample 2D texture or 2D
    texture array, and \a sampleCount defines the number of samples desired. The
    resulting QQuickRenderTarget will use an intermediate, automatically created
    multisample texture (or texture array) as its color attachment, and will
    resolve the samples into \a image. This is the recommended approach to
    perform MSAA when the native Vulkan image is not already multisample.

    The number of array elements (layers) is given in \a arraySize. When greater
    than 1, it implies multiview rendering
    (\l{https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_multiview.html}{VK_KHR_multiview}),
    which can be relevant with VR/AR especially. \a arraySize is the number of
    views, typically \c 2. See \l QSGMaterial::viewCount() for details on
    enabling multiview rendering within the Qt Quick scenegraph.

    The texture is used as the first color attachment of the render target used
    by the Qt Quick scenegraph.  A depth-stencil buffer, if applicable, is
    created and used automatically. When the color buffer is multisample, the
    depth-stencil buffer will automatically be multisample too. For multiview
    rendering, the depth-stencil texture will automatically be made an array
    with a matching \a arraySize.

    \note the resulting QQuickRenderTarget does not own any native resources, it
    merely contains references and the associated metadata of the size and
    sample count. It is the caller's responsibility to ensure that the native
    resource exists as long as necessary.

    \since 6.8

    \sa QQuickWindow::setRenderTarget(), QQuickRenderControl
 */
QQuickRenderTarget QQuickRenderTarget::fromVulkanImage(VkImage image, VkImageLayout layout, VkFormat format, VkFormat viewFormat,
                                                       QSize pixelSize, int sampleCount, int arraySize, Flags flags)
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

    QRhiTexture::Flags formatFlags;
    QRhiTexture::Format rhiFormat = QSGRhiSupport::toRhiTextureFormatFromVulkan(format, &formatFlags);
    QRhiTexture::Flags viewFormatFlags;
    QRhiTexture::Format rhiViewFormat = QSGRhiSupport::toRhiTextureFormatFromVulkan(viewFormat, &viewFormatFlags);

    d->pixelSize = pixelSize;
    d->sampleCount = qMax(1, sampleCount);
    d->multisampleResolve = flags.testFlag(Flag::MultisampleResolve);

    if (arraySize <= 1) {
        d->type = QQuickRenderTargetPrivate::Type::NativeTexture;
        d->u.nativeTexture = { quint64(image), layout, uint(rhiFormat), uint(formatFlags), uint(rhiViewFormat), uint(viewFormatFlags) };
    } else {
        d->type = QQuickRenderTargetPrivate::Type::NativeTextureArray;
        d->u.nativeTextureArray = { quint64(image), layout, arraySize, uint(rhiFormat), uint(formatFlags), uint(rhiViewFormat), uint(viewFormatFlags) };
    }

    return rt;
}

#endif // Vulkan

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
            || d->mirrorVertically != other.d->mirrorVertically
            || d->multisampleResolve != other.d->multisampleResolve)
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
                || d->u.nativeTexture.rhiFormatFlags != other.d->u.nativeTexture.rhiFormatFlags
                || d->u.nativeTexture.rhiViewFormat != other.d->u.nativeTexture.rhiViewFormat
                || d->u.nativeTexture.rhiViewFormatFlags != other.d->u.nativeTexture.rhiViewFormatFlags)
            return false;
        break;
    case QQuickRenderTargetPrivate::Type::NativeTextureArray:
        if (d->u.nativeTextureArray.object != other.d->u.nativeTextureArray.object
                || d->u.nativeTextureArray.layoutOrState != other.d->u.nativeTextureArray.layoutOrState
                || d->u.nativeTextureArray.arraySize != other.d->u.nativeTextureArray.arraySize
                || d->u.nativeTextureArray.rhiFormat != other.d->u.nativeTextureArray.rhiFormat
                || d->u.nativeTextureArray.rhiFormatFlags != other.d->u.nativeTextureArray.rhiFormatFlags
                || d->u.nativeTextureArray.rhiViewFormat != other.d->u.nativeTextureArray.rhiViewFormat
                || d->u.nativeTextureArray.rhiViewFormatFlags != other.d->u.nativeTextureArray.rhiViewFormatFlags)
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

static bool createRhiRenderTargetWithRenderBuffer(QRhiRenderBuffer *renderBuffer,
                                                  const QSize &pixelSize,
                                                  int sampleCount,
                                                  QRhi *rhi,
                                                  QQuickWindowRenderTarget *dst)
{
    sampleCount = QSGRhiSupport::chooseSampleCount(sampleCount, rhi);

    std::unique_ptr<QRhiRenderBuffer> depthStencil;
    if (dst->implicitBuffers.depthStencil) {
        if (dst->implicitBuffers.depthStencil->pixelSize() == pixelSize
            && dst->implicitBuffers.depthStencil->sampleCount() == sampleCount)
        {
            depthStencil.reset(dst->implicitBuffers.depthStencil);
            dst->implicitBuffers.depthStencil = nullptr;
        }
    }
    dst->implicitBuffers.reset(rhi);

    if (!depthStencil) {
        depthStencil.reset(rhi->newRenderBuffer(QRhiRenderBuffer::DepthStencil, pixelSize, sampleCount));
        depthStencil->setName(QByteArrayLiteral("Depth-stencil buffer for QQuickRenderTarget"));
        if (!depthStencil->create()) {
            qWarning("Failed to build depth-stencil buffer for QQuickRenderTarget");
            return false;
        }
    }

    QRhiColorAttachment colorAttachment(renderBuffer);
    QRhiTextureRenderTargetDescription rtDesc(colorAttachment);
    rtDesc.setDepthStencilBuffer(depthStencil.get());
    std::unique_ptr<QRhiTextureRenderTarget> rt(rhi->newTextureRenderTarget(rtDesc));
    rt->setName(QByteArrayLiteral("RT for QQuickRenderTarget with renderbuffer"));
    std::unique_ptr<QRhiRenderPassDescriptor> rp(rt->newCompatibleRenderPassDescriptor());
    rt->setRenderPassDescriptor(rp.get());

    if (!rt->create()) {
        qWarning("Failed to build renderbuffer-based render target for QQuickRenderTarget");
        return false;
    }

    dst->rt.renderTarget = rt.release();
    dst->rt.owns = true;
    dst->res.rpDesc = rp.release();
    dst->implicitBuffers.depthStencil = depthStencil.release();

    return true;
}

static bool createRhiRenderTarget(QRhiTexture *texture,
                                  const QSize &pixelSize,
                                  int sampleCount,
                                  bool multisampleResolve,
                                  QRhi *rhi,
                                  QQuickWindowRenderTarget *dst)
{
    // Simple path: no user-supplied depth texture. So create our own
    // depth-stencil buffer, using renderbuffers (so this is still GLES 2.0
    // compatible), with MSAA support being GLES 3.0 compatible.

    sampleCount = QSGRhiSupport::chooseSampleCount(sampleCount, rhi);
    if (sampleCount <= 1)
        multisampleResolve = false;

    std::unique_ptr<QRhiRenderBuffer> depthStencil;
    if (dst->implicitBuffers.depthStencil) {
        if (dst->implicitBuffers.depthStencil->pixelSize() == pixelSize
            && dst->implicitBuffers.depthStencil->sampleCount() == sampleCount)
        {
            depthStencil.reset(dst->implicitBuffers.depthStencil);
            dst->implicitBuffers.depthStencil = nullptr;
        }
    }

    std::unique_ptr<QRhiTexture> colorBuffer;
    QRhiTexture::Flags multisampleTextureFlags;
    QRhiTexture::Format multisampleTextureFormat = texture->format();
    if (multisampleResolve) {
        multisampleTextureFlags = QRhiTexture::RenderTarget;
        if (texture->flags().testFlag(QRhiTexture::sRGB))
            multisampleTextureFlags |= QRhiTexture::sRGB;

        if (dst->implicitBuffers.multisampleTexture) {
            if (dst->implicitBuffers.multisampleTexture->pixelSize() == pixelSize
                && dst->implicitBuffers.multisampleTexture->format() == multisampleTextureFormat
                && dst->implicitBuffers.multisampleTexture->sampleCount() == sampleCount
                && dst->implicitBuffers.multisampleTexture->flags().testFlags(multisampleTextureFlags))
            {
                colorBuffer.reset(dst->implicitBuffers.multisampleTexture);
                dst->implicitBuffers.multisampleTexture = nullptr;
            }
        }
    }

    dst->implicitBuffers.reset(rhi);

    if (!depthStencil) {
        depthStencil.reset(rhi->newRenderBuffer(QRhiRenderBuffer::DepthStencil, pixelSize, sampleCount));
        depthStencil->setName(QByteArrayLiteral("Depth-stencil buffer for QQuickRenderTarget"));
        if (!depthStencil->create()) {
            qWarning("Failed to build depth-stencil buffer for QQuickRenderTarget");
            return false;
        }
    }

    if (multisampleResolve && !colorBuffer) {
        colorBuffer.reset(rhi->newTexture(multisampleTextureFormat, pixelSize, sampleCount, multisampleTextureFlags));
        colorBuffer->setName(QByteArrayLiteral("Multisample color buffer for QQuickRenderTarget"));
        colorBuffer->setWriteViewFormat(texture->writeViewFormat());
        if (!colorBuffer->create()) {
            qWarning("Failed to build multisample color buffer for QQuickRenderTarget");
            return false;
        }
    }

    QRhiColorAttachment colorAttachment;
    if (multisampleResolve) {
        colorAttachment.setTexture(colorBuffer.get());
        colorAttachment.setResolveTexture(texture);
    } else {
        colorAttachment.setTexture(texture);
    }
    QRhiTextureRenderTargetDescription rtDesc(colorAttachment);
    rtDesc.setDepthStencilBuffer(depthStencil.get());
    std::unique_ptr<QRhiTextureRenderTarget> rt(rhi->newTextureRenderTarget(rtDesc));
    rt->setName(QByteArrayLiteral("RT for QQuickRenderTarget"));
    std::unique_ptr<QRhiRenderPassDescriptor> rp(rt->newCompatibleRenderPassDescriptor());
    rt->setRenderPassDescriptor(rp.get());

    if (!rt->create()) {
        qWarning("Failed to build texture render target for QQuickRenderTarget");
        return false;
    }

    dst->rt.renderTarget = rt.release();
    dst->rt.owns = true;
    dst->res.rpDesc = rp.release();
    dst->implicitBuffers.depthStencil = depthStencil.release();
    if (multisampleResolve)
        dst->implicitBuffers.multisampleTexture = colorBuffer.release();

    return true;
}

static bool createRhiRenderTargetWithDepthTexture(QRhiTexture *texture,
                                                  QRhiTexture *depthTexture,
                                                  const QSize &pixelSize,
                                                  int sampleCount,
                                                  bool multisampleResolve,
                                                  QRhi *rhi,
                                                  QQuickWindowRenderTarget *dst)
{
    // This version takes a user-supplied depthTexture. That texture is always
    // non-multisample. If sample count is > 1, we still need our own
    // multisample depth-stencil buffer, and the depth(stencil) data is expected
    // to be resolved (and written out) to depthTexture, _if_ the underlying API
    // supports it (see QRhi's ResolveDepthStencil feature). The intermediate,
    // multisample depth-stencil buffer must be a texture here (not
    // renderbuffer), specifically for OpenGL ES and its related multisample
    // extensions.

    sampleCount = QSGRhiSupport::chooseSampleCount(sampleCount, rhi);
    if (sampleCount <= 1)
        multisampleResolve = false;

    std::unique_ptr<QRhiTexture> depthStencil;
    if (dst->implicitBuffers.depthStencilTexture) {
        if (dst->implicitBuffers.depthStencilTexture->pixelSize() == pixelSize
            && dst->implicitBuffers.depthStencilTexture->sampleCount() == sampleCount)
        {
            depthStencil.reset(dst->implicitBuffers.depthStencilTexture);
            dst->implicitBuffers.depthStencilTexture = nullptr;
        }
    }

    std::unique_ptr<QRhiTexture> colorBuffer;
    QRhiTexture::Flags multisampleTextureFlags;
    QRhiTexture::Format multisampleTextureFormat = texture->format();
    if (multisampleResolve) {
        multisampleTextureFlags = QRhiTexture::RenderTarget;
        if (texture->flags().testFlag(QRhiTexture::sRGB))
            multisampleTextureFlags |= QRhiTexture::sRGB;

        if (dst->implicitBuffers.multisampleTexture) {
            if (dst->implicitBuffers.multisampleTexture->pixelSize() == pixelSize
                && dst->implicitBuffers.multisampleTexture->format() == multisampleTextureFormat
                && dst->implicitBuffers.multisampleTexture->sampleCount() == sampleCount
                && dst->implicitBuffers.multisampleTexture->flags().testFlags(multisampleTextureFlags))
            {
                colorBuffer.reset(dst->implicitBuffers.multisampleTexture);
                dst->implicitBuffers.multisampleTexture = nullptr;
            }
        }
    }

    dst->implicitBuffers.reset(rhi);

    bool needsDepthStencilBuffer = true;
    if (sampleCount <= 1) {
        depthStencil.reset();
        needsDepthStencilBuffer = false;
    }
    if (depthTexture->pixelSize() != pixelSize) {
        qWarning("Custom depth texture size (%dx%d) does not match the QQuickRenderTarget (%dx%d)",
                    depthTexture->pixelSize().width(),
                    depthTexture->pixelSize().height(),
                    pixelSize.width(),
                    pixelSize.height());
        return false;
    }
    if (depthTexture->sampleCount() > 1) {
        qWarning("Custom depth texture cannot be multisample");
        return false;
    }
    if (needsDepthStencilBuffer && !depthStencil) {
        QRhiTexture::Format multisampleDepthTextureFormat = depthTexture->format();
        depthStencil.reset(rhi->newTexture(multisampleDepthTextureFormat, pixelSize, sampleCount, QRhiTexture::RenderTarget));
        depthStencil->setName(QByteArrayLiteral("Depth-stencil texture for QQuickRenderTarget"));
        if (!depthStencil->create()) {
            qWarning("Failed to build depth-stencil buffer for QQuickRenderTarget");
            return false;
        }
    }

    if (multisampleResolve && !colorBuffer) {
        colorBuffer.reset(rhi->newTexture(multisampleTextureFormat, pixelSize, sampleCount, multisampleTextureFlags));
        colorBuffer->setName(QByteArrayLiteral("Multisample color buffer for QQuickRenderTarget"));
        colorBuffer->setWriteViewFormat(texture->writeViewFormat());
        if (!colorBuffer->create()) {
            qWarning("Failed to build multisample color buffer for QQuickRenderTarget");
            return false;
        }
    }

    QRhiColorAttachment colorAttachment;
    if (multisampleResolve) {
        colorAttachment.setTexture(colorBuffer.get());
        colorAttachment.setResolveTexture(texture);
    } else {
        colorAttachment.setTexture(texture);
    }

    QRhiTextureRenderTargetDescription rtDesc(colorAttachment);
    if (sampleCount > 1) {
        rtDesc.setDepthTexture(depthStencil.get());
        if (rhi->isFeatureSupported(QRhi::ResolveDepthStencil))
            rtDesc.setDepthResolveTexture(depthTexture);
        else
            qWarning("Depth-stencil resolve is not supported by the underlying 3D API, depth contents will not be resolved");
    } else {
        rtDesc.setDepthTexture(depthTexture);
    }

    std::unique_ptr<QRhiTextureRenderTarget> rt(rhi->newTextureRenderTarget(rtDesc));
    rt->setName(QByteArrayLiteral("RT for QQuickRenderTarget"));
    std::unique_ptr<QRhiRenderPassDescriptor> rp(rt->newCompatibleRenderPassDescriptor());
    rt->setRenderPassDescriptor(rp.get());

    if (!rt->create()) {
        qWarning("Failed to build texture render target for QQuickRenderTarget");
        return false;
    }

    dst->rt.renderTarget = rt.release();
    dst->rt.owns = true;
    dst->res.rpDesc = rp.release();
    if (depthStencil)
        dst->implicitBuffers.depthStencilTexture = depthStencil.release();
    if (multisampleResolve)
        dst->implicitBuffers.multisampleTexture = colorBuffer.release();

    return true;
}

static bool createRhiRenderTargetMultiView(QRhiTexture *texture,
                                           QRhiTexture *maybeCustomDepthTexture,
                                           const QSize &pixelSize,
                                           int arraySize,
                                           int sampleCount,
                                           bool multisampleResolve,
                                           QRhi *rhi,
                                           QQuickWindowRenderTarget *dst)
{
    // Multiview path, working with texture arrays. Optionally with a
    // user-supplied, non-multisample depth texture (array). (same semantics
    // then as with createRhiRenderTargetWithDepthTexture, but everything is a
    // 2D texture array here)

    sampleCount = QSGRhiSupport::chooseSampleCount(sampleCount, rhi);
    if (sampleCount <= 1)
        multisampleResolve = false;

    std::unique_ptr<QRhiTexture> depthStencil;
    if (dst->implicitBuffers.depthStencilTexture) {
        if (dst->implicitBuffers.depthStencilTexture->pixelSize() == pixelSize
            && dst->implicitBuffers.depthStencilTexture->sampleCount() == sampleCount
            && dst->implicitBuffers.depthStencilTexture->arraySize() == arraySize)
        {
            depthStencil.reset(dst->implicitBuffers.depthStencilTexture);
            dst->implicitBuffers.depthStencilTexture = nullptr;
        }
    }

    std::unique_ptr<QRhiTexture> colorBuffer;
    QRhiTexture::Flags multisampleTextureFlags;
    QRhiTexture::Format multisampleTextureFormat = texture->format();
    if (multisampleResolve) {
        multisampleTextureFlags = QRhiTexture::RenderTarget;
        if (texture->flags().testFlag(QRhiTexture::sRGB))
            multisampleTextureFlags |= QRhiTexture::sRGB;

        if (dst->implicitBuffers.multisampleTexture) {
            if (dst->implicitBuffers.multisampleTexture->pixelSize() == pixelSize
                && dst->implicitBuffers.multisampleTexture->format() == multisampleTextureFormat
                && dst->implicitBuffers.multisampleTexture->sampleCount() == sampleCount
                && dst->implicitBuffers.multisampleTexture->arraySize() == arraySize
                && dst->implicitBuffers.multisampleTexture->flags().testFlags(multisampleTextureFlags))
            {
                colorBuffer.reset(dst->implicitBuffers.multisampleTexture);
                dst->implicitBuffers.multisampleTexture = nullptr;
            }
        }
    }

    dst->implicitBuffers.reset(rhi);

    bool needsDepthStencilBuffer = true;
    if (maybeCustomDepthTexture) {
        if (sampleCount <= 1) {
            depthStencil.reset();
            needsDepthStencilBuffer = false;
        }
        if (maybeCustomDepthTexture->arraySize() != arraySize) {
            qWarning("Custom depth texture array size (%d) does not match QQuickRenderTarget (%d)",
                     maybeCustomDepthTexture->arraySize(), arraySize);
            return false;
        }
        if (maybeCustomDepthTexture->pixelSize() != pixelSize) {
            qWarning("Custom depth texture size (%dx%d) does not match the QQuickRenderTarget (%dx%d)",
                     maybeCustomDepthTexture->pixelSize().width(),
                     maybeCustomDepthTexture->pixelSize().height(),
                     pixelSize.width(),
                     pixelSize.height());
            return false;
        }
        if (maybeCustomDepthTexture->sampleCount() > 1) {
            qWarning("Custom depth texture cannot be multisample");
            return false;
        }
    }
    if (needsDepthStencilBuffer && !depthStencil) {
        QRhiTexture::Format format;
        if (maybeCustomDepthTexture)
            format = maybeCustomDepthTexture->format();
        else
            format = rhi->isTextureFormatSupported(QRhiTexture::D24S8) ? QRhiTexture::D24S8 : QRhiTexture::D32FS8;
        depthStencil.reset(rhi->newTextureArray(format, arraySize, pixelSize, sampleCount, QRhiTexture::RenderTarget));
        depthStencil->setName(QByteArrayLiteral("Depth-stencil buffer (multiview) for QQuickRenderTarget"));
        if (!depthStencil->create()) {
            qWarning("Failed to build depth-stencil texture array for QQuickRenderTarget");
            return false;
        }
    }

    if (multisampleResolve && !colorBuffer) {
        colorBuffer.reset(rhi->newTextureArray(multisampleTextureFormat, arraySize, pixelSize, sampleCount, multisampleTextureFlags));
        colorBuffer->setName(QByteArrayLiteral("Multisample color buffer (multiview) for QQuickRenderTarget"));
        colorBuffer->setWriteViewFormat(texture->writeViewFormat());
        if (!colorBuffer->create()) {
            qWarning("Failed to build multisample texture array for QQuickRenderTarget");
            return false;
        }
    }

    QRhiColorAttachment colorAttachment;
    colorAttachment.setMultiViewCount(arraySize);
    if (multisampleResolve) {
        colorAttachment.setTexture(colorBuffer.get());
        colorAttachment.setResolveTexture(texture);
    } else {
        colorAttachment.setTexture(texture);
    }

    QRhiTextureRenderTargetDescription rtDesc(colorAttachment);
    if (sampleCount > 1) {
        rtDesc.setDepthTexture(depthStencil.get());
        if (maybeCustomDepthTexture) {
            if (rhi->isFeatureSupported(QRhi::ResolveDepthStencil))
                rtDesc.setDepthResolveTexture(maybeCustomDepthTexture);
            else
                qWarning("Depth-stencil resolve is not supported by the underlying 3D API, depth contents will not be resolved");
        }
    } else {
        if (depthStencil)
            rtDesc.setDepthTexture(depthStencil.get());
        else if (maybeCustomDepthTexture)
            rtDesc.setDepthTexture(maybeCustomDepthTexture);
    }

    QRhiTextureRenderTarget::Flags rtFlags;
    if (!maybeCustomDepthTexture)
        rtFlags |= QRhiTextureRenderTarget::DoNotStoreDepthStencilContents;

    std::unique_ptr<QRhiTextureRenderTarget> rt(rhi->newTextureRenderTarget(rtDesc, rtFlags));
    rt->setName(QByteArrayLiteral("RT for multiview QQuickRenderTarget"));
    std::unique_ptr<QRhiRenderPassDescriptor> rp(rt->newCompatibleRenderPassDescriptor());
    rt->setRenderPassDescriptor(rp.get());

    if (!rt->create()) {
        qWarning("Failed to build multiview texture render target for QQuickRenderTarget");
        return false;
    }

    dst->rt.renderTarget = rt.release();
    dst->rt.owns = true;
    dst->res.rpDesc = rp.release();
    if (depthStencil)
        dst->implicitBuffers.depthStencilTexture = depthStencil.release();
    if (multisampleResolve)
        dst->implicitBuffers.multisampleTexture = colorBuffer.release();

    dst->rt.multiViewCount = arraySize;

    return true;
}

bool QQuickRenderTargetPrivate::resolve(QRhi *rhi, QQuickWindowRenderTarget *dst)
{
    // dst->implicitBuffers may contain valid objects. If so, and their
    // properties are suitable, they are expected to be reused. Once taken what
    // we can reuse, it needs to be reset().

    switch (type) {
    case Type::Null:
        dst->implicitBuffers.reset(rhi);
        return true;

    case Type::NativeTexture:
    {
        QRhiTexture::Format format = u.nativeTexture.rhiFormat == QRhiTexture::UnknownFormat ? QRhiTexture::RGBA8
                                                                                             : QRhiTexture::Format(u.nativeTexture.rhiFormat);
        QRhiTexture::Format viewFormat = u.nativeTexture.rhiViewFormat == QRhiTexture::UnknownFormat ? QRhiTexture::RGBA8
                                                                                                     : QRhiTexture::Format(u.nativeTexture.rhiViewFormat);
        const auto flags = QRhiTexture::RenderTarget | QRhiTexture::Flags(u.nativeTexture.rhiFormatFlags);
        std::unique_ptr<QRhiTexture> texture(rhi->newTexture(format, pixelSize, multisampleResolve ? 1 : sampleCount, flags));
        const bool textureIsSrgb = flags.testFlag(QRhiTexture::sRGB);
        const bool viewIsSrgb = QRhiTexture::Flags(u.nativeTexture.rhiViewFormatFlags).testFlag(QRhiTexture::sRGB);
        if (viewFormat != format || viewIsSrgb != textureIsSrgb)
            texture->setWriteViewFormat({ viewFormat, viewIsSrgb });
        if (!texture->createFrom({ u.nativeTexture.object, u.nativeTexture.layoutOrState })) {
            qWarning("Failed to build wrapper texture for QQuickRenderTarget");
            return false;
        }
        if (customDepthTexture) {
            if (!createRhiRenderTargetWithDepthTexture(texture.get(), customDepthTexture, pixelSize, sampleCount, multisampleResolve, rhi, dst))
                return false;
        } else {
            if (!createRhiRenderTarget(texture.get(), pixelSize, sampleCount, multisampleResolve, rhi, dst))
                return false;
        }
        dst->res.texture = texture.release();
    }
        return true;

    case Type::NativeTextureArray:
    {
        QRhiTexture::Format format = u.nativeTextureArray.rhiFormat == QRhiTexture::UnknownFormat ? QRhiTexture::RGBA8
                                                                                                  : QRhiTexture::Format(u.nativeTextureArray.rhiFormat);
        QRhiTexture::Format viewFormat = u.nativeTextureArray.rhiViewFormat == QRhiTexture::UnknownFormat ? QRhiTexture::RGBA8
                                                                                                          : QRhiTexture::Format(u.nativeTextureArray.rhiViewFormat);
        const auto flags = QRhiTexture::RenderTarget | QRhiTexture::Flags(u.nativeTextureArray.rhiFormatFlags);
        const int arraySize = u.nativeTextureArray.arraySize;
        std::unique_ptr<QRhiTexture> texture(rhi->newTextureArray(format, arraySize, pixelSize, multisampleResolve ? 1 : sampleCount, flags));
        const bool textureIsSrgb = flags.testFlag(QRhiTexture::sRGB);
        const bool viewIsSrgb = QRhiTexture::Flags(u.nativeTextureArray.rhiViewFormatFlags).testFlag(QRhiTexture::sRGB);
        if (viewFormat != format || viewIsSrgb != textureIsSrgb)
            texture->setWriteViewFormat({ viewFormat, viewIsSrgb });
        if (!texture->createFrom({ u.nativeTextureArray.object, u.nativeTextureArray.layoutOrState })) {
            qWarning("Failed to build wrapper texture array for QQuickRenderTarget");
            return false;
        }
        if (!createRhiRenderTargetMultiView(texture.get(), customDepthTexture, pixelSize, arraySize, sampleCount, multisampleResolve, rhi, dst))
             return false;
        dst->res.texture = texture.release();
    }
        return true;

    case Type::NativeRenderbuffer:
    {
        std::unique_ptr<QRhiRenderBuffer> renderbuffer(rhi->newRenderBuffer(QRhiRenderBuffer::Color, pixelSize, sampleCount));
        if (!renderbuffer->createFrom({ u.nativeRenderbufferObject })) {
            qWarning("Failed to build wrapper renderbuffer for QQuickRenderTarget");
            return false;
        }
        if (customDepthTexture)
            qWarning("Custom depth texture is not supported with renderbuffers in QQuickRenderTarget");
        if (!createRhiRenderTargetWithRenderBuffer(renderbuffer.get(), pixelSize, sampleCount, rhi, dst))
            return false;
        dst->res.renderBuffer = renderbuffer.release();
    }
        return true;

    case Type::RhiRenderTarget:
        dst->implicitBuffers.reset(rhi);
        dst->rt.renderTarget = u.rhiRt;
        dst->rt.owns = false;
        if (dst->rt.renderTarget->resourceType() == QRhiResource::TextureRenderTarget) {
            auto texRt = static_cast<QRhiTextureRenderTarget *>(dst->rt.renderTarget);
            const QRhiTextureRenderTargetDescription desc = texRt->description();
            bool first = true;
            for (auto it = desc.cbeginColorAttachments(), end = desc.cendColorAttachments(); it != end; ++it) {
                if (it->multiViewCount() <= 1)
                    continue;
                if (first || dst->rt.multiViewCount == it->multiViewCount()) {
                    first = false;
                    if (it->texture() && it->texture()->flags().testFlag(QRhiTexture::TextureArray)) {
                        if (it->texture()->arraySize() >= it->layer() + it->multiViewCount()) {
                            dst->rt.multiViewCount = it->multiViewCount();
                        } else {
                            qWarning("Invalid QQuickRenderTarget; needs at least %d elements in texture array, got %d",
                                     it->layer() + it->multiViewCount(),
                                     it->texture()->arraySize());
                            return false;
                        }
                    } else {
                        qWarning("Invalid QQuickRenderTarget; multiview requires a texture array");
                        return false;
                    }
                } else {
                    qWarning("Inconsistent multiViewCount in QQuickRenderTarget (was %d, now found an attachment with %d)",
                             dst->rt.multiViewCount, it->multiViewCount());
                    return false;
                }
            }
        }
        if (customDepthTexture)
            qWarning("Custom depth texture is not supported with QRhiRenderTarget in QQuickRenderTarget");
        return true;

    case Type::PaintDevice:
        dst->implicitBuffers.reset(rhi);
        dst->sw.paintDevice = u.paintDevice;
        dst->sw.owns = false;
        return true;
    }

    Q_UNREACHABLE_RETURN(false);
}

QT_END_NAMESPACE
