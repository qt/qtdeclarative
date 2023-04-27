// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "rhiitem_p.h"

RhiItemNode::RhiItemNode(RhiItem *item)
    : m_item(item)
{
    m_window = m_item->window();
    Q_ASSERT(m_window);
    connect(m_window, &QQuickWindow::beforeRendering, this, &RhiItemNode::render);
    connect(m_window, &QQuickWindow::screenChanged, this, [this]() {
        if (m_window->effectiveDevicePixelRatio() != m_dpr)
            m_item->update();
    });
}

RhiItemNode::~RhiItemNode()
{
    delete m_renderer;
    delete m_sgWrapperTexture;
    releaseNativeTexture();
}

QSGTexture *RhiItemNode::texture() const
{
    return m_sgWrapperTexture;
}

void RhiItemNode::createNativeTexture()
{
    Q_ASSERT(!m_texture);
    Q_ASSERT(!m_pixelSize.isEmpty());

    m_texture = m_rhi->newTexture(QRhiTexture::RGBA8, m_pixelSize, 1, QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource);
    if (!m_texture->create()) {
        qWarning("Failed to create RhiItem texture of size %dx%d", m_pixelSize.width(), m_pixelSize.height());
        delete m_texture;
        m_texture = nullptr;
    }
}

void RhiItemNode::releaseNativeTexture()
{
    if (m_texture) {
        m_texture->deleteLater();
        m_texture = nullptr;
    }
}

void RhiItemNode::sync()
{
    if (!m_rhi) {
        m_rhi = m_window->rhi();
        if (!m_rhi) {
            qWarning("No QRhi found for window %p, RhiItem will not be functional", m_window);
            return;
        }
    }

    QSize newSize(m_item->explicitTextureWidth(), m_item->explicitTextureHeight());
    if (newSize.isEmpty()) {
        m_dpr = m_window->effectiveDevicePixelRatio();
        const int minTexSize = m_rhi->resourceLimit(QRhi::TextureSizeMin);
        newSize = QSize(qMax<int>(minTexSize, m_item->width()),
                        qMax<int>(minTexSize, m_item->height())) * m_dpr;
    }

    bool needsNew = !m_sgWrapperTexture;
    if (newSize != m_pixelSize) {
        needsNew = true;
        m_pixelSize = newSize;
    }

    if (needsNew) {
        if (m_texture && m_sgWrapperTexture) {
            m_texture->setPixelSize(m_pixelSize);
            if (m_texture->create())
                m_sgWrapperTexture->setTextureSize(m_pixelSize);
            else
                qWarning("Failed to recreate RhiItem texture of size %dx%d", m_pixelSize.width(), m_pixelSize.height());
        } else {
            delete m_sgWrapperTexture;
            m_sgWrapperTexture = nullptr;
            releaseNativeTexture();
            createNativeTexture();
            if (m_texture) {
                m_sgWrapperTexture = new QSGPlainTexture;
                m_sgWrapperTexture->setOwnsTexture(false);
                m_sgWrapperTexture->setTexture(m_texture);
                m_sgWrapperTexture->setTextureSize(m_pixelSize);
                m_sgWrapperTexture->setHasAlphaChannel(m_item->alphaBlending());
                setTexture(m_sgWrapperTexture);
            }
        }
        RhiItemPrivate::get(m_item)->effectiveTextureSize = m_pixelSize;
        emit m_item->effectiveTextureSizeChanged();
        if (m_texture)
            m_renderer->initialize(m_rhi, m_texture);
    }

    if (m_sgWrapperTexture && m_sgWrapperTexture->hasAlphaChannel() != m_item->alphaBlending()) {
        m_sgWrapperTexture->setHasAlphaChannel(m_item->alphaBlending());
        // hasAlphaChannel is mapped to QSGMaterial::Blending in setTexture() so that has to be called again
        setTexture(m_sgWrapperTexture);
    }

    m_renderer->synchronize(m_item);
}

void RhiItemNode::render()
{
    // called before Qt Quick starts recording its main render pass

    if (!m_rhi || !m_texture || !m_renderer)
        return;

    if (!m_renderPending)
        return;

    QSGRendererInterface *rif = m_window->rendererInterface();
    QRhiCommandBuffer *cb = nullptr;
    QRhiSwapChain *swapchain = static_cast<QRhiSwapChain *>(
        rif->getResource(m_window, QSGRendererInterface::RhiSwapchainResource));
    cb = swapchain ? swapchain->currentFrameCommandBuffer()
                   : static_cast<QRhiCommandBuffer *>(rif->getResource(m_window, QSGRendererInterface::RhiRedirectCommandBuffer));
    if (!cb) {
        qWarning("Neither swapchain nor redirected command buffer are available.");
        return;
    }

    m_renderPending = false;
    m_renderer->render(cb);

    markDirty(QSGNode::DirtyMaterial);
    emit textureChanged();
}

void RhiItemNode::scheduleUpdate()
{
    m_renderPending = true;
    m_window->update(); // ensure getting to beforeRendering() at some point
}

void RhiItemNode::setMirrorVertically(bool b)
{
    if (m_rhi->isYUpInFramebuffer())
        b = !b;

    setTextureCoordinatesTransform(b ? QSGSimpleTextureNode::MirrorVertically : QSGSimpleTextureNode::NoTransform);
}

RhiItem::RhiItem(QQuickItem *parent)
    : QQuickItem(*new RhiItemPrivate, parent)
{
    setFlag(ItemHasContents);
}

QSGNode *RhiItem::updatePaintNode(QSGNode *node, UpdatePaintNodeData *)
{
    Q_D(RhiItem);
    RhiItemNode *n = static_cast<RhiItemNode *>(node);

    // Changing to an empty size should not involve destroying and then later
    // recreating the node, because we do not know how expensive the user's
    // renderer setup is. Rather, keep the node if it already exist, and clamp
    // all accesses to width and height. Hence the unusual !n condition here.
    if (!n && (width() <= 0 || height() <= 0))
        return nullptr;

    if (!n) {
        if (!d->node)
            d->node = new RhiItemNode(this);
        n = d->node;
    }

    if (!n->hasRenderer()) {
        RhiItemRenderer *r = createRenderer();
        if (r) {
            r->data = n;
            n->setRenderer(r);
        } else {
            qWarning("No RhiItemRenderer was created; the item will not render");
            delete n;
            d->node = nullptr;
            return nullptr;
        }
    }

    n->sync();

    if (!n->isValid()) {
        delete n;
        d->node = nullptr;
        return nullptr;
    }

    n->setMirrorVertically(d->mirrorVertically);
    n->setFiltering(QSGTexture::Linear);
    n->setRect(0, 0, qMax<int>(0, width()), qMax<int>(0, height()));

    n->scheduleUpdate();

    return n;
}

void RhiItem::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickItem::geometryChange(newGeometry, oldGeometry);
    if (newGeometry.size() != oldGeometry.size())
        update();
}

void RhiItem::releaseResources()
{
    // called on the gui thread if the item is removed from scene

    Q_D(RhiItem);
    d->node = nullptr;
}

void RhiItem::invalidateSceneGraph()
{
    // called on the render thread when the scenegraph is invalidated

    Q_D(RhiItem);
    d->node = nullptr;
}

bool RhiItem::isTextureProvider() const
{
    return true;
}

QSGTextureProvider *RhiItem::textureProvider() const
{
    if (QQuickItem::isTextureProvider()) // e.g. if Item::layer::enabled == true
        return QQuickItem::textureProvider();

    QQuickWindow *w = window();
    if (!w || !w->isSceneGraphInitialized() || QThread::currentThread() != QQuickWindowPrivate::get(w)->context->thread()) {
        qWarning("RhiItem::textureProvider: can only be queried on the rendering thread of an exposed window");
        return nullptr;
    }

    Q_D(const RhiItem);
    if (!d->node) // create a node to have a provider, the texture will be null but that's ok
        d->node = new RhiItemNode(const_cast<RhiItem *>(this));

    return d->node;
}

int RhiItem::explicitTextureWidth() const
{
    Q_D(const RhiItem);
    return d->explicitTextureWidth;
}

void RhiItem::setExplicitTextureWidth(int w)
{
    Q_D(RhiItem);
    if (d->explicitTextureWidth == w)
        return;

    d->explicitTextureWidth = w;
    emit explicitTextureWidthChanged();
    update();
}

int RhiItem::explicitTextureHeight() const
{
    Q_D(const RhiItem);
    return d->explicitTextureHeight;
}

void RhiItem::setExplicitTextureHeight(int h)
{
    Q_D(RhiItem);
    if (d->explicitTextureHeight == h)
        return;

    d->explicitTextureHeight = h;
    emit explicitTextureHeightChanged();
    update();
}

QSize RhiItem::effectiveTextureSize() const
{
    Q_D(const RhiItem);
    return d->effectiveTextureSize;
}

bool RhiItem::alphaBlending() const
{
    Q_D(const RhiItem);
    return d->blend;
}

void RhiItem::setAlphaBlending(bool enable)
{
    Q_D(RhiItem);
    if (d->blend == enable)
        return;

    d->blend = enable;
    emit alphaBlendingChanged();
    update();
}

bool RhiItem::mirrorVertically() const
{
    Q_D(const RhiItem);
    return d->mirrorVertically;
}

void RhiItem::setMirrorVertically(bool enable)
{
    Q_D(RhiItem);
    if (d->mirrorVertically == enable)
        return;

    d->mirrorVertically = enable;
    emit mirrorVerticallyChanged();
    update();
}

void RhiItemRenderer::update()
{
    if (data)
        static_cast<RhiItemNode *>(data)->scheduleUpdate();
}

RhiItemRenderer::~RhiItemRenderer()
{
}

void RhiItemRenderer::initialize(QRhi *rhi, QRhiTexture *outputTexture)
{
    Q_UNUSED(rhi);
    Q_UNUSED(outputTexture);
}

void RhiItemRenderer::synchronize(RhiItem *item)
{
    Q_UNUSED(item);
}

void RhiItemRenderer::render(QRhiCommandBuffer *cb)
{
    Q_UNUSED(cb);
}
