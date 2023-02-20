// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsgrhilayer_p.h"

#include <private/qqmlglobal_p.h>
#include <private/qsgrenderer_p.h>
#include <private/qsgdefaultrendercontext_p.h>

QSGRhiLayer::QSGRhiLayer(QSGRenderContext *context)
    : QSGLayer(*(new QSGTexturePrivate(this)))
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

qint64 QSGRhiLayer::comparisonKey() const
{
    return qint64(m_texture);
}

bool QSGRhiLayer::hasAlphaChannel() const
{
    return true;
}

bool QSGRhiLayer::hasMipmaps() const
{
    return m_mipmap;
}

QRhiTexture *QSGRhiLayer::rhiTexture() const
{
    return m_texture;
}

void QSGRhiLayer::commitTextureOperations(QRhi *rhi, QRhiResourceUpdateBatch *resourceUpdates)
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

void QSGRhiLayer::setRect(const QRectF &logicalRect)
{
    if (logicalRect == m_logicalRect)
        return;

    m_logicalRect = logicalRect;
    markDirtyTexture();
}

void QSGRhiLayer::setSize(const QSize &pixelSize)
{
    if (pixelSize == m_pixelSize)
        return;

    m_pixelSize = pixelSize;

    if (m_live && m_pixelSize.isNull())
        releaseResources();

    markDirtyTexture();
}

void QSGRhiLayer::setFormat(Format format)
{
    QRhiTexture::Format rhiFormat = QRhiTexture::RGBA8;
    switch (format) {
    case RGBA16F:
        rhiFormat = QRhiTexture::RGBA16F;
        break;
    case RGBA32F:
        rhiFormat = QRhiTexture::RGBA32F;
        break;
    default:
        break;
    }

    if (rhiFormat == m_format)
        return;

    if (m_rhi->isTextureFormatSupported(rhiFormat)) {
        m_format = rhiFormat;
        markDirtyTexture();
    } else {
        qWarning("QSGRhiLayer: Attempted to set unsupported texture format %d", int(rhiFormat));
    }
}

void QSGRhiLayer::setLive(bool live)
{
    if (live == m_live)
        return;

    m_live = live;

    if (m_live && (!m_item || m_pixelSize.isNull()))
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
    if (!m_item || m_pixelSize.isEmpty()) {
        releaseResources();
        m_dirtyTexture = false;
        return;
    }

    int effectiveSamples = m_samples;
    // if no layer.samples was provided use the window's msaa setting
    if (effectiveSamples <= 1)
        effectiveSamples = m_context->msaaSampleCount();

    const bool needsNewRt = !m_rt || m_rt->pixelSize() != m_pixelSize || (m_recursive && !m_secondaryTexture) || (m_texture && m_texture->format() != m_format);
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

        // Not the same as m_context->useDepthBufferFor2D(), only the env.var
        // is to be checked here. Consider a layer with a non-offscreen View3D
        // in it. That still needs a depth buffer, even when the 2D content
        // renders without relying on it (i.e. RenderMode2DNoDepthBuffer does
        // not imply not having a depth/stencil attachment for the render
        // target! The env.var serves as a hard switch, on the other hand, and
        // that will likely break 3D for instance but that's fine)
        static bool depthBufferEnabled = qEnvironmentVariableIsEmpty("QSG_NO_DEPTH_BUFFER");

        if (m_multisampling) {
            releaseResources();
            m_msaaColorBuffer = m_rhi->newRenderBuffer(QRhiRenderBuffer::Color, m_pixelSize, effectiveSamples);
            if (!m_msaaColorBuffer->create()) {
                qWarning("Failed to build multisample color buffer for layer of size %dx%d, sample count %d",
                         m_pixelSize.width(), m_pixelSize.height(), effectiveSamples);
                releaseResources();
                return;
            }
            m_texture = m_rhi->newTexture(m_format, m_pixelSize, 1, textureFlags);
            if (!m_texture->create()) {
                qWarning("Failed to build texture for layer of size %dx%d", m_pixelSize.width(), m_pixelSize.height());
                releaseResources();
                return;
            }
            if (depthBufferEnabled) {
                m_ds = m_rhi->newRenderBuffer(QRhiRenderBuffer::DepthStencil, m_pixelSize, effectiveSamples);
                if (!m_ds->create()) {
                    qWarning("Failed to build depth-stencil buffer for layer");
                    releaseResources();
                    return;
                }
            }
            QRhiTextureRenderTargetDescription desc;
            QRhiColorAttachment color0(m_msaaColorBuffer);
            color0.setResolveTexture(m_texture);
            desc.setColorAttachments({ color0 });
            if (depthBufferEnabled)
                desc.setDepthStencilBuffer(m_ds);
            m_rt = m_rhi->newTextureRenderTarget(desc);
            m_rtRp = m_rt->newCompatibleRenderPassDescriptor();
            if (!m_rtRp) {
                qWarning("Failed to build render pass descriptor for layer");
                releaseResources();
                return;
            }
            m_rt->setRenderPassDescriptor(m_rtRp);
            if (!m_rt->create()) {
                qWarning("Failed to build texture render target for layer");
                releaseResources();
                return;
            }
        } else {
            releaseResources();
            m_texture = m_rhi->newTexture(m_format, m_pixelSize, 1, textureFlags);
            if (!m_texture->create()) {
                qWarning("Failed to build texture for layer of size %dx%d", m_pixelSize.width(), m_pixelSize.height());
                releaseResources();
                return;
            }
            if (depthBufferEnabled) {
                m_ds = m_rhi->newRenderBuffer(QRhiRenderBuffer::DepthStencil, m_pixelSize);
                if (!m_ds->create()) {
                    qWarning("Failed to build depth-stencil buffer for layer");
                    releaseResources();
                    return;
                }
            }
            QRhiColorAttachment color0(m_texture);
            if (m_recursive) {
                // Here rt is associated with m_secondaryTexture instead of m_texture.
                // We will issue a copy to m_texture afterwards.
                m_secondaryTexture = m_rhi->newTexture(m_format, m_pixelSize, 1, textureFlags);
                if (!m_secondaryTexture->create()) {
                    qWarning("Failed to build texture for layer of size %dx%d", m_pixelSize.width(), m_pixelSize.height());
                    releaseResources();
                    return;
                }
                color0.setTexture(m_secondaryTexture);
            }
            if (depthBufferEnabled)
                m_rt = m_rhi->newTextureRenderTarget({ color0, m_ds });
            else
                m_rt = m_rhi->newTextureRenderTarget({ color0 });
            m_rtRp = m_rt->newCompatibleRenderPassDescriptor();
            if (!m_rtRp) {
                qWarning("Failed to build render pass descriptor for layer");
                releaseResources();
                return;
            }
            m_rt->setRenderPassDescriptor(m_rtRp);
            if (!m_rt->create()) {
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
        const bool useDepth = m_context->useDepthBufferFor2D();
        const QSGRendererInterface::RenderMode renderMode = useDepth ? QSGRendererInterface::RenderMode2D
                                                                     : QSGRendererInterface::RenderMode2DNoDepthBuffer;
        m_renderer = m_context->createRenderer(renderMode);
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
    m_renderer->setDeviceRect(m_pixelSize);
    m_renderer->setViewportRect(m_pixelSize);
    QRectF mirrored; // in logical coordinates (no dpr) since this gets passed to setProjectionMatrixToRect()
    if (m_rhi->isYUpInFramebuffer()) {
        mirrored = QRectF(m_mirrorHorizontal ? m_logicalRect.right() : m_logicalRect.left(),
                          m_mirrorVertical ? m_logicalRect.bottom() : m_logicalRect.top(),
                          m_mirrorHorizontal ? -m_logicalRect.width() : m_logicalRect.width(),
                          m_mirrorVertical ? -m_logicalRect.height() : m_logicalRect.height());
    } else {
        mirrored = QRectF(m_mirrorHorizontal ? m_logicalRect.right() : m_logicalRect.left(),
                          m_mirrorVertical ? m_logicalRect.top() : m_logicalRect.bottom(),
                          m_mirrorHorizontal ? -m_logicalRect.width() : m_logicalRect.width(),
                          m_mirrorVertical ? m_logicalRect.height() : -m_logicalRect.height());
    }
    QSGAbstractRenderer::MatrixTransformFlags matrixFlags;
    if (!m_rhi->isYUpInNDC())
        matrixFlags |= QSGAbstractRenderer::MatrixTransformFlipY;
    m_renderer->setProjectionMatrixToRect(mirrored, matrixFlags);
    m_renderer->setClearColor(Qt::transparent);
    m_renderer->setRenderTarget({ m_rt, m_rtRp, m_context->currentFrameCommandBuffer() });

    QRhiResourceUpdateBatch *resourceUpdates = nullptr;

    // render with our own "sub-renderer" (this will just add a render pass to the command buffer)
    if (m_multisampling) {
        m_context->renderNextFrame(m_renderer);
    } else {
        if (m_recursive) {
            m_context->renderNextFrame(m_renderer);
            if (!resourceUpdates)
                resourceUpdates = m_rhi->nextResourceUpdateBatch();
            resourceUpdates->copyTexture(m_texture, m_secondaryTexture);
        } else {
            m_context->renderNextFrame(m_renderer);
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

    // There is little room for negotiation here, the texture is one of the formats from setFormat.
    // Also, Quick is always premultiplied alpha.
    QImage::Format imageFormat = QImage::Format_RGBA8888_Premultiplied;
    if (m_format == QRhiTexture::RGBA16F)
        imageFormat = QImage::Format_RGBA16FPx4_Premultiplied;
    else if (m_format == QRhiTexture::RGBA32F)
        imageFormat = QImage::Format_RGBA32FPx4_Premultiplied;

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
