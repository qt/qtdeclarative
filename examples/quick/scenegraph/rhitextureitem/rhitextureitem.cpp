// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "rhitextureitem.h"
#include <QFile>

//! [nodector]
RhiItemNode::RhiItemNode(RhiItem *item)
    : m_item(item)
{
    m_window = m_item->window();
    connect(m_window, &QQuickWindow::beforeRendering, this, &RhiItemNode::render,
            Qt::DirectConnection);
//! [nodector]
    connect(m_window, &QQuickWindow::screenChanged, this, [this]() {
        if (m_window->effectiveDevicePixelRatio() != m_dpr)
            m_item->update();
    }, Qt::DirectConnection);
}

QSGTexture *RhiItemNode::texture() const
{
    return m_sgTexture.get();
}

//! [nodesync]
void RhiItemNode::sync()
{
    if (!m_rhi) {
        m_rhi = m_window->rhi();
        if (!m_rhi) {
            qWarning("No QRhi found for window %p, RhiItem will not be functional", m_window);
            return;
        }
    }

    m_dpr = m_window->effectiveDevicePixelRatio();
    const int minTexSize = m_rhi->resourceLimit(QRhi::TextureSizeMin);
    QSize newSize = QSize(qMax<int>(minTexSize, m_item->width()),
                          qMax<int>(minTexSize, m_item->height())) * m_dpr;

    bool needsNew = !m_sgTexture;
    if (newSize != m_pixelSize) {
        needsNew = true;
        m_pixelSize = newSize;
    }

    if (needsNew) {
        QRhiTexture *texture = m_rhi->newTexture(QRhiTexture::RGBA8, m_pixelSize, 1, QRhiTexture::RenderTarget);
        if (texture->create()) {
            m_sgTexture.reset(m_window->createTextureFromRhiTexture(texture));
            setTexture(m_sgTexture.get());
            m_renderer->initialize(m_rhi, texture);
        } else {
            qWarning("Failed to create RhiItem texture of size %dx%d", m_pixelSize.width(), m_pixelSize.height());
            delete texture;
        }
    }

    m_renderer->synchronize(m_item);
}
//! [nodesync]

//! [noderender]
void RhiItemNode::render()
{
    // called before Qt Quick starts recording its main render pass

    if (!isValid() || !m_renderPending)
        return;

    QRhiSwapChain *swapchain = m_window->swapChain();
    QSGRendererInterface *rif = m_window->rendererInterface();

    // For completeness, handle both cases: on-screen QQuickWindow vs.
    // off-screen QQuickWindow e.g. by using QQuickRenderControl to redirect
    // into a texture. For the application's purposes just handling the first
    // (swapchain is non-null) would be sufficient.
    QRhiCommandBuffer *cb = swapchain ? swapchain->currentFrameCommandBuffer()
                                      : static_cast<QRhiCommandBuffer *>(
                                          rif->getResource(m_window, QSGRendererInterface::RhiRedirectCommandBuffer));

    if (!cb) {
        qWarning("Neither swapchain nor redirected command buffer are available.");
        return;
    }

    m_renderPending = false;
    m_renderer->render(cb);

    markDirty(QSGNode::DirtyMaterial);
    emit textureChanged();
}
//! [noderender]

void RhiItemNode::scheduleUpdate()
{
    m_renderPending = true;
    m_window->update(); // ensure getting to beforeRendering() at some point
}

RhiItem::RhiItem(QQuickItem *parent)
    : QQuickItem(parent)
{
    setFlag(ItemHasContents);
}

QSGNode *RhiItem::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    // Changing to an empty size should not involve destroying and then later
    // recreating the node, because we do not know how expensive the user's
    // renderer setup is. Rather, keep the node if it already exist, and clamp
    // all accesses to width and height. Hence the unusual !oldNode condition here.
    if (!oldNode && (width() <= 0 || height() <= 0))
        return nullptr;

    RhiItemNode *n = static_cast<RhiItemNode *>(oldNode);
    if (!n) {
        if (!node)
            node = new RhiItemNode(this);
        if (!node->hasRenderer()) {
            RhiItemRenderer *r = createRenderer();
            if (r) {
                r->data = node;
                node->setRenderer(r);
            } else {
                qWarning("No RhiItemRenderer was created; the item will not render");
                delete node;
                node = nullptr;
                return nullptr;
            }
        }
        n = node;
    }

    n->sync();

    if (!n->isValid()) {
        delete n;
        node = nullptr;
        return nullptr;
    }

    if (window()->rhi()->isYUpInFramebuffer())
        n->setTextureCoordinatesTransform(QSGSimpleTextureNode::MirrorVertically);

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

    node = nullptr;
}

void RhiItem::invalidateSceneGraph()
{
    // called on the render thread when the scenegraph is invalidated

    node = nullptr;
}

bool RhiItem::isTextureProvider() const
{
    return true;
}

QSGTextureProvider *RhiItem::textureProvider() const
{
    if (QQuickItem::isTextureProvider()) // e.g. if Item::layer::enabled == true
        return QQuickItem::textureProvider();

    if (!node) // create a node to have a provider, the texture will be null but that's ok
        node = new RhiItemNode(const_cast<RhiItem *>(this));

    return node;
}

void RhiItemRenderer::update()
{
    if (data)
        static_cast<RhiItemNode *>(data)->scheduleUpdate();
}

RhiItemRenderer *ExampleRhiItem::createRenderer()
{
    return new ExampleRhiItemRenderer;
}

void ExampleRhiItem::setAngle(float a)
{
    if (m_angle == a)
        return;

    m_angle = a;
    emit angleChanged();
    update();
}

static QShader getShader(const QString &name)
{
    QFile f(name);
    if (f.open(QIODevice::ReadOnly))
        return QShader::fromSerialized(f.readAll());

    return QShader();
}

//! [exampleinit]
void ExampleRhiItemRenderer::initialize(QRhi *rhi, QRhiTexture *outputTexture)
{
    m_rhi = rhi;

    if (m_output && m_output != outputTexture) {
        m_rt.reset();
        m_rp.reset();
    }

    m_output = outputTexture;

    if (!m_ds) {
        m_ds.reset(m_rhi->newRenderBuffer(QRhiRenderBuffer::DepthStencil, m_output->pixelSize()));
        m_ds->create();
    } else if (m_ds->pixelSize() != m_output->pixelSize()) {
        m_ds->setPixelSize(m_output->pixelSize());
        m_ds->create();
    }

    if (!m_rt) {
        m_rt.reset(m_rhi->newTextureRenderTarget({ { m_output }, m_ds.get() }));
        m_rp.reset(m_rt->newCompatibleRenderPassDescriptor());
        m_rt->setRenderPassDescriptor(m_rp.get());
        m_rt->create();
    }

    if (!scene.vbuf) {
        createGeometry();

        const quint32 vsize = m_vertices.size() * 3 * sizeof(float);
        const quint32 nsize = m_normals.size() * 3 * sizeof(float);
        const quint32 vbufSize = vsize + nsize;
        scene.vbuf.reset(m_rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, vbufSize));
        scene.vbuf->create();
//! [exampleinit]

        scene.resourceUpdates = m_rhi->nextResourceUpdateBatch();
        scene.resourceUpdates->uploadStaticBuffer(scene.vbuf.get(), 0, vsize, m_vertices.constData());
        scene.resourceUpdates->uploadStaticBuffer(scene.vbuf.get(), vsize, nsize, m_normals.constData());

        scene.ubuf.reset(m_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 64));
        scene.ubuf->create();

        scene.srb.reset(m_rhi->newShaderResourceBindings());
        scene.srb->setBindings({
            QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage, scene.ubuf.get()),
        });
        scene.srb->create();

        scene.ps.reset(m_rhi->newGraphicsPipeline());
        scene.ps->setDepthTest(true);
        scene.ps->setDepthWrite(true);
        scene.ps->setShaderStages({
            { QRhiShaderStage::Vertex, getShader(QLatin1String(":/scenegraph/rhitextureitem/shaders/logo.vert.qsb")) },
            { QRhiShaderStage::Fragment, getShader(QLatin1String(":/scenegraph/rhitextureitem/shaders/logo.frag.qsb")) }
        });
        QRhiVertexInputLayout inputLayout;
        inputLayout.setBindings({
            { 3 * sizeof(float) },
            { 3 * sizeof(float) }
        });
        inputLayout.setAttributes({
            { 0, 0, QRhiVertexInputAttribute::Float3, 0 },
            { 1, 1, QRhiVertexInputAttribute::Float3, 0 }
        });
        scene.ps->setVertexInputLayout(inputLayout);
        scene.ps->setShaderResourceBindings(scene.srb.get());
        scene.ps->setRenderPassDescriptor(m_rp.get());
        scene.ps->create();
    }
}

//! [examplesync]
void ExampleRhiItemRenderer::synchronize(RhiItem *rhiItem)
{
    // called on the render thread (if there is one), while the main (gui) thread is blocked

    ExampleRhiItem *item = static_cast<ExampleRhiItem *>(rhiItem);
    if (item->angle() != scene.logoAngle)
        scene.logoAngle = item->angle();
}
//! [examplesync]

//! [examplerender]
void ExampleRhiItemRenderer::render(QRhiCommandBuffer *cb)
{
    QRhiResourceUpdateBatch *rub = scene.resourceUpdates;
    if (rub)
        scene.resourceUpdates = nullptr;
    else
        rub = m_rhi->nextResourceUpdateBatch();

    const QMatrix4x4 matrix = m_rhi->clipSpaceCorrMatrix() * calculateModelViewMatrix();
    rub->updateDynamicBuffer(scene.ubuf.get(), 0, 64, matrix.constData());

    const QColor clearColor = QColor::fromRgbF(0.5f, 0.5f, 0.7f, 1.0f);
    cb->beginPass(m_rt.get(), clearColor, { 1.0f, 0 }, rub);

    cb->setGraphicsPipeline(scene.ps.get());
    const QSize outputSize = m_output->pixelSize();
    cb->setViewport(QRhiViewport(0, 0, outputSize.width(), outputSize.height()));
    cb->setShaderResources();
    const QRhiCommandBuffer::VertexInput vbufBindings[] = {
        { scene.vbuf.get(), 0 },
        { scene.vbuf.get(), quint32(m_vertices.size() * 3 * sizeof(float)) }
    };
    cb->setVertexInput(0, 2, vbufBindings);
    cb->draw(m_vertices.size());

    cb->endPass();
}
//! [examplerender]

void ExampleRhiItemRenderer::createGeometry()
{
    m_vertices.clear();
    m_normals.clear();

    qreal x1 = +0.06f;
    qreal y1 = -0.14f;
    qreal x2 = +0.14f;
    qreal y2 = -0.06f;
    qreal x3 = +0.08f;
    qreal y3 = +0.00f;
    qreal x4 = +0.30f;
    qreal y4 = +0.22f;

    quad(x1, y1, x2, y2, y2, x2, y1, x1);
    quad(x3, y3, x4, y4, y4, x4, y3, x3);

    extrude(x1, y1, x2, y2);
    extrude(x2, y2, y2, x2);
    extrude(y2, x2, y1, x1);
    extrude(y1, x1, x1, y1);
    extrude(x3, y3, x4, y4);
    extrude(x4, y4, y4, x4);
    extrude(y4, x4, y3, x3);

    const qreal Pi = M_PI;
    const int NumSectors = 100;

    for (int i = 0; i < NumSectors; ++i) {
        qreal angle1 = (i * 2 * Pi) / NumSectors;
        qreal x5 = 0.30 * sin(angle1);
        qreal y5 = 0.30 * cos(angle1);
        qreal x6 = 0.20 * sin(angle1);
        qreal y6 = 0.20 * cos(angle1);

        qreal angle2 = ((i + 1) * 2 * Pi) / NumSectors;
        qreal x7 = 0.20 * sin(angle2);
        qreal y7 = 0.20 * cos(angle2);
        qreal x8 = 0.30 * sin(angle2);
        qreal y8 = 0.30 * cos(angle2);

        quad(x5, y5, x6, y6, x7, y7, x8, y8);

        extrude(x6, y6, x7, y7);
        extrude(x8, y8, x5, y5);
    }

    for (int i = 0; i < m_vertices.size(); i++)
        m_vertices[i] *= 2.0f;
}

void ExampleRhiItemRenderer::quad(qreal x1, qreal y1, qreal x2, qreal y2, qreal x3, qreal y3, qreal x4, qreal y4)
{
    m_vertices << QVector3D(x1, y1, -0.05f);
    m_vertices << QVector3D(x2, y2, -0.05f);
    m_vertices << QVector3D(x4, y4, -0.05f);

    m_vertices << QVector3D(x3, y3, -0.05f);
    m_vertices << QVector3D(x4, y4, -0.05f);
    m_vertices << QVector3D(x2, y2, -0.05f);

    QVector3D n = QVector3D::normal(QVector3D(x2 - x1, y2 - y1, 0.0f), QVector3D(x4 - x1, y4 - y1, 0.0f));

    m_normals << n;
    m_normals << n;
    m_normals << n;

    m_normals << n;
    m_normals << n;
    m_normals << n;

    m_vertices << QVector3D(x4, y4, 0.05f);
    m_vertices << QVector3D(x2, y2, 0.05f);
    m_vertices << QVector3D(x1, y1, 0.05f);

    m_vertices << QVector3D(x2, y2, 0.05f);
    m_vertices << QVector3D(x4, y4, 0.05f);
    m_vertices << QVector3D(x3, y3, 0.05f);

    n = QVector3D::normal(QVector3D(x2 - x4, y2 - y4, 0.0f), QVector3D(x1 - x4, y1 - y4, 0.0f));

    m_normals << n;
    m_normals << n;
    m_normals << n;

    m_normals << n;
    m_normals << n;
    m_normals << n;
}

void ExampleRhiItemRenderer::extrude(qreal x1, qreal y1, qreal x2, qreal y2)
{
    m_vertices << QVector3D(x1, y1, +0.05f);
    m_vertices << QVector3D(x2, y2, +0.05f);
    m_vertices << QVector3D(x1, y1, -0.05f);

    m_vertices << QVector3D(x2, y2, -0.05f);
    m_vertices << QVector3D(x1, y1, -0.05f);
    m_vertices << QVector3D(x2, y2, +0.05f);

    QVector3D n = QVector3D::normal(QVector3D(x2 - x1, y2 - y1, 0.0f), QVector3D(0.0f, 0.0f, -0.1f));

    m_normals << n;
    m_normals << n;
    m_normals << n;

    m_normals << n;
    m_normals << n;
    m_normals << n;
}

QMatrix4x4 ExampleRhiItemRenderer::calculateModelViewMatrix() const
{
    QMatrix4x4 modelview;
    modelview.rotate(scene.logoAngle, 0.0f, 1.0f, 0.0f);
    modelview.rotate(scene.logoAngle, 1.0f, 0.0f, 0.0f);
    modelview.rotate(scene.logoAngle, 0.0f, 0.0f, 1.0f);
    modelview.translate(0.0f, -0.2f, 0.0f);
    return modelview;
}
