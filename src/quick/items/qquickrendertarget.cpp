/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquickrendertarget_p.h"
#include <QtGui/private/qrhi_p.h>
#include <QtQuick/private/qquickitem_p.h>
#include <QtQuick/private/qquickwindow_p.h>

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

QQuickRenderTargetPrivate::QQuickRenderTargetPrivate(const QQuickRenderTargetPrivate *other)
    : ref(1),
      type(other->type),
      pixelSize(other->pixelSize),
      sampleCount(other->sampleCount),
      u(other->u)
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
    \return a new QQuickRenderTarget referencing an OpenGL texture object
    specified by \a textureId.

    \a pixelSize specifies the size of the image, in pixels. Currently only 2D
    textures are supported.

    \a sampleCount specific the number of samples. 0 or 1 means no
    multisampling, while a value like 4 or 8 states that the native object is a
    multisample texture.

    \note the resulting QQuickRenderTarget does not own any native resources,
    it merely contains references and the associated metadata of the size and
    sample count. It is the caller's responsibility to ensure that the native
    resource exists as long as necessary.

    \sa QQuickWindow::setRenderTarget(), QQuickRenderControl
 */
#if QT_CONFIG(opengl) || defined(Q_CLANG_QDOC)
QQuickRenderTarget QQuickRenderTarget::fromOpenGLTexture(uint textureId, const QSize &pixelSize, int sampleCount)
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
    d->u.nativeTexture = { textureId, 0 };

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
    \return a new QQuickRenderTarget referencing an D3D11 texture object
    specified by \a texture.

    \a pixelSize specifies the size of the image, in pixels. Currently only 2D
    textures are supported.

    \a sampleCount specific the number of samples. 0 or 1 means no
    multisampling, while a value like 4 or 8 states that the native object is a
    multisample texture.

    \note the resulting QQuickRenderTarget does not own any native resources,
    it merely contains references and the associated metadata of the size and
    sample count. It is the caller's responsibility to ensure that the native
    resource exists as long as necessary.

    \sa QQuickWindow::setRenderTarget(), QQuickRenderControl
 */
#if defined(Q_OS_WIN) || defined(Q_CLANG_QDOC)
QQuickRenderTarget QQuickRenderTarget::fromD3D11Texture(void *texture, const QSize &pixelSize, int sampleCount)
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
    d->u.nativeTexture = { quint64(texture), 0 };

    return rt;
}
#endif

/*!
    \return a new QQuickRenderTarget referencing an Metal texture object
    specified by \a texture.

    \a pixelSize specifies the size of the image, in pixels. Currently only 2D
    textures are supported.

    \a sampleCount specific the number of samples. 0 or 1 means no
    multisampling, while a value like 4 or 8 states that the native object is a
    multisample texture.

    \note the resulting QQuickRenderTarget does not own any native resources,
    it merely contains references and the associated metadata of the size and
    sample count. It is the caller's responsibility to ensure that the native
    resource exists as long as necessary.

    \sa QQuickWindow::setRenderTarget(), QQuickRenderControl
 */
#if defined(Q_OS_MACOS) || defined(Q_OS_IOS) || defined(Q_CLANG_QDOC)
QQuickRenderTarget QQuickRenderTarget::fromMetalTexture(MTLTexture *texture, const QSize &pixelSize, int sampleCount)
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
    d->u.nativeTexture = { quint64(texture), 0 };

    return rt;
}
#endif

/*!
    \return a new QQuickRenderTarget referencing an Vulkan image object
    specified by \a image. The current \a layout of the image must be provided
    as well.

    \a pixelSize specifies the size of the image, in pixels. Currently only 2D
    textures are supported.

    \a sampleCount specific the number of samples. 0 or 1 means no
    multisampling, while a value like 4 or 8 states that the native object is a
    multisample texture.

    \note the resulting QQuickRenderTarget does not own any native resources,
    it merely contains references and the associated metadata of the size and
    sample count. It is the caller's responsibility to ensure that the native
    resource exists as long as necessary.

    \sa QQuickWindow::setRenderTarget(), QQuickRenderControl
 */
#if QT_CONFIG(vulkan) || defined(Q_CLANG_QDOC)
QQuickRenderTarget QQuickRenderTarget::fromVulkanImage(VkImage image, VkImageLayout layout, const QSize &pixelSize, int sampleCount)
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
    d->u.nativeTexture = { quint64(image), layout };

    return rt;
}
#endif

/*!
    \internal
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
            || d->sampleCount != other.d->sampleCount)
    {
        return false;
    }

    switch (d->type) {
    case QQuickRenderTargetPrivate::Type::Null:
        break;
    case QQuickRenderTargetPrivate::Type::NativeTexture:
        if (d->u.nativeTexture.object != other.d->u.nativeTexture.object
                || d->u.nativeTexture.layout != other.d->u.nativeTexture.layout)
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
        dst->owns = false;
        return true;

    case Type::NativeTexture:
    {
        std::unique_ptr<QRhiTexture> texture(rhi->newTexture(QRhiTexture::RGBA8, pixelSize, sampleCount, QRhiTexture::RenderTarget));
        if (!texture->createFrom({ u.nativeTexture.object, u.nativeTexture.layout })) {
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

    default:
        break;
    }
    return false;
}

QT_END_NAMESPACE
