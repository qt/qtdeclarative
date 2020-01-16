/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "qsgrhilayer_p.h"

#include <private/qqmlglobal_p.h>
#include <private/qsgrenderer_p.h>
#include <private/qsgdefaultrendercontext_p.h>

QSGRhiLayer::QSGRhiLayer(QSGRenderContext *context)
    : QSGLayer(*(new QSGRhiLayerPrivate))
    , m_mipmap(false)
    , m_live(true)
    , m_recursive(false)
    , m_dirtyTexture(true)
    , m_multisampling(false)
    , m_grab(false)
    , m_mirrorHorizontal(false)
    , m_mirrorVertical(true)
{
    m_context = static_cast<QSGDefaultRenderContext *>(context);
    m_rhi = m_context->rhi();
    Q_ASSERT(m_rhi);
}

QSGRhiLayer::~QSGRhiLayer()
{
    invalidated();
}

void QSGRhiLayer::invalidated()
{
    releaseResources();

    delete m_renderer;
    m_renderer = nullptr;
}

int QSGRhiLayerPrivate::comparisonKey() const
{
    Q_Q(const QSGRhiLayer);
    return int(qintptr(q->m_texture));
}

bool QSGRhiLayer::hasAlphaChannel() const
{
    return true;
}

bool QSGRhiLayer::hasMipmaps() const
{
    return m_mipmap;
}

int QSGRhiLayer::textureId() const
{
    Q_ASSERT_X(false, "QSGRhiLayer::textureId()", "Not implemented for RHI");
    return 0;
}

void QSGRhiLayer::bind()
{
    Q_ASSERT_X(false, "QSGRhiLayer::bind()", "Not implemented for RHI");
}

QRhiTexture *QSGRhiLayerPrivate::rhiTexture() const
{
    Q_Q(const QSGRhiLayer);
    return q->m_texture;
}

void QSGRhiLayerPrivate::updateRhiTexture(QRhi *rhi, QRhiResourceUpdateBatch *resourceUpdates)
{
    Q_UNUSED(rhi);
    Q_UNUSED(resourceUpdates);
}

bool QSGRhiLayer::updateTexture()
{
    // called during the preprocess phase, outside of frame rendering -> good

    bool doGrab = (m_live || m_grab) && m_dirtyTexture;
    if (doGrab)
        grab();

    if (m_grab)
        emit scheduledUpdateCompleted();

    m_grab = false;
    return doGrab;
}

void QSGRhiLayer::setHasMipmaps(bool mipmap)
{
    if (mipmap == m_mipmap)
        return;

    m_mipmap = mipmap;
    if (m_mipmap && m_texture)
        markDirtyTexture();
}


void QSGRhiLayer::setItem(QSGNode *item)
{
    if (item == m_item)
        return;

    m_item = item;

    if (m_live && !m_item)
        releaseResources();

    markDirtyTexture();
}

void QSGRhiLayer::setRect(const QRectF &rect)
{
    if (rect == m_rect)
        return;

    m_rect = rect;
    markDirtyTexture();
}

void QSGRhiLayer::setSize(const QSize &size)
{
    if (size == m_size)
        return;

    m_size = size;

    if (m_live && m_size.isNull())
        releaseResources();

    markDirtyTexture();
}

void QSGRhiLayer::setFormat(uint format)
{
    Q_UNUSED(format);
}

void QSGRhiLayer::setLive(bool live)
{
    if (live == m_live)
        return;

    m_live = live;

    if (m_live && (!m_item || m_size.isNull()))
        releaseResources();

    markDirtyTexture();
}

void QSGRhiLayer::scheduleUpdate()
{
    if (m_grab)
        return;

    m_grab = true;
    if (m_dirtyTexture)
        emit updateRequested();
}

void QSGRhiLayer::setRecursive(bool recursive)
{
    m_recursive = recursive;
}

void QSGRhiLayer::setMirrorHorizontal(bool mirror)
{
    m_mirrorHorizontal = mirror;
}

void QSGRhiLayer::setMirrorVertical(bool mirror)
{
    m_mirrorVertical = mirror;
}

void QSGRhiLayer::markDirtyTexture()
{
    m_dirtyTexture = true;
    if (m_live || m_grab)
        emit updateRequested();
}

void QSGRhiLayer::releaseResources()
{
    delete m_rt;
    m_rt = nullptr;

    delete m_rtRp;
    m_rtRp = nullptr;

    delete m_ds;
    m_ds = nullptr;

    delete m_msaaColorBuffer;
    m_msaaColorBuffer = nullptr;

    delete m_texture;
    m_texture = nullptr;

    delete m_secondaryTexture;
    m_secondaryTexture = nullptr;
}

void QSGRhiLayer::grab()
{
    if (!m_item || m_size.isNull()) {
        releaseResources();
        m_dirtyTexture = false;
        return;
    }

    int effectiveSamples = m_samples;
    // if no layer.samples was provided use the window's msaa setting
    if (effectiveSamples <= 1)
        effectiveSamples = m_context->msaaSampleCount();

    const bool needsNewRt = !m_rt || m_rt->pixelSize() != m_size || (m_recursive && !m_secondaryTexture);
    const bool mipmapSettingChanged = m_texture && m_texture->flags().testFlag(QRhiTexture::MipMapped) != m_mipmap;
    const bool msaaSettingChanged = (effectiveSamples > 1 && !m_msaaColorBuffer) || (effectiveSamples <= 1 && m_msaaColorBuffer);

    if (needsNewRt ||mipmapSettingChanged || msaaSettingChanged) {
        if (effectiveSamples <= 1) {
            m_multisampling = false;
        } else {
            m_multisampling = m_rhi->isFeatureSupported(QRhi::MultisampleRenderBuffer);
            if (!m_multisampling)
                qWarning("Layer requested %d samples but multisample renderbuffers are not supported", effectiveSamples);
        }

        QRhiTexture::Flags textureFlags = QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource;
        if (m_mipmap)
            textureFlags |= QRhiTexture::MipMapped | QRhiTexture::UsedWithGenerateMips;

        if (m_multisampling) {
            releaseResources();
            m_msaaColorBuffer = m_rhi->newRenderBuffer(QRhiRenderBuffer::Color, m_size, effectiveSamples);
            if (!m_msaaColorBuffer->build()) {
                qWarning("Failed to build multisample color buffer for layer of size %dx%d, sample count %d",
                         m_size.width(), m_size.height(), effectiveSamples);
                releaseResources();
                return;
            }
            m_texture = m_rhi->newTexture(m_format, m_size, 1, textureFlags);
            if (!m_texture->build()) {
                qWarning("Failed to build texture for layer of size %dx%d", m_size.width(), m_size.height());
                releaseResources();
                return;
            }
            m_ds = m_rhi->newRenderBuffer(QRhiRenderBuffer::DepthStencil, m_size, effectiveSamples);
            if (!m_ds->build()) {
                qWarning("Failed to build depth-stencil buffer for layer");
                releaseResources();
                return;
            }
            QRhiTextureRenderTargetDescription desc;
            QRhiColorAttachment color0(m_msaaColorBuffer);
            color0.setResolveTexture(m_texture);
            desc.setColorAttachments({ color0 });
            desc.setDepthStencilBuffer(m_ds);
            m_rt = m_rhi->newTextureRenderTarget(desc);
            m_rtRp = m_rt->newCompatibleRenderPassDescriptor();
            if (!m_rtRp) {
                qWarning("Failed to build render pass descriptor for layer");
                releaseResources();
                return;
            }
            m_rt->setRenderPassDescriptor(m_rtRp);
            if (!m_rt->build()) {
                qWarning("Failed to build texture render target for layer");
                releaseResources();
                return;
            }
        } else {
            releaseResources();
            m_texture = m_rhi->newTexture(m_format, m_size, 1, textureFlags);
            if (!m_texture->build()) {
                qWarning("Failed to build texture for layer of size %dx%d", m_size.width(), m_size.height());
                releaseResources();
                return;
            }
            m_ds = m_rhi->newRenderBuffer(QRhiRenderBuffer::DepthStencil, m_size);
            if (!m_ds->build()) {
                qWarning("Failed to build depth-stencil buffer for layer");
                releaseResources();
                return;
            }
            QRhiColorAttachment color0(m_texture);
            if (m_recursive) {
                // Here rt is associated with m_secondaryTexture instead of m_texture.
                // We will issue a copy to m_texture afterwards.
                m_secondaryTexture = m_rhi->newTexture(m_format, m_size, 1, textureFlags);
                if (!m_secondaryTexture->build()) {
                    qWarning("Failed to build texture for layer of size %dx%d", m_size.width(), m_size.height());
                    releaseResources();
                    return;
                }
                color0.setTexture(m_secondaryTexture);
            }
            m_rt = m_rhi->newTextureRenderTarget({ color0, m_ds });
            m_rtRp = m_rt->newCompatibleRenderPassDescriptor();
            if (!m_rtRp) {
                qWarning("Failed to build render pass descriptor for layer");
                releaseResources();
                return;
            }
            m_rt->setRenderPassDescriptor(m_rtRp);
            if (!m_rt->build()) {
                qWarning("Failed to build texture render target for layer");
                releaseResources();
                return;
            }
        }
    }

    QSGNode *root = m_item;
    while (root->firstChild() && root->type() != QSGNode::RootNodeType)
        root = root->firstChild();
    if (root->type() != QSGNode::RootNodeType)
        return;

    if (!m_renderer) {
        m_renderer = m_context->createRenderer();
        connect(m_renderer, SIGNAL(sceneGraphChanged()), this, SLOT(markDirtyTexture()));
    }
    m_renderer->setRootNode(static_cast<QSGRootNode *>(root));
    root->markDirty(QSGNode::DirtyForceUpdate); // Force matrix, clip and opacity update.
    m_renderer->nodeChanged(root, QSGNode::DirtyForceUpdate); // Force render list update.

    // This must not be moved. The flag must be reset only after the
    // nodeChanged otherwise we end up with constantly updating even when the
    // layer contents do not change.
    m_dirtyTexture = false;

    m_renderer->setDevicePixelRatio(m_dpr);
    m_renderer->setDeviceRect(m_size);
    m_renderer->setViewportRect(m_size);
    QRectF mirrored;
    if (m_rhi->isYUpInFramebuffer()) {
        mirrored = QRectF(m_mirrorHorizontal ? m_rect.right() : m_rect.left(),
                          m_mirrorVertical ? m_rect.bottom() : m_rect.top(),
                          m_mirrorHorizontal ? -m_rect.width() : m_rect.width(),
                          m_mirrorVertical ? -m_rect.height() : m_rect.height());
    } else {
        mirrored = QRectF(m_mirrorHorizontal ? m_rect.right() : m_rect.left(),
                          m_mirrorVertical ? m_rect.top() : m_rect.bottom(),
                          m_mirrorHorizontal ? -m_rect.width() : m_rect.width(),
                          m_mirrorVertical ? m_rect.height() : -m_rect.height());
    }
    QSGAbstractRenderer::MatrixTransformFlags matrixFlags;
    if (!m_rhi->isYUpInNDC())
        matrixFlags |= QSGAbstractRenderer::MatrixTransformFlipY;
    m_renderer->setProjectionMatrixToRect(mirrored, matrixFlags);
    m_renderer->setClearColor(Qt::transparent);
    m_renderer->setRenderTarget(m_rt);
    m_renderer->setCommandBuffer(m_context->currentFrameCommandBuffer());
    m_renderer->setRenderPassDescriptor(m_rtRp);

    QRhiResourceUpdateBatch *resourceUpdates = nullptr;

    // render with our own "sub-renderer" (this will just add a render pass to the command buffer)
    if (m_multisampling) {
        m_context->renderNextRhiFrame(m_renderer);
    } else {
        if (m_recursive) {
            m_context->renderNextRhiFrame(m_renderer);
            if (!resourceUpdates)
                resourceUpdates = m_rhi->nextResourceUpdateBatch();
            resourceUpdates->copyTexture(m_texture, m_secondaryTexture);
        } else {
            m_context->renderNextRhiFrame(m_renderer);
        }
    }

    if (m_mipmap) {
        if (!resourceUpdates)
            resourceUpdates = m_rhi->nextResourceUpdateBatch();
        // going to be expensive - if done every frame - but the user asked for it...
        resourceUpdates->generateMips(m_texture);
    }

    // Do not defer committing the resource updates to the main pass - with
    // multiple layers there can be dependencies, so the texture should be
    // usable once we return.
    m_context->currentFrameCommandBuffer()->resourceUpdate(resourceUpdates);

    root->markDirty(QSGNode::DirtyForceUpdate); // Force matrix, clip, opacity and render list update.

    if (m_recursive)
        markDirtyTexture(); // Continuously update if 'live' and 'recursive'.
}

QImage QSGRhiLayer::toImage() const
{
    if (!m_texture)
        return QImage();

    QRhiCommandBuffer *cb = m_context->currentFrameCommandBuffer();
    QRhiResourceUpdateBatch *resourceUpdates = m_rhi->nextResourceUpdateBatch();
    QRhiReadbackResult result;
    QRhiReadbackDescription readbackDesc(m_texture);
    resourceUpdates->readBackTexture(readbackDesc, &result);

    cb->resourceUpdate(resourceUpdates);

    // Inefficient but what can you do. We need the results right away. This is
    // not something that occurs in a normal rendering process anyway. (used by
    // QQuickItem's grabToImage).
    m_rhi->finish();

    if (result.data.isEmpty()) {
        qWarning("Layer grab failed");
        return QImage();
    }

    // There is no room for negotiation here, the texture is RGBA8, and the
    // readback happens with GL_RGBA on GL, so RGBA8888 is the only option.
    // Also, Quick is always premultiplied alpha.
    const QImage::Format imageFormat = QImage::Format_RGBA8888_Premultiplied;

    const uchar *p = reinterpret_cast<const uchar *>(result.data.constData());
    return QImage(p, result.pixelSize.width(), result.pixelSize.height(), imageFormat).mirrored();
}

QRectF QSGRhiLayer::normalizedTextureSubRect() const
{
    return QRectF(m_mirrorHorizontal ? 1 : 0,
                  m_mirrorVertical ? 0 : 1,
                  m_mirrorHorizontal ? -1 : 1,
                  m_mirrorVertical ? 1 : -1);
}

#include "moc_qsgrhilayer_p.cpp"
