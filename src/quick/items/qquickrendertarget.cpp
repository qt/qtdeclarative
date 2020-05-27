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
    \return a new QQuickRenderTarget referencing a native texture or image
    object.

    \a nativeTexture references a native resource, for example, a \c VkImage,
    \c ID3D11Texture2D*, c MTLTexture*, or \c GLuint. Where applicable, this
    must be complemented by the current layout of the image.

    \note the \c object field in \a nativeTexture must always be a pointer to
    the native object, even if the type is already a pointer.

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
QQuickRenderTarget QQuickRenderTarget::fromNativeTexture(const QSGTexture::NativeTexture &nativeTexture,
                                                         const QSize &pixelSize,
                                                         int sampleCount)
{
    QQuickRenderTarget rt;
    QQuickRenderTargetPrivate *d = QQuickRenderTargetPrivate::get(&rt);

    if (!nativeTexture.object) {
        qWarning("QQuickRenderTarget: nativeTexture.object is null");
        return rt;
    }

    if (pixelSize.isEmpty()) {
        qWarning("QQuickRenderTarget: Cannot create with empty size");
        return rt;
    }

    d->type = QQuickRenderTargetPrivate::Type::NativeTexture;
    d->pixelSize = pixelSize;
    d->sampleCount = qMax(1, sampleCount);
    d->u.nativeTexture = nativeTexture;

    return rt;
}

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
    \return true if \a a and \a b refer to the same set of native objects and
    have matching associated data (size, sample count).
 */
bool operator==(const QQuickRenderTarget &a, const QQuickRenderTarget &b) Q_DECL_NOTHROW
{
    const QQuickRenderTargetPrivate *da = QQuickRenderTargetPrivate::get(&a);
    const QQuickRenderTargetPrivate *db = QQuickRenderTargetPrivate::get(&b);
    if (da->type != db->type
            || da->pixelSize != db->pixelSize
            || da->sampleCount != db->sampleCount)
    {
        return false;
    }

    switch (da->type) {
    case QQuickRenderTargetPrivate::Type::Null:
        break;
    case QQuickRenderTargetPrivate::Type::NativeTexture:
        if (da->u.nativeTexture.object != db->u.nativeTexture.object
                || da->u.nativeTexture.layout != db->u.nativeTexture.layout)
            return false;
        break;
    case QQuickRenderTargetPrivate::Type::RhiRenderTarget:
        if (da->u.rhiRt != db->u.rhiRt)
            return false;
        break;
    default:
        break;
    }

    return true;
}

/*!
    \return true if \a a and \a b refer to a different set of native objects,
    or the associated data (size, sample count) does not match.
 */
bool operator!=(const QQuickRenderTarget &a, const QQuickRenderTarget &b) Q_DECL_NOTHROW
{
    return !(a == b);
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
        QRhiTexture *texture = rhi->newTexture(QRhiTexture::RGBA8, pixelSize, sampleCount, QRhiTexture::RenderTarget);
        if (!texture->createFrom({ u.nativeTexture.object, u.nativeTexture.layout })) {
            qWarning("Failed to build texture for QQuickRenderTarget");
            return false;
        }
        QRhiRenderBuffer *depthStencil = rhi->newRenderBuffer(QRhiRenderBuffer::DepthStencil, pixelSize, sampleCount);
        if (!depthStencil->create()) {
            qWarning("Failed to build depth-stencil buffer for QQuickRenderTarget");
            delete texture;
            return false;
        }

        QRhiColorAttachment att(texture);
        QRhiTextureRenderTargetDescription rtDesc(att);
        rtDesc.setDepthStencilBuffer(depthStencil);
        QRhiTextureRenderTarget *rt = rhi->newTextureRenderTarget(rtDesc);
        QRhiRenderPassDescriptor *rp = rt->newCompatibleRenderPassDescriptor();
        rt->setRenderPassDescriptor(rp);
        if (!rt->create()) {
            qWarning("Failed to build texture render target for QQuickRenderTarget");
            delete rp;
            delete depthStencil;
            delete texture;
            return false;
        }
        dst->renderTarget = rt;
        dst->rpDesc = rp;
        dst->texture = texture;
        dst->depthStencil = depthStencil;
        dst->owns = true; // ownership of the native resource itself is not transferred but the QRhi objects are on us now
    }
        return true;

    case Type::RhiRenderTarget:
        dst->renderTarget = u.rhiRt;
        dst->owns = false;
        return true;

    default:
        break;
    }
    return false;
}

QT_END_NAMESPACE
