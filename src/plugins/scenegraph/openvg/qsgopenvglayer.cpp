/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "qsgopenvglayer.h"
#include "qsgopenvgrenderer_p.h"
#include "qsgopenvgcontext_p.h"

QT_BEGIN_NAMESPACE

QSGOpenVGLayer::QSGOpenVGLayer(QSGRenderContext *renderContext)
    : m_item(nullptr)
    , m_renderer(nullptr)
    , m_device_pixel_ratio(1)
    , m_mirrorHorizontal(false)
    , m_mirrorVertical(false)
    , m_live(true)
    , m_grab(true)
    , m_recursive(false)
    , m_dirtyTexture(true)
    , m_image(0)
    , m_renderTarget(0)
    , m_layerContext(0)
{
    m_context = static_cast<QSGOpenVGRenderContext*>(renderContext);
    m_vgContext = m_context->vgContext();
}

QSGOpenVGLayer::~QSGOpenVGLayer()
{
    invalidated();
}

int QSGOpenVGLayer::textureId() const
{
    return static_cast<int>(m_image);
}

QSize QSGOpenVGLayer::textureSize() const
{
    if (m_image != 0) {
        VGint imageWidth = vgGetParameteri(m_image, VG_IMAGE_WIDTH);
        VGint imageHeight = vgGetParameteri(m_image, VG_IMAGE_HEIGHT);
        return QSize(imageWidth, imageHeight);
    }

    return QSize();
}

bool QSGOpenVGLayer::hasAlphaChannel() const
{
    VGImageFormat format = static_cast<VGImageFormat>(vgGetParameteri(m_image, VG_IMAGE_FORMAT));

    switch (format) {
    case VG_sRGBA_8888:
    case VG_sRGBA_8888_PRE:
    case VG_sRGBA_5551:
    case VG_sRGBA_4444:
    case VG_lRGBA_8888:
    case VG_lRGBA_8888_PRE:
    case VG_A_8:
    case VG_A_1:
    case VG_A_4:
    case VG_sARGB_8888:
    case VG_sARGB_8888_PRE:
    case VG_sARGB_1555:
    case VG_sARGB_4444:
    case VG_lARGB_8888:
    case VG_lARGB_8888_PRE:
    case VG_sBGRA_8888:
    case VG_sBGRA_8888_PRE:
    case VG_sBGRA_5551:
    case VG_sBGRA_4444:
    case VG_lBGRA_8888:
    case VG_lBGRA_8888_PRE:
    case VG_sABGR_8888:
    case VG_sABGR_8888_PRE:
    case VG_sABGR_1555:
    case VG_sABGR_4444:
    case VG_lABGR_8888:
    case VG_lABGR_8888_PRE:
        return true;
        break;
    default:
        break;
    }
    return false;
}

bool QSGOpenVGLayer::hasMipmaps() const
{
    return false;
}

void QSGOpenVGLayer::bind()
{
}

bool QSGOpenVGLayer::updateTexture()
{
    bool doGrab = (m_live || m_grab) && m_dirtyTexture;
    if (doGrab)
        grab();
    if (m_grab)
        emit scheduledUpdateCompleted();
    m_grab = false;
    return doGrab;
}

void QSGOpenVGLayer::setItem(QSGNode *item)
{
    if (item == m_item)
        return;
    m_item = item;

    if (m_live && !m_item) {
        vgDestroyImage(m_image);
        m_image = 0;
    }

    markDirtyTexture();
}

void QSGOpenVGLayer::setRect(const QRectF &rect)
{
    if (rect == m_rect)
        return;
    m_rect = rect;
    markDirtyTexture();
}

void QSGOpenVGLayer::setSize(const QSize &size)
{
    if (size == m_size)
        return;
    m_size = size;

    if (m_live && m_size.isNull()) {
        vgDestroyImage(m_image);
        m_image = 0;
    }

    markDirtyTexture();
}

void QSGOpenVGLayer::scheduleUpdate()
{
    if (m_grab)
        return;
    m_grab = true;
    if (m_dirtyTexture) {
        emit updateRequested();
    }
}

QImage QSGOpenVGLayer::toImage() const
{
    // XXX
    return QImage();
}

void QSGOpenVGLayer::setLive(bool live)
{
    if (live == m_live)
        return;
    m_live = live;

    if (m_live && (!m_item || m_size.isNull())) {
        vgDestroyImage(m_image);
        m_image = 0;
    }

    markDirtyTexture();
}

void QSGOpenVGLayer::setRecursive(bool recursive)
{
    m_recursive = recursive;
}

void QSGOpenVGLayer::setFormat(uint format)
{
    Q_UNUSED(format)
}

void QSGOpenVGLayer::setHasMipmaps(bool mipmap)
{
    Q_UNUSED(mipmap)
}

void QSGOpenVGLayer::setDevicePixelRatio(qreal ratio)
{
    m_device_pixel_ratio = ratio;
}

void QSGOpenVGLayer::setMirrorHorizontal(bool mirror)
{
    if (m_mirrorHorizontal == mirror)
        return;
    m_mirrorHorizontal = mirror;
    markDirtyTexture();
}

void QSGOpenVGLayer::setMirrorVertical(bool mirror)
{
    if (m_mirrorVertical == mirror)
        return;
    m_mirrorVertical = mirror;
    markDirtyTexture();
}

void QSGOpenVGLayer::markDirtyTexture()
{
    m_dirtyTexture = true;
    if (m_live || m_grab) {
        emit updateRequested();
    }
}

void QSGOpenVGLayer::invalidated()
{
    delete m_renderer;
    m_renderer = 0;
}

void QSGOpenVGLayer::grab()
{
    if (!m_item || m_size.isNull()) {
        vgDestroyImage(m_image);
        m_image = 0;
        m_dirtyTexture = false;
        return;
    }
    QSGNode *root = m_item;
    while (root->firstChild() && root->type() != QSGNode::RootNodeType)
        root = root->firstChild();
    if (root->type() != QSGNode::RootNodeType)
        return;

    if (!m_renderer) {
        m_renderer = new QSGOpenVGRenderer(m_context);
        connect(m_renderer, SIGNAL(sceneGraphChanged()), this, SLOT(markDirtyTexture()));
    }
    m_renderer->setDevicePixelRatio(m_device_pixel_ratio);
    m_renderer->setRootNode(static_cast<QSGRootNode *>(root));

    if (m_image == 0 || m_imageSize != m_size ) {
        if (m_image != 0)
            vgDestroyImage(m_image);

        m_image = vgCreateImage(VG_lARGB_8888_PRE, m_size.width(), m_size.height(), VG_IMAGE_QUALITY_BETTER);
        m_imageSize = m_size;

        //Destroy old RenderTarget
        if (m_renderTarget != 0)
            eglDestroySurface(m_vgContext->eglDisplay(), m_renderTarget);

        const EGLint configAttribs[] = {
            EGL_CONFORMANT, EGL_OPENVG_BIT,
            EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
            EGL_RED_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_BLUE_SIZE, 8,
            EGL_ALPHA_SIZE, 8,
            EGL_ALPHA_MASK_SIZE, 8,
            EGL_NONE
        };

        EGLConfig pbufferConfig;
        EGLint numConfig;
        eglChooseConfig(m_vgContext->eglDisplay(), configAttribs, &pbufferConfig, 1, &numConfig);

        if (m_layerContext == 0) {
            // Create new context
            m_layerContext = eglCreateContext(m_vgContext->eglDisplay(), pbufferConfig, m_vgContext->eglContext(), 0);
        }

        m_renderTarget = eglCreatePbufferFromClientBuffer(m_vgContext->eglDisplay(),
                                                          EGL_OPENVG_IMAGE,
                                                          (EGLClientBuffer)m_image,
                                                          pbufferConfig,
                                                          0);
    }

    if (m_renderTarget == EGL_NO_SURFACE) {
        qDebug() << "invalid renderTarget!";
        return;
    }

    // Render texture.
    root->markDirty(QSGNode::DirtyForceUpdate); // Force matrix, clip and opacity update.
    m_renderer->nodeChanged(root, QSGNode::DirtyForceUpdate); // Force render list update.

    m_dirtyTexture = false;

    m_renderer->setDeviceRect(m_size);
    m_renderer->setViewportRect(m_size);
    QRect mirrored(m_mirrorHorizontal ? m_rect.right() * m_device_pixel_ratio : m_rect.left() * m_device_pixel_ratio,
                   m_mirrorVertical ? m_rect.top() * m_device_pixel_ratio : m_rect.bottom() * m_device_pixel_ratio,
                   m_mirrorHorizontal ? -m_rect.width() * m_device_pixel_ratio : m_rect.width() * m_device_pixel_ratio,
                   m_mirrorVertical ? m_rect.height() * m_device_pixel_ratio : -m_rect.height() * m_device_pixel_ratio);
    m_renderer->setProjectionMatrixToRect(mirrored);
    m_renderer->setClearColor(Qt::transparent);

    eglMakeCurrent(m_vgContext->eglDisplay(), m_renderTarget, m_renderTarget, m_layerContext);

    // Before Rendering setup context for adjusting to Qt Coordinates to PixelBuffer
    // Should already be inverted by default
    vgSeti(VG_MATRIX_MODE, VG_MATRIX_PATH_USER_TO_SURFACE);
    vgLoadIdentity();

    m_renderer->renderScene();

    eglSwapBuffers(m_vgContext->eglDisplay(), m_renderTarget);

    // make the default surface current again
    m_vgContext->makeCurrent();

    root->markDirty(QSGNode::DirtyForceUpdate); // Force matrix, clip, opacity and render list update.

    if (m_recursive)
        markDirtyTexture(); // Continuously update if 'live' and 'recursive'.
}

QT_END_NAMESPACE
