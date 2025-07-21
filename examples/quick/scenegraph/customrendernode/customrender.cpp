// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "customrender.h"
#include <QSGTextureProvider>
#include <QSGRenderNode>
#include <QSGTransformNode>
#include <QQuickWindow>
#include <QFile>

#include <rhi/qrhi.h>

//![node]
class CustomRenderNode : public QSGRenderNode
{
public:
    CustomRenderNode(QQuickWindow *window);

    void setVertices(const QList<QVector2D> &vertices);

    void prepare() override;
    void render(const RenderState *state) override;
    void releaseResources() override;
    RenderingFlags flags() const override;
    QSGRenderNode::StateFlags changedStates() const override;

protected:
    QQuickWindow *m_window;
    std::unique_ptr<QRhiBuffer> m_vertexBuffer;
    std::unique_ptr<QRhiBuffer> m_uniformBuffer;
    std::unique_ptr<QRhiShaderResourceBindings> m_resourceBindings;
    std::unique_ptr<QRhiGraphicsPipeline> m_pipeline;
    QList<QRhiShaderStage> m_shaders;
    bool m_verticesDirty = true;
    QList<QVector2D> m_vertices;
};
//![node]

CustomRenderNode::CustomRenderNode(QQuickWindow *window)
    : m_window(window)
{
    QFile file;
    file.setFileName(":/scenegraph/customrendernode/shaders/customrender.vert.qsb");
    if (!file.open(QFile::ReadOnly))
        qFatal("Failed to load vertex shader");
    m_shaders.append(QRhiShaderStage(QRhiShaderStage::Vertex, QShader::fromSerialized(file.readAll())));

    file.close();
    file.setFileName(":/scenegraph/customrendernode/shaders/customrender.frag.qsb");
    if (!file.open(QFile::ReadOnly))
        qFatal("Failed to load fragment shader");
    m_shaders.append(QRhiShaderStage(QRhiShaderStage::Fragment, QShader::fromSerialized(file.readAll())));
}

void CustomRenderNode::setVertices(const QList<QVector2D> &vertices)
{
    if (m_vertices == vertices)
        return;

    m_verticesDirty = true;
    m_vertices = vertices;

    markDirty(QSGNode::DirtyGeometry);
}

//![node-release]
void CustomRenderNode::releaseResources()
{
    m_vertexBuffer.reset();
    m_uniformBuffer.reset();
    m_pipeline.reset();
    m_resourceBindings.reset();
}
//![node-release]

//![node-flags]
QSGRenderNode::RenderingFlags CustomRenderNode::flags() const
{
    // We are rendering 2D content directly into the scene graph using QRhi, no
    // direct usage of a 3D API. Hence NoExternalRendering. This is a minor
    // optimization.

    // Additionally, the node takes the item transform into account by relying
    // on projectionMatrix() and matrix() (see prepare()) and never rendering at
    // other Z coordinates. Hence DepthAwareRendering. This is a potentially
    // bigger optimization.

    return QSGRenderNode::NoExternalRendering | QSGRenderNode::DepthAwareRendering;
}
//![node-flags]

QSGRenderNode::StateFlags CustomRenderNode::changedStates() const
{
    // In Qt 6 only ViewportState and ScissorState matter, the rest is ignored.
    return QSGRenderNode::StateFlag::ViewportState | QSGRenderNode::StateFlag::CullState;
}

//![node-prepare]
void CustomRenderNode::prepare()
{
    QRhi *rhi = m_window->rhi();
    QRhiResourceUpdateBatch *resourceUpdates = rhi->nextResourceUpdateBatch();

    if (m_verticesDirty) {
        m_vertexBuffer.reset();
        m_verticesDirty = false;
    }

    if (!m_vertexBuffer) {
        m_vertexBuffer.reset(rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer,
                                            m_vertices.count() * sizeof(QVector2D)));
        m_vertexBuffer->create();
        resourceUpdates->uploadStaticBuffer(m_vertexBuffer.get(), m_vertices.constData());
    }
//![node-prepare]
    if (!m_uniformBuffer) {
        m_uniformBuffer.reset(rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 68));
        m_uniformBuffer->create();
    }

    if (!m_resourceBindings) {
        m_resourceBindings.reset(rhi->newShaderResourceBindings());
        m_resourceBindings->setBindings({ QRhiShaderResourceBinding::uniformBuffer(
                0,
                QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage,
                m_uniformBuffer.get()) });
        m_resourceBindings->create();
    }

    if (!m_pipeline) {
        m_pipeline.reset(rhi->newGraphicsPipeline());

        // If layer.enabled == true on our QQuickItem and layer.textureMirroring is the
        // default MirrorVertically, the winding order needs to be inverted when
        // isYUpInFrameBuffer == true (OpenGL). This does not happen with other backends,
        // unless textureMirroring is changed so that the situation is inverted, meaning
        // GL needs no adjustments while others need the opposite winding order. Here we
        // choose the replicate, to a degree, what the scenegraph renderer does, but note that
        // this is only correct as long as textureMirroring is not changed from the default.
        m_pipeline->setFrontFace(renderTarget()->resourceType() == QRhiResource::TextureRenderTarget
                                                 && rhi->isYUpInFramebuffer()
                                         ? QRhiGraphicsPipeline::CW
                                         : QRhiGraphicsPipeline::CCW);
        m_pipeline->setCullMode(QRhiGraphicsPipeline::Back);
        m_pipeline->setTopology(QRhiGraphicsPipeline::TriangleStrip);
        QRhiGraphicsPipeline::TargetBlend blend;
        blend.enable = true;
        m_pipeline->setTargetBlends({ blend });
        m_pipeline->setShaderResourceBindings(m_resourceBindings.get());
        m_pipeline->setShaderStages(m_shaders.cbegin(), m_shaders.cend());
        m_pipeline->setDepthTest(true);
        QRhiVertexInputLayout inputLayout;
        inputLayout.setBindings({ { 2 * sizeof(float) } });
        inputLayout.setAttributes({ { 0, 0, QRhiVertexInputAttribute::Float2, 0 } });
        m_pipeline->setVertexInputLayout(inputLayout);
        m_pipeline->setRenderPassDescriptor(renderTarget()->renderPassDescriptor());
        m_pipeline->create();
    }

    const QMatrix4x4 mvp = *projectionMatrix() * *matrix();
    const float opacity = inheritedOpacity();

    resourceUpdates->updateDynamicBuffer(m_uniformBuffer.get(), 0, 64, mvp.constData());
    resourceUpdates->updateDynamicBuffer(m_uniformBuffer.get(), 64, 4, &opacity);
    commandBuffer()->resourceUpdate(resourceUpdates);
}

//![node-render]
void CustomRenderNode::render(const RenderState *)
{
    QRhiCommandBuffer *cb = commandBuffer();
    cb->setGraphicsPipeline(m_pipeline.get());
    QSize renderTargetSize = renderTarget()->pixelSize();
    cb->setViewport(QRhiViewport(0, 0, renderTargetSize.width(), renderTargetSize.height()));
    cb->setShaderResources();
    QRhiCommandBuffer::VertexInput vertexBindings[] = { { m_vertexBuffer.get(), 0 } };
    cb->setVertexInput(0, 1, vertexBindings);
    cb->draw(m_vertices.count());
}
//![node-render]

//![item-ctor]
CustomRender::CustomRender(QQuickItem *parent)
    : QQuickItem(parent)
{
    setFlag(ItemHasContents, true);
    connect(this, &CustomRender::verticesChanged, this, &CustomRender::update);
}
//![item-ctor]

QList<QVector2D> CustomRender::vertices() const
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

//![item-update]
QSGNode *CustomRender::updatePaintNode(QSGNode *old, UpdatePaintNodeData *)
{
    CustomRenderNode *node = static_cast<CustomRenderNode *>(old);

    if (!node)
        node = new CustomRenderNode(window());

    node->setVertices(m_vertices);

    return node;
}
//![item-update]
