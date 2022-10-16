// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "customrender.h"
#include <QSGTextureProvider>
#include <QSGRenderNode>
#include <QSGTransformNode>
#include <QSGRendererInterface>
#include <QQuickWindow>
#include <QFile>
#include <private/qrhi_p.h>
#include <private/qsgrendernode_p.h>

class CustomRenderNode : public QSGRenderNode
{
public:
    CustomRenderNode(QQuickWindow *window);
    virtual ~CustomRenderNode();

    void setVertices(const QList<QVector2D> &vertices);

    void prepare() override;
    void render(const RenderState *state) override;
    void releaseResources() override;
    RenderingFlags flags() const override;
    QSGRenderNode::StateFlags changedStates() const override;

protected:
    QQuickWindow *m_window = nullptr;
    QRhiBuffer *m_vertexBuffer = nullptr;
    QRhiBuffer *m_uniformBuffer = nullptr;
    QRhiShaderResourceBindings *m_resourceBindings = nullptr;
    QRhiGraphicsPipeline *m_pipeLine = nullptr;
    QList<QRhiShaderStage> m_shaders;
    bool m_verticesDirty = true;
    QList<QVector2D> m_vertices;
};

CustomRenderNode::CustomRenderNode(QQuickWindow *window) : m_window(window)
{
    Q_ASSERT(QFile::exists(":/scenegraph/customrendernode/shaders/customrender.vert.qsb"));
    Q_ASSERT(QFile::exists(":/scenegraph/customrendernode/shaders/customrender.frag.qsb"));

    QFile file;
    file.setFileName(":/scenegraph/customrendernode/shaders/customrender.vert.qsb");
    file.open(QFile::ReadOnly);
    m_shaders.append(
            QRhiShaderStage(QRhiShaderStage::Vertex, QShader::fromSerialized(file.readAll())));

    file.close();
    file.setFileName(":/scenegraph/customrendernode/shaders/customrender.frag.qsb");
    file.open(QFile::ReadOnly);
    m_shaders.append(
            QRhiShaderStage(QRhiShaderStage::Fragment, QShader::fromSerialized(file.readAll())));
}

CustomRenderNode::~CustomRenderNode()
{
    if (m_pipeLine)
        delete m_pipeLine;
    if (m_resourceBindings)
        delete m_resourceBindings;
    if (m_vertexBuffer)
        delete m_vertexBuffer;
    if (m_uniformBuffer)
        delete m_uniformBuffer;
}

void CustomRenderNode::setVertices(const QList<QVector2D> &vertices)
{
    if (m_vertices == vertices)
        return;

    m_verticesDirty = true;
    m_vertices = vertices;

    markDirty(QSGNode::DirtyGeometry);
}

void CustomRenderNode::releaseResources()
{
    if (m_vertexBuffer) {
        delete m_vertexBuffer;
        m_vertexBuffer = nullptr;
    }

    if (m_uniformBuffer) {
        delete m_uniformBuffer;
        m_uniformBuffer = nullptr;
    }

    if (m_pipeLine) {
        delete m_pipeLine;
        m_pipeLine = nullptr;
    }

    if (m_resourceBindings) {
        delete m_resourceBindings;
        m_resourceBindings = nullptr;
    }

}

QSGRenderNode::RenderingFlags CustomRenderNode::flags() const
{
    // We are rendering 2D content directly into the scene graph
    return { QSGRenderNode::NoExternalRendering | QSGRenderNode::DepthAwareRendering };
}

QSGRenderNode::StateFlags CustomRenderNode::changedStates() const
{
    return {QSGRenderNode::StateFlag::ViewportState | QSGRenderNode::StateFlag::CullState};
}

void CustomRenderNode::prepare()
{
    QSGRendererInterface *renderInterface = m_window->rendererInterface();
    QRhiSwapChain *swapChain = static_cast<QRhiSwapChain *>(
            renderInterface->getResource(m_window, QSGRendererInterface::RhiSwapchainResource));
    QRhi *rhi = static_cast<QRhi *>(
            renderInterface->getResource(m_window, QSGRendererInterface::RhiResource));
    Q_ASSERT(swapChain);
    Q_ASSERT(rhi);

    QRhiResourceUpdateBatch *resourceUpdates = rhi->nextResourceUpdateBatch();

    if (m_verticesDirty) {
        if (m_vertexBuffer) {
            delete m_vertexBuffer;
            m_vertexBuffer = nullptr;
        }
        m_verticesDirty = false;
    }

    if (!m_vertexBuffer) {
        m_vertexBuffer = rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer,
                                        m_vertices.count() * sizeof(QVector2D));
        m_vertexBuffer->create();
        resourceUpdates->uploadStaticBuffer(m_vertexBuffer, m_vertices.constData());
    }

    if (!m_uniformBuffer) {
        m_uniformBuffer = rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 68);
        m_uniformBuffer->create();
    }

    if (!m_resourceBindings) {
        m_resourceBindings = rhi->newShaderResourceBindings();
        m_resourceBindings->setBindings({ QRhiShaderResourceBinding::uniformBuffer(
                0,
                QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage,
                m_uniformBuffer) });
        m_resourceBindings->create();
    }

    if (!m_pipeLine) {

        m_pipeLine = rhi->newGraphicsPipeline();

        //
        // If layer.enabled == true on our QQuickItem, the rendering face is flipped for
        // backends with isYUpInFrameBuffer == true (OpenGL). This does not happen with
        // RHI backends with isYUpInFrameBuffer == false. We swap the triangle winding
        // order to work around this.
        //
        m_pipeLine->setFrontFace(QSGRenderNodePrivate::get(this)->m_rt.rt->resourceType() == QRhiResource::TextureRenderTarget
                                                 && rhi->isYUpInFramebuffer()
                                         ? QRhiGraphicsPipeline::CW
                                         : QRhiGraphicsPipeline::CCW);
        m_pipeLine->setCullMode(QRhiGraphicsPipeline::Back);
        m_pipeLine->setTopology(QRhiGraphicsPipeline::TriangleStrip);
        QRhiGraphicsPipeline::TargetBlend blend;
        blend.enable = true;
        m_pipeLine->setTargetBlends({ blend });
        m_pipeLine->setShaderResourceBindings(m_resourceBindings);
        m_pipeLine->setShaderStages(m_shaders.cbegin(), m_shaders.cend());
        m_pipeLine->setDepthTest(true);
        QRhiVertexInputLayout inputLayout;
        inputLayout.setBindings({ { 2 * sizeof(float) } });
        inputLayout.setAttributes({ { 0, 0, QRhiVertexInputAttribute::Float2, 0 } });
        m_pipeLine->setVertexInputLayout(inputLayout);
        m_pipeLine->setRenderPassDescriptor(QSGRenderNodePrivate::get(this)->m_rt.rpDesc);
        m_pipeLine->create();
    }

    QMatrix4x4 mvp = *projectionMatrix() * *matrix();
    float opacity = inheritedOpacity();

    resourceUpdates->updateDynamicBuffer(m_uniformBuffer, 0, 64, mvp.constData());
    resourceUpdates->updateDynamicBuffer(m_uniformBuffer, 64, 4, &opacity);

    swapChain->currentFrameCommandBuffer()->resourceUpdate(resourceUpdates);
}

void CustomRenderNode::render(const RenderState *state)
{

    QSGRendererInterface *renderInterface = m_window->rendererInterface();
    QRhiSwapChain *swapChain = static_cast<QRhiSwapChain *>(
            renderInterface->getResource(m_window, QSGRendererInterface::RhiSwapchainResource));
    Q_ASSERT(swapChain);

    QRhiCommandBuffer *cb = swapChain->currentFrameCommandBuffer();
    Q_ASSERT(cb);

    cb->setGraphicsPipeline(m_pipeLine);
    QSize renderTargetSize = QSGRenderNodePrivate::get(this)->m_rt.rt->pixelSize();
    cb->setViewport(QRhiViewport(0, 0, renderTargetSize.width(), renderTargetSize.height()));
    cb->setShaderResources();
    QRhiCommandBuffer::VertexInput vertexBindings[] = { { m_vertexBuffer, 0 } };
    cb->setVertexInput(0, 1, vertexBindings);
    cb->draw(m_vertices.count());
}

CustomRender::CustomRender(QQuickItem *parent) : QQuickItem(parent)
{
    setFlag(ItemHasContents, true);
    connect(this, &CustomRender::verticesChanged, this, &CustomRender::update);
}

const QList<QVector2D> &CustomRender::vertices() const
{
    return m_vertices;
}

void CustomRender::setVertices(const QList<QVector2D> &newVertices)
{
    if (m_vertices == newVertices)
        return;

    m_vertices = newVertices;
    emit verticesChanged();
}

QSGNode *CustomRender::updatePaintNode(QSGNode *old, UpdatePaintNodeData *)
{
    CustomRenderNode *node = static_cast<CustomRenderNode *>(old);

    if (!node)
        node = new CustomRenderNode(window());

    node->setVertices(m_vertices);

    return node;
}
