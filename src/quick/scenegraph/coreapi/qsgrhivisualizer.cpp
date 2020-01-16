/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Copyright (C) 2016 Jolla Ltd, author: <gunnar.sletta@jollamobile.com>
** Copyright (C) 2016 Robin Burchell <robin.burchell@viroteck.net>
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

#include "qsgrhivisualizer_p.h"
#include <qmath.h>
#include <QQuickWindow>
#include <private/qsgmaterialrhishader_p.h>
#include <private/qsgshadersourcebuilder_p.h>

QT_BEGIN_NAMESPACE

namespace QSGBatchRenderer
{

#define QSGNODE_TRAVERSE(NODE) for (QSGNode *child = NODE->firstChild(); child; child = child->nextSibling())
#define SHADOWNODE_TRAVERSE(NODE) for (Node *child = NODE->firstChild(); child; child = child->sibling())
#define QSGNODE_DIRTY_PARENT (QSGNode::DirtyNodeAdded \
                              | QSGNode::DirtyOpacity \
                              | QSGNode::DirtyMatrix \
                              | QSGNode::DirtyNodeRemoved)

QMatrix4x4 qsg_matrixForRoot(Node *node);
QRhiVertexInputAttribute::Format qsg_vertexInputFormat(const QSGGeometry::Attribute &a);
QRhiCommandBuffer::IndexFormat qsg_indexFormat(const QSGGeometry *geometry);
QRhiGraphicsPipeline::Topology qsg_topology(int geomDrawMode);

RhiVisualizer::RhiVisualizer(Renderer *renderer)
    : Visualizer(renderer)
{
}

RhiVisualizer::~RhiVisualizer()
{
    releaseResources();
}

void RhiVisualizer::releaseResources()
{
    m_pipelines.releaseResources();

    m_fade.releaseResources();

    m_changeVis.releaseResources();
    m_batchVis.releaseResources();
    m_clipVis.releaseResources();
    m_overdrawVis.releaseResources();
}

void RhiVisualizer::prepareVisualize()
{
    // Called before the render pass has begun (but after preparing the
    // batches). Now is the time to put resource updates to the renderer's
    // current m_resourceUpdates instance.

    if (m_visualizeMode == VisualizeNothing)
        return;

    if (!m_vs.isValid()) {
        m_vs = QSGMaterialRhiShaderPrivate::loadShader(
                    QLatin1String(":/qt-project.org/scenegraph/shaders_ng/visualization.vert.qsb"));
        m_fs = QSGMaterialRhiShaderPrivate::loadShader(
                    QLatin1String(":/qt-project.org/scenegraph/shaders_ng/visualization.frag.qsb"));
    }

    m_fade.prepare(this, m_renderer->m_rhi, m_renderer->m_resourceUpdates, m_renderer->renderPassDescriptor());

    const bool forceUintIndex = m_renderer->m_uint32IndexForRhi;

    switch (m_visualizeMode) {
    case VisualizeBatches:
        m_batchVis.prepare(m_renderer->m_opaqueBatches, m_renderer->m_alphaBatches,
                           this,
                           m_renderer->m_rhi, m_renderer->m_resourceUpdates,
                           forceUintIndex);
        break;
    case VisualizeClipping:
        m_clipVis.prepare(m_renderer->rootNode(), this,
                          m_renderer->m_rhi, m_renderer->m_resourceUpdates);
        break;
    case VisualizeChanges:
        m_changeVis.prepare(m_renderer->m_nodes.value(m_renderer->rootNode()),
                            this,
                            m_renderer->m_rhi, m_renderer->m_resourceUpdates);
        m_visualizeChangeSet.clear();
        break;
    case VisualizeOverdraw:
        m_overdrawVis.prepare(m_renderer->m_nodes.value(m_renderer->rootNode()),
                              this,
                              m_renderer->m_rhi, m_renderer->m_resourceUpdates);
        break;
    default:
        Q_UNREACHABLE();
        break;
    }
}

void RhiVisualizer::visualize()
{
    if (m_visualizeMode == VisualizeNothing)
        return;

    QRhiCommandBuffer *cb = m_renderer->commandBuffer();
    m_fade.render(cb);

    switch (m_visualizeMode) {
    case VisualizeBatches:
        m_batchVis.render(cb);
        break;
    case VisualizeClipping:
        m_clipVis.render(cb);
        break;
    case VisualizeChanges:
        m_changeVis.render(cb);
        break;
    case VisualizeOverdraw:
        m_overdrawVis.render(cb);
        break;
    default:
        Q_UNREACHABLE();
        break;
    }
}

void RhiVisualizer::recordDrawCalls(const QVector<DrawCall> &drawCalls,
                                    QRhiCommandBuffer *cb,
                                    QRhiShaderResourceBindings *srb,
                                    bool blendOneOne)
{
    for (const DrawCall &dc : drawCalls) {
        QRhiGraphicsPipeline *ps = m_pipelines.pipeline(this, m_renderer->m_rhi, srb, m_renderer->renderPassDescriptor(),
                                                        dc.vertex.topology, dc.vertex.format, dc.vertex.stride,
                                                        blendOneOne);
        if (!ps)
            continue;
        cb->setGraphicsPipeline(ps); // no-op if same as the last one
        QRhiCommandBuffer::DynamicOffset dynofs(0, dc.buf.ubufOffset);
        cb->setShaderResources(srb, 1, &dynofs);
        QRhiCommandBuffer::VertexInput vb(dc.buf.vbuf, dc.buf.vbufOffset);
        if (dc.index.count) {
            cb->setVertexInput(0, 1, &vb, dc.buf.ibuf, dc.buf.ibufOffset, dc.index.format);
            cb->drawIndexed(dc.index.count);
        } else {
            cb->setVertexInput(0, 1, &vb);
            cb->draw(dc.vertex.count);
        }
    }
}

const QRhiShaderResourceBinding::StageFlags ubufVisibility =
        QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage;

void RhiVisualizer::Fade::prepare(RhiVisualizer *visualizer,
                                  QRhi *rhi, QRhiResourceUpdateBatch *u, QRhiRenderPassDescriptor *rpDesc)
{
    this->visualizer = visualizer;

    if (!vbuf) {
        float v[] = { -1, 1,   1, 1,   -1, -1,   1, -1 };
        vbuf = rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(v));
        if (!vbuf->build())
            return;
        u->uploadStaticBuffer(vbuf, v);
    }

    if (!ubuf) {
        ubuf = rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, DrawCall::UBUF_SIZE);
        if (!ubuf->build())
            return;
        float bgOpacity = 0.8f;
        if (visualizer->m_visualizeMode == Visualizer::VisualizeBatches)
            bgOpacity = 1.0;
        QMatrix4x4 ident;
        u->updateDynamicBuffer(ubuf, 0, 64, ident.constData()); // matrix
        u->updateDynamicBuffer(ubuf, 64, 64, ident.constData()); // rotation
        float color[4] = { 0.0f, 0.0f, 0.0f, bgOpacity };
        u->updateDynamicBuffer(ubuf, 128, 16, color);
        float pattern = 0.0f;
        u->updateDynamicBuffer(ubuf, 144, 4, &pattern);
        qint32 projection = 0;
        u->updateDynamicBuffer(ubuf, 148, 4, &projection);
    }

    if (!srb) {
        srb = rhi->newShaderResourceBindings();
        srb->setBindings({ QRhiShaderResourceBinding::uniformBuffer(0, ubufVisibility, ubuf) });
        if (!srb->build())
            return;
    }

    if (!ps) {
        ps = rhi->newGraphicsPipeline();
        ps->setTopology(QRhiGraphicsPipeline::TriangleStrip);
        QRhiGraphicsPipeline::TargetBlend blend; // defaults to premul alpha, just what we need
        blend.enable = true;
        ps->setTargetBlends({ blend });
        ps->setShaderStages({ { QRhiShaderStage::Vertex, visualizer->m_vs },
                              { QRhiShaderStage::Fragment, visualizer->m_fs } });
        QRhiVertexInputLayout inputLayout;
        inputLayout.setBindings({ { 2 * sizeof(float) } });
        inputLayout.setAttributes({ { 0, 0, QRhiVertexInputAttribute::Float2, 0 } });
        ps->setVertexInputLayout(inputLayout);
        ps->setShaderResourceBindings(srb);
        ps->setRenderPassDescriptor(rpDesc);
        if (!ps->build())
            return;
    }
}

void RhiVisualizer::Fade::releaseResources()
{
    delete ps;
    ps = nullptr;

    delete srb;
    srb = nullptr;

    delete ubuf;
    ubuf = nullptr;

    delete vbuf;
    vbuf = nullptr;
}

void RhiVisualizer::Fade::render(QRhiCommandBuffer *cb)
{
    cb->setGraphicsPipeline(ps);
    cb->setViewport(visualizer->m_renderer->m_pstate.viewport);
    cb->setShaderResources();
    QRhiCommandBuffer::VertexInput vb(vbuf, 0);
    cb->setVertexInput(0, 1, &vb);
    cb->draw(4);
}

static void fillVertexIndex(RhiVisualizer::DrawCall *dc, QSGGeometry *g, bool withData, bool forceUintIndex)
{
    dc->vertex.topology = qsg_topology(g->drawingMode());
    dc->vertex.format = qsg_vertexInputFormat(g->attributes()[0]);
    dc->vertex.count = g->vertexCount();
    dc->vertex.stride = g->sizeOfVertex();
    if (withData)
        dc->vertex.data = g->vertexData();

    dc->index.format = forceUintIndex ? QRhiCommandBuffer::IndexUInt32 : qsg_indexFormat(g);
    dc->index.count = g->indexCount();
    dc->index.stride = forceUintIndex ? sizeof(quint32) : g->sizeOfIndex();
    if (withData && g->indexCount())
        dc->index.data = g->indexData();
}

static inline uint aligned(uint v, uint byteAlign)
{
    return (v + byteAlign - 1) & ~(byteAlign - 1);
}

static bool ensureBuffer(QRhi *rhi, QRhiBuffer **buf, QRhiBuffer::UsageFlags usage, int newSize)
{
    if (!*buf) {
        *buf = rhi->newBuffer(QRhiBuffer::Dynamic, usage, newSize);
        if (!(*buf)->build())
            return false;
    } else if ((*buf)->size() < newSize) {
        (*buf)->setSize(newSize);
        if (!(*buf)->build())
            return false;
    }
    return true;
}

QRhiGraphicsPipeline *RhiVisualizer::PipelineCache::pipeline(RhiVisualizer *visualizer,
                                                             QRhi *rhi,
                                                             QRhiShaderResourceBindings *srb,
                                                             QRhiRenderPassDescriptor *rpDesc,
                                                             QRhiGraphicsPipeline::Topology topology,
                                                             QRhiVertexInputAttribute::Format vertexFormat,
                                                             quint32 vertexStride,
                                                             bool blendOneOne)
{
    for (int i = 0, ie = pipelines.count(); i != ie; ++i) {
        const Pipeline &p(pipelines.at(i));
        if (p.topology == topology && p.format == vertexFormat && p.stride == vertexStride)
            return p.ps;
    }

    QRhiGraphicsPipeline *ps = rhi->newGraphicsPipeline();
    ps->setTopology(topology);
    QRhiGraphicsPipeline::TargetBlend blend; // premul alpha
    blend.enable = true;
    if (blendOneOne) {
        // only for visualizing overdraw, other modes use premul alpha
        blend.srcColor = QRhiGraphicsPipeline::One;
        blend.dstColor = QRhiGraphicsPipeline::One;
        blend.srcAlpha = QRhiGraphicsPipeline::One;
        blend.dstAlpha = QRhiGraphicsPipeline::One;
    }
    ps->setTargetBlends({ blend });
    ps->setShaderStages({ { QRhiShaderStage::Vertex, visualizer->m_vs },
                          { QRhiShaderStage::Fragment, visualizer->m_fs } });
    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({ { vertexStride } });
    inputLayout.setAttributes({ { 0, 0, vertexFormat, 0 } });
    ps->setVertexInputLayout(inputLayout);
    ps->setShaderResourceBindings(srb);
    ps->setRenderPassDescriptor(rpDesc);
    if (!ps->build())
        return nullptr;

    Pipeline p;
    p.topology = topology;
    p.format = vertexFormat;
    p.stride = vertexStride;
    p.ps = ps;
    pipelines.append(p);

    return ps;
}

void RhiVisualizer::PipelineCache::releaseResources()
{
    for (int i = 0, ie = pipelines.count(); i != ie; ++i)
        delete pipelines.at(i).ps;

    pipelines.clear();
}

void RhiVisualizer::ChangeVis::gather(Node *n)
{
    if (n->type() == QSGNode::GeometryNodeType && n->element()->batch && visualizer->m_visualizeChangeSet.contains(n)) {
        const uint dirty = visualizer->m_visualizeChangeSet.value(n);
        const bool tinted = (dirty & QSGNODE_DIRTY_PARENT) != 0;
        const QColor color = QColor::fromHsvF((rand() & 1023) / 1023.0f, 0.3f, 1.0f);
        const float alpha = 0.5f;

        QMatrix4x4 matrix = visualizer->m_renderer->m_current_projection_matrix;
        if (n->element()->batch->root)
            matrix = matrix * qsg_matrixForRoot(n->element()->batch->root);

        QSGGeometryNode *gn = static_cast<QSGGeometryNode *>(n->sgNode);
        matrix = matrix * *gn->matrix();

        QSGGeometry *g = gn->geometry();
        if (g->attributeCount() >= 1) {
            DrawCall dc;
            memcpy(dc.uniforms.data, matrix.constData(), 64);
            QMatrix4x4 rotation;
            memcpy(dc.uniforms.data + 64, rotation.constData(), 64);
            float c[4] = {
                float(color.redF()) * alpha,
                float(color.greenF()) * alpha,
                float(color.blueF()) * alpha,
                alpha
            };
            memcpy(dc.uniforms.data + 128, c, 16);
            float pattern = tinted ? 0.5f : 0.0f;
            memcpy(dc.uniforms.data + 144, &pattern, 4);
            qint32 projection = 0;
            memcpy(dc.uniforms.data + 148, &projection, 4);

            fillVertexIndex(&dc, g, true, false);
            drawCalls.append(dc);
        }

        // This is because many changes don't propegate their dirty state to the
        // parent so the node updater will not unset these states. They are
        // not used for anything so, unsetting it should have no side effects.
        n->dirtyState = { };
    }

    SHADOWNODE_TRAVERSE(n) {
        gather(child);
    }
}

void RhiVisualizer::ChangeVis::prepare(Node *n, RhiVisualizer *visualizer,
                                       QRhi *rhi, QRhiResourceUpdateBatch *u)
{
    this->visualizer = visualizer;

    drawCalls.clear();
    gather(n);

    if (drawCalls.isEmpty())
        return;

    const int ubufAlign = rhi->ubufAlignment();
    int vbufOffset = 0;
    int ibufOffset = 0;
    int ubufOffset = 0;
    for (RhiVisualizer::DrawCall &dc : drawCalls) {
        dc.buf.vbufOffset = aligned(vbufOffset, 4);
        vbufOffset = dc.buf.vbufOffset + dc.vertex.count * dc.vertex.stride;

        dc.buf.ibufOffset = aligned(ibufOffset, 4);
        ibufOffset = dc.buf.ibufOffset + dc.index.count * dc.index.stride;

        dc.buf.ubufOffset = aligned(ubufOffset, ubufAlign);
        ubufOffset = dc.buf.ubufOffset + DrawCall::UBUF_SIZE;
    }

    ensureBuffer(rhi, &vbuf, QRhiBuffer::VertexBuffer, vbufOffset);
    if (ibufOffset)
        ensureBuffer(rhi, &ibuf, QRhiBuffer::IndexBuffer, ibufOffset);
    const int ubufSize = ubufOffset;
    ensureBuffer(rhi, &ubuf, QRhiBuffer::UniformBuffer, ubufSize);

    for (RhiVisualizer::DrawCall &dc : drawCalls) {
        u->updateDynamicBuffer(vbuf, dc.buf.vbufOffset, dc.vertex.count * dc.vertex.stride, dc.vertex.data);
        dc.buf.vbuf = vbuf;
        if (dc.index.count) {
            u->updateDynamicBuffer(ibuf, dc.buf.ibufOffset, dc.index.count * dc.index.stride, dc.index.data);
            dc.buf.ibuf = ibuf;
        }
        u->updateDynamicBuffer(ubuf, dc.buf.ubufOffset, DrawCall::UBUF_SIZE, dc.uniforms.data);
    }

    if (!srb) {
        srb = rhi->newShaderResourceBindings();
        srb->setBindings({ QRhiShaderResourceBinding::uniformBufferWithDynamicOffset(0, ubufVisibility, ubuf, DrawCall::UBUF_SIZE) });
        if (!srb->build())
            return;
    }
}

void RhiVisualizer::ChangeVis::releaseResources()
{
    delete srb;
    srb = nullptr;

    delete ubuf;
    ubuf = nullptr;

    delete ibuf;
    ibuf = nullptr;

    delete vbuf;
    vbuf = nullptr;
}

void RhiVisualizer::ChangeVis::render(QRhiCommandBuffer *cb)
{
    visualizer->recordDrawCalls(drawCalls, cb, srb);
}

void RhiVisualizer::BatchVis::gather(Batch *b)
{
    if (b->positionAttribute != 0)
        return;

    QMatrix4x4 matrix(visualizer->m_renderer->m_current_projection_matrix);
    if (b->root)
        matrix = matrix * qsg_matrixForRoot(b->root);

    DrawCall dc;

    QMatrix4x4 rotation;
    memcpy(dc.uniforms.data + 64, rotation.constData(), 64);

    QColor color = QColor::fromHsvF((rand() & 1023) / 1023.0, 1.0, 1.0);

    float c[4] = {
        float(color.redF()),
        float(color.greenF()),
        float(color.blueF()),
        1.0f
    };
    memcpy(dc.uniforms.data + 128, c, 16);

    float pattern = b->merged ? 0.0f : 1.0f;
    memcpy(dc.uniforms.data + 144, &pattern, 4);

    qint32 projection = 0;
    memcpy(dc.uniforms.data + 148, &projection, 4);

    if (b->merged) {
        memcpy(dc.uniforms.data, matrix.constData(), 64);

        QSGGeometryNode *gn = b->first->node;
        QSGGeometry *g = gn->geometry();

        fillVertexIndex(&dc, g, false, forceUintIndex);

        for (int ds = 0; ds < b->drawSets.size(); ++ds) {
            const DrawSet &set = b->drawSets.at(ds);
            dc.buf.vbuf = b->vbo.buf;
            dc.buf.vbufOffset = set.vertices;
            dc.buf.ibuf = b->ibo.buf;
            dc.buf.ibufOffset = set.indices;
            dc.index.count = set.indexCount;
            drawCalls.append(dc);
        }
    } else {
        Element *e = b->first;
        int vOffset = 0;
        int iOffset = 0;

        while (e) {
            QSGGeometryNode *gn = e->node;
            QSGGeometry *g = gn->geometry();

            QMatrix4x4 m = matrix * *gn->matrix();
            memcpy(dc.uniforms.data, m.constData(), 64);

            fillVertexIndex(&dc, g, false, forceUintIndex);

            dc.buf.vbuf = b->vbo.buf;
            dc.buf.vbufOffset = vOffset;
            if (g->indexCount()) {
                dc.buf.ibuf = b->ibo.buf;
                dc.buf.ibufOffset = iOffset;
            }

            drawCalls.append(dc);

            vOffset += dc.vertex.count * dc.vertex.stride;
            iOffset += dc.index.count * dc.index.stride;

            e = e->nextInBatch;
        }
    }
}

void RhiVisualizer::BatchVis::prepare(const QDataBuffer<Batch *> &opaqueBatches, const QDataBuffer<Batch *> &alphaBatches,
                                      RhiVisualizer *visualizer,
                                      QRhi *rhi, QRhiResourceUpdateBatch *u,
                                      bool forceUintIndex)
{
    this->visualizer = visualizer;
    this->forceUintIndex = forceUintIndex;

    drawCalls.clear();

    srand(0); // To force random colors to be roughly the same every time..
    for (int i = 0; i < opaqueBatches.size(); ++i)
        gather(opaqueBatches.at(i));
    for (int i = 0; i < alphaBatches.size(); ++i)
        gather(alphaBatches.at(i));

    if (drawCalls.isEmpty())
        return;

    const int ubufAlign = rhi->ubufAlignment();
    int ubufOffset = 0;
    for (RhiVisualizer::DrawCall &dc : drawCalls) {
        dc.buf.ubufOffset = aligned(ubufOffset, ubufAlign);
        ubufOffset = dc.buf.ubufOffset + DrawCall::UBUF_SIZE;
    }

    const int ubufSize = ubufOffset;
    ensureBuffer(rhi, &ubuf, QRhiBuffer::UniformBuffer, ubufSize);

    for (RhiVisualizer::DrawCall &dc : drawCalls)
        u->updateDynamicBuffer(ubuf, dc.buf.ubufOffset, DrawCall::UBUF_SIZE, dc.uniforms.data);

    if (!srb) {
        srb = rhi->newShaderResourceBindings();
        srb->setBindings({ QRhiShaderResourceBinding::uniformBufferWithDynamicOffset(0, ubufVisibility, ubuf, DrawCall::UBUF_SIZE) });
        if (!srb->build())
            return;
    }
}

void RhiVisualizer::BatchVis::releaseResources()
{
    delete srb;
    srb = nullptr;

    delete ubuf;
    ubuf = nullptr;
}

void RhiVisualizer::BatchVis::render(QRhiCommandBuffer *cb)
{
    visualizer->recordDrawCalls(drawCalls, cb, srb);
}

void RhiVisualizer::ClipVis::gather(QSGNode *node)
{
    if (node->type() == QSGNode::ClipNodeType) {
        QSGClipNode *clipNode = static_cast<QSGClipNode *>(node);
        QMatrix4x4 matrix = visualizer->m_renderer->m_current_projection_matrix;
        if (clipNode->matrix())
            matrix = matrix * *clipNode->matrix();

        QSGGeometry *g = clipNode->geometry();
        if (g->attributeCount() >= 1) {
            DrawCall dc;
            memcpy(dc.uniforms.data, matrix.constData(), 64);
            QMatrix4x4 rotation;
            memcpy(dc.uniforms.data + 64, rotation.constData(), 64);
            float c[4] = { 0.2f, 0.0f, 0.0f, 0.2f };
            memcpy(dc.uniforms.data + 128, c, 16);
            float pattern = 0.5f;
            memcpy(dc.uniforms.data + 144, &pattern, 4);
            qint32 projection = 0;
            memcpy(dc.uniforms.data + 148, &projection, 4);
            fillVertexIndex(&dc, g, true, false);
            drawCalls.append(dc);
        }
    }

    QSGNODE_TRAVERSE(node) {
        gather(child);
    }
}

void RhiVisualizer::ClipVis::prepare(QSGNode *node, RhiVisualizer *visualizer,
                                     QRhi *rhi, QRhiResourceUpdateBatch *u)
{
    this->visualizer = visualizer;

    drawCalls.clear();
    gather(node);

    if (drawCalls.isEmpty())
        return;

    const int ubufAlign = rhi->ubufAlignment();
    int vbufOffset = 0;
    int ibufOffset = 0;
    int ubufOffset = 0;
    for (RhiVisualizer::DrawCall &dc : drawCalls) {
        dc.buf.vbufOffset = aligned(vbufOffset, 4);
        vbufOffset = dc.buf.vbufOffset + dc.vertex.count * dc.vertex.stride;

        dc.buf.ibufOffset = aligned(ibufOffset, 4);
        ibufOffset = dc.buf.ibufOffset + dc.index.count * dc.index.stride;

        dc.buf.ubufOffset = aligned(ubufOffset, ubufAlign);
        ubufOffset = dc.buf.ubufOffset + DrawCall::UBUF_SIZE;
    }

    ensureBuffer(rhi, &vbuf, QRhiBuffer::VertexBuffer, vbufOffset);
    if (ibufOffset)
        ensureBuffer(rhi, &ibuf, QRhiBuffer::IndexBuffer, ibufOffset);
    const int ubufSize = ubufOffset;
    ensureBuffer(rhi, &ubuf, QRhiBuffer::UniformBuffer, ubufSize);

    for (RhiVisualizer::DrawCall &dc : drawCalls) {
        u->updateDynamicBuffer(vbuf, dc.buf.vbufOffset, dc.vertex.count * dc.vertex.stride, dc.vertex.data);
        dc.buf.vbuf = vbuf;
        if (dc.index.count) {
            u->updateDynamicBuffer(ibuf, dc.buf.ibufOffset, dc.index.count * dc.index.stride, dc.index.data);
            dc.buf.ibuf = ibuf;
        }
        u->updateDynamicBuffer(ubuf, dc.buf.ubufOffset, DrawCall::UBUF_SIZE, dc.uniforms.data);
    }

    if (!srb) {
        srb = rhi->newShaderResourceBindings();
        srb->setBindings({ QRhiShaderResourceBinding::uniformBufferWithDynamicOffset(0, ubufVisibility, ubuf, DrawCall::UBUF_SIZE) });
        if (!srb->build())
            return;
    }
}

void RhiVisualizer::ClipVis::releaseResources()
{
    delete srb;
    srb = nullptr;

    delete ubuf;
    ubuf = nullptr;

    delete ibuf;
    ibuf = nullptr;

    delete vbuf;
    vbuf = nullptr;
}

void RhiVisualizer::ClipVis::render(QRhiCommandBuffer *cb)
{
    visualizer->recordDrawCalls(drawCalls, cb, srb);
}

void RhiVisualizer::OverdrawVis::gather(Node *n)
{
    if (n->type() == QSGNode::GeometryNodeType && n->element()->batch) {
        QMatrix4x4 matrix = visualizer->m_renderer->m_current_projection_matrix;
        matrix(2, 2) = visualizer->m_renderer->m_zRange;
        matrix(2, 3) = 1.0f - n->element()->order * visualizer->m_renderer->m_zRange;

        if (n->element()->batch->root)
            matrix = matrix * qsg_matrixForRoot(n->element()->batch->root);

        QSGGeometryNode *gn = static_cast<QSGGeometryNode *>(n->sgNode);
        matrix = matrix * *gn->matrix();

        QSGGeometry *g = gn->geometry();
        if (g->attributeCount() >= 1) {
            DrawCall dc;
            memcpy(dc.uniforms.data, matrix.constData(), 64);
            memcpy(dc.uniforms.data + 64, rotation.constData(), 64);

            float c[4];
            const float ca = 0.33f;
            if (n->element()->batch->isOpaque) {
                c[0] = ca * 0.3f; c[1] = ca * 1.0f; c[2] = ca * 0.3f; c[3] = ca;
            } else {
                c[0] = ca * 1.0f; c[1] = ca * 0.3f; c[2] = ca * 0.3f; c[3] = ca;
            }
            memcpy(dc.uniforms.data + 128, c, 16);
            float pattern = 0.0f;
            memcpy(dc.uniforms.data + 144, &pattern, 4);
            qint32 projection = 1;
            memcpy(dc.uniforms.data + 148, &projection, 4);

            fillVertexIndex(&dc, g, true, false);
            drawCalls.append(dc);
        }
    }

    SHADOWNODE_TRAVERSE(n) {
        gather(child);
    }
}

void RhiVisualizer::OverdrawVis::prepare(Node *n, RhiVisualizer *visualizer,
                                         QRhi *rhi, QRhiResourceUpdateBatch *u)
{
    this->visualizer = visualizer;

    step += float(M_PI * 2 / 1000.0);
    if (step > float(M_PI * 2))
        step = 0.0f;

    const float yfix = rhi->isYUpInNDC() ? 1.0f : -1.0f;
    rotation.setToIdentity();
    rotation.translate(0.0f, 0.5f * yfix, 4.0f);
    rotation.scale(2.0f, 2.0f, 1.0f);
    rotation.rotate(-30.0f * yfix, 1.0f, 0.0f, 0.0f);
    rotation.rotate(80.0f * std::sin(step), 0.0f, 1.0f, 0.0f);
    rotation.translate(0.0f, 0.0f, -1.0f);

    drawCalls.clear();
    gather(n);

    if (!box.vbuf) {
        const float v[] = {
            // lower
            -1, 1, 0,   1, 1, 0,
            -1, 1, 0,   -1, -1, 0,
            1, 1, 0,    1, -1, 0,
            -1, -1, 0,  1, -1, 0,

            // upper
            -1, 1, 1,   1, 1, 1,
            -1, 1, 1,   -1, -1, 1,
            1, 1, 1,    1, -1, 1,
            -1, -1, 1,  1, -1, 1,

            // sides
            -1, -1, 0,  -1, -1, 1,
            1, -1, 0,   1, -1, 1,
            -1, 1, 0,   -1, 1, 1,
            1, 1, 0,    1, 1, 1
        };
        box.vbuf = rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(v));
        if (!box.vbuf->build())
            return;
        u->uploadStaticBuffer(box.vbuf, v);
    }

    if (!box.ubuf) {
        box.ubuf = rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, DrawCall::UBUF_SIZE);
        if (!box.ubuf->build())
            return;
        QMatrix4x4 ident;
        u->updateDynamicBuffer(box.ubuf, 0, 64, ident.constData());
        float color[4] = { 0.5f, 0.5f, 1.0f, 1.0f };
        u->updateDynamicBuffer(box.ubuf, 128, 16, color);
        float pattern = 0.0f;
        u->updateDynamicBuffer(box.ubuf, 144, 4, &pattern);
        qint32 projection = 1;
        u->updateDynamicBuffer(box.ubuf, 148, 4, &projection);
    }

    u->updateDynamicBuffer(box.ubuf, 64, 64, rotation.constData());

    if (!box.srb) {
        box.srb = rhi->newShaderResourceBindings();
        box.srb->setBindings({ QRhiShaderResourceBinding::uniformBuffer(0, ubufVisibility, box.ubuf) });
        if (!box.srb->build())
            return;
    }

    if (!box.ps) {
        box.ps = rhi->newGraphicsPipeline();
        box.ps->setTopology(QRhiGraphicsPipeline::Lines);
        box.ps->setLineWidth(2); // may be be ignored (D3D, Metal), but may be used on GL and Vulkan
        QRhiGraphicsPipeline::TargetBlend blend;
        blend.enable = true;
        blend.srcColor = QRhiGraphicsPipeline::One;
        blend.dstColor = QRhiGraphicsPipeline::One;
        blend.srcAlpha = QRhiGraphicsPipeline::One;
        blend.dstAlpha = QRhiGraphicsPipeline::One;
        box.ps->setTargetBlends({ blend });
        box.ps->setShaderStages({ { QRhiShaderStage::Vertex, visualizer->m_vs },
                                  { QRhiShaderStage::Fragment, visualizer->m_fs } });
        QRhiVertexInputLayout inputLayout;
        inputLayout.setBindings({ { 3 * sizeof(float) } });
        inputLayout.setAttributes({ { 0, 0, QRhiVertexInputAttribute::Float3, 0 } });
        box.ps->setVertexInputLayout(inputLayout);
        box.ps->setShaderResourceBindings(box.srb);
        box.ps->setRenderPassDescriptor(visualizer->m_renderer->renderPassDescriptor());
        if (!box.ps->build())
            return;
    }

    if (drawCalls.isEmpty())
        return;

    const int ubufAlign = rhi->ubufAlignment();
    int vbufOffset = 0;
    int ibufOffset = 0;
    int ubufOffset = 0;
    for (RhiVisualizer::DrawCall &dc : drawCalls) {
        dc.buf.vbufOffset = aligned(vbufOffset, 4);
        vbufOffset = dc.buf.vbufOffset + dc.vertex.count * dc.vertex.stride;

        dc.buf.ibufOffset = aligned(ibufOffset, 4);
        ibufOffset = dc.buf.ibufOffset + dc.index.count * dc.index.stride;

        dc.buf.ubufOffset = aligned(ubufOffset, ubufAlign);
        ubufOffset = dc.buf.ubufOffset + DrawCall::UBUF_SIZE;
    }

    ensureBuffer(rhi, &vbuf, QRhiBuffer::VertexBuffer, vbufOffset);
    if (ibufOffset)
        ensureBuffer(rhi, &ibuf, QRhiBuffer::IndexBuffer, ibufOffset);
    const int ubufSize = ubufOffset;
    ensureBuffer(rhi, &ubuf, QRhiBuffer::UniformBuffer, ubufSize);

    for (RhiVisualizer::DrawCall &dc : drawCalls) {
        u->updateDynamicBuffer(vbuf, dc.buf.vbufOffset, dc.vertex.count * dc.vertex.stride, dc.vertex.data);
        dc.buf.vbuf = vbuf;
        if (dc.index.count) {
            u->updateDynamicBuffer(ibuf, dc.buf.ibufOffset, dc.index.count * dc.index.stride, dc.index.data);
            dc.buf.ibuf = ibuf;
        }
        u->updateDynamicBuffer(ubuf, dc.buf.ubufOffset, DrawCall::UBUF_SIZE, dc.uniforms.data);
    }

    if (!srb) {
        srb = rhi->newShaderResourceBindings();
        srb->setBindings({ QRhiShaderResourceBinding::uniformBufferWithDynamicOffset(0, ubufVisibility, ubuf, DrawCall::UBUF_SIZE) });
        if (!srb->build())
            return;
    }
}

void RhiVisualizer::OverdrawVis::releaseResources()
{
    delete srb;
    srb = nullptr;

    delete ubuf;
    ubuf = nullptr;

    delete ibuf;
    ibuf = nullptr;

    delete vbuf;
    vbuf = nullptr;

    delete box.ps;
    box.ps = nullptr;

    delete box.srb;
    box.srb = nullptr;

    delete box.ubuf;
    box.ubuf = nullptr;

    delete box.vbuf;
    box.vbuf = nullptr;
}

void RhiVisualizer::OverdrawVis::render(QRhiCommandBuffer *cb)
{
    cb->setGraphicsPipeline(box.ps);
    cb->setShaderResources();
    QRhiCommandBuffer::VertexInput vb(box.vbuf, 0);
    cb->setVertexInput(0, 1, &vb);
    cb->draw(24);

    visualizer->recordDrawCalls(drawCalls, cb, srb, true);
}

}

QT_END_NAMESPACE
