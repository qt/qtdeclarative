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

#include "qsgd3d12renderer_p.h"
#include "qsgd3d12rendercontext_p.h"
#include "qsgd3d12material_p.h"
#include <private/qsgnodeupdater_p.h>

QT_BEGIN_NAMESPACE

#define QSGNODE_TRAVERSE(NODE) for (QSGNode *child = NODE->firstChild(); child; child = child->nextSibling())

#define DECLARE_DEBUG_VAR(variable) \
    static bool debug_ ## variable() \
    { static bool value = qgetenv("QSG_RENDERER_DEBUG").contains(QT_STRINGIFY(variable)); return value; }

DECLARE_DEBUG_VAR(build)
DECLARE_DEBUG_VAR(change)
DECLARE_DEBUG_VAR(render)

class DummyUpdater : public QSGNodeUpdater
{
public:
    void updateState(QSGNode *) { };
};

QSGD3D12Renderer::QSGD3D12Renderer(QSGRenderContext *context)
    : QSGRenderer(context),
      m_renderList(16),
      m_vboData(1024),
      m_iboData(256),
      m_cboData(256)
{
    setNodeUpdater(new DummyUpdater);
}

void QSGD3D12Renderer::renderScene(GLuint fboId)
{
    Q_UNUSED(fboId);

    struct B : public QSGBindable {
        void bind() const { }
    } bindable;

    QSGRenderer::renderScene(bindable);
}

// Search through the node set and remove nodes that are leaves of other
// nodes in the same set.
static QSet<QSGNode *> qsg_filterSubTree(const QSet<QSGNode *> &nodes, QSGRootNode *root)
{
    QSet<QSGNode *> result = nodes;
    for (QSGNode *node : nodes) {
        QSGNode *n = node;
        while (n != root) {
            if (result.contains(n)) {
                result.remove(node);
                break;
            }
            n = n->parent();
        }
    }
    return result;
}

void QSGD3D12Renderer::updateMatrices(QSGNode *node, QSGTransformNode *xform)
{
    if (node->isSubtreeBlocked())
        return;

    if (node->type() == QSGNode::TransformNodeType) {
        QSGTransformNode *tn = static_cast<QSGTransformNode *>(node);
        if (xform)
            tn->setCombinedMatrix(xform->combinedMatrix() * tn->matrix());
        else
            tn->setCombinedMatrix(tn->matrix());
        QSGNODE_TRAVERSE(node)
            updateMatrices(child, tn);

    } else {
        if (node->type() == QSGNode::GeometryNodeType || node->type() == QSGNode::ClipNodeType) {
           static_cast<QSGBasicGeometryNode *>(node)->setMatrix(xform ? &xform->combinedMatrix() : 0);
        }
        QSGNODE_TRAVERSE(node)
            updateMatrices(child, xform);
    }
}

void QSGD3D12Renderer::buildRenderList(QSGNode *node, QSGClipNode *clip)
{
    if (node->isSubtreeBlocked())
        return;

    if (node->type() == QSGNode::GeometryNodeType || node->type() == QSGNode::ClipNodeType) {
        QSGBasicGeometryNode *gn = static_cast<QSGBasicGeometryNode *>(node);
        QSGGeometry *g = gn->geometry();

        Element e;
        e.node = gn;

        if (g->vertexCount() > 0) {
            e.vboOffset = m_vboData.size();
            const int vertexSize = g->sizeOfVertex() * g->vertexCount();
            m_vboData.resize(m_vboData.size() + vertexSize);
            memcpy(m_vboData.data() + e.vboOffset, g->vertexData(), vertexSize);
        }

        if (g->indexCount() > 0) {
            e.iboOffset = m_iboData.size();
            e.iboStride = g->sizeOfIndex();
            const int indexSize = e.iboStride * g->indexCount();
            m_iboData.resize(m_iboData.size() + indexSize);
            memcpy(m_iboData.data() + e.iboOffset, g->indexData(), indexSize);
        }

        if (node->type() == QSGNode::GeometryNodeType) {
            QSGD3D12Material *m = static_cast<QSGD3D12Material *>(static_cast<QSGGeometryNode *>(node)->activeMaterial());
            e.cboOffset = m_cboData.size();
            e.cboSize = m->constantBufferSize();
            m_cboData.resize(m_cboData.size() + e.cboSize);
        }

        m_renderList.add(e);

        gn->setClipList(clip);
        if (node->type() == QSGNode::ClipNodeType)
            clip = static_cast<QSGClipNode *>(node);
    }

    QSGNODE_TRAVERSE(node)
        buildRenderList(child, clip);
}

void QSGD3D12Renderer::render()
{
    QSGD3D12RenderContext *rc = static_cast<QSGD3D12RenderContext *>(context());
    m_engine = rc->engine();
    m_engine->beginFrame();

    if (m_rebuild) {
        // This changes everything, so discard all cached states
        m_rebuild = false;
        m_dirtyTransformNodes.clear();
        m_dirtyTransformNodes.insert(rootNode());

        m_renderList.reset();
        m_vboData.reset();
        m_iboData.reset();
        m_cboData.reset();

        buildRenderList(rootNode(), 0);

        m_engine->setVertexBuffer(m_vboData.data(), m_vboData.size());
        m_engine->setIndexBuffer(m_iboData.data(), m_iboData.size());

        if (Q_UNLIKELY(debug_build())) {
            qDebug("renderList: %d elements in total", m_renderList.size());
            for (int i = 0; i < m_renderList.size(); ++i) {
                const Element &e = m_renderList.at(i);
                qDebug() << " - " << e.vboOffset << e.iboOffset << e.cboOffset << e.cboSize << e.node;
            }
        }
    }

    if (m_dirtyTransformNodes.size()) {
        const QSet<QSGNode *> subTreeRoots = qsg_filterSubTree(m_dirtyTransformNodes, rootNode());
        for (QSGNode *node : subTreeRoots) {
            // First find the parent transform so we have the accumulated
            // matrix up until this point.
            QSGTransformNode *xform = 0;
            QSGNode *n = node;
            if (n->type() == QSGNode::TransformNodeType)
                n = node->parent();
            while (n != rootNode() && n->type() != QSGNode::TransformNodeType)
                n = n->parent();
            if (n != rootNode())
                xform = static_cast<QSGTransformNode *>(n);

            // Then update in the subtree
            updateMatrices(node, xform);
        }
    }

    if (m_dirtyOpaqueElements) {
        m_dirtyOpaqueElements = false;
        m_opaqueElements.clear();
        m_opaqueElements.resize(m_renderList.size());
        for (int i = 0; i < m_renderList.size(); ++i) {
            const Element &e = m_renderList.at(i);
            if (e.node->type() == QSGNode::GeometryNodeType) {
                const QSGGeometryNode *gn = static_cast<QSGGeometryNode *>(e.node);
                if (gn->inheritedOpacity() > 0.999f && ((gn->activeMaterial()->flags() & QSGMaterial::Blending) == 0))
                    m_opaqueElements.setBit(i);
            }
        }
    }

    renderElements();

    m_engine->endFrame();
    m_engine = nullptr;
}

void QSGD3D12Renderer::nodeChanged(QSGNode *node, QSGNode::DirtyState state)
{
    // note that with DirtyNodeRemoved the window and all the graphics engine may already be gone

    if (Q_UNLIKELY(debug_change())) {
        QDebug debug = qDebug();
        debug << "dirty:";
        if (state & QSGNode::DirtyGeometry)
            debug << "Geometry";
        if (state & QSGNode::DirtyMaterial)
            debug << "Material";
        if (state & QSGNode::DirtyMatrix)
            debug << "Matrix";
        if (state & QSGNode::DirtyNodeAdded)
            debug << "Added";
        if (state & QSGNode::DirtyNodeRemoved)
            debug << "Removed";
        if (state & QSGNode::DirtyOpacity)
            debug << "Opacity";
        if (state & QSGNode::DirtySubtreeBlocked)
            debug << "SubtreeBlocked";
        if (state & QSGNode::DirtyForceUpdate)
            debug << "ForceUpdate";

        // when removed, some parts of the node could already have been destroyed
        // so don't debug it out.
        if (state & QSGNode::DirtyNodeRemoved)
            debug << (void *) node << node->type();
        else
            debug << node;
    }

    if (state & (QSGNode::DirtyNodeAdded
                 | QSGNode::DirtyNodeRemoved
                 | QSGNode::DirtySubtreeBlocked
                 | QSGNode::DirtyGeometry
                 | QSGNode::DirtyForceUpdate))
        m_rebuild = true;

    if (state & QSGNode::DirtyMatrix)
        m_dirtyTransformNodes << node;

    if (state & QSGNode::DirtyMaterial)
        m_dirtyOpaqueElements = true;

    QSGRenderer::nodeChanged(node, state);
}

void QSGD3D12Renderer::renderElements()
{
    QRect r = viewportRect();
    r.setY(deviceRect().bottom() - r.bottom());
    m_engine->queueViewport(r);
    m_engine->queueScissor(r);
    m_engine->queueSetRenderTarget();
    m_engine->queueClearRenderTarget(clearColor());
    m_engine->queueClearDepthStencil(1, 0);

    m_pipelineState.premulBlend = false;
    m_pipelineState.depthEnable = true;
    m_pipelineState.depthWrite = true;

    m_current_projection_matrix = projectionMatrix();

    // First do opaque...
    // The algorithm is quite simple. We traverse the list back-to-from
    // and for every item, we start a second traversal from this point
    // and draw all elements which have identical material. Then we clear
    // the bit for this in the rendered list so we don't draw it again
    // when we come to that index.
    QBitArray rendered = m_opaqueElements;
    for (int i = m_renderList.size() - 1; i >= 0; --i) {
        if (rendered.testBit(i)) {
            renderElement(i);
            for (int j = i - 1; j >= 0; --j) {
                if (rendered.testBit(j)) {
                    const QSGGeometryNode *gni = static_cast<QSGGeometryNode *>(m_renderList.at(i).node);
                    const QSGGeometryNode *gnj = static_cast<QSGGeometryNode *>(m_renderList.at(j).node);
                    if (gni->clipList() == gnj->clipList()
                            && gni->inheritedOpacity() == gnj->inheritedOpacity()
                            && gni->geometry()->drawingMode() == gnj->geometry()->drawingMode()
                            && gni->geometry()->attributes() == gnj->geometry()->attributes()) {
                        const QSGMaterial *ami = gni->activeMaterial();
                        const QSGMaterial *amj = gnj->activeMaterial();
                        if (ami->type() == amj->type()
                                && ami->flags() == amj->flags()
                                && ami->compare(amj) == 0) {
                            renderElement(j);
                            rendered.clearBit(j);
                        }
                    }
                }
            }
        }
    }

    m_pipelineState.premulBlend = true;
    m_pipelineState.depthWrite = false;

    // ...then the alpha ones
    for (int i = 0; i < m_renderList.size(); ++i) {
        if (m_renderList.at(i).node->type() == QSGNode::GeometryNodeType && !m_opaqueElements.testBit(i))
            renderElement(i);
    }
}

void QSGD3D12Renderer::renderElement(int elementIndex)
{
    Element &e = m_renderList.at(elementIndex);
    Q_ASSERT(e.node->type() == QSGNode::GeometryNodeType);

    if (e.vboOffset < 0)
        return;

    Q_ASSERT(e.cboOffset >= 0);

    const QSGGeometryNode *gn = static_cast<QSGGeometryNode *>(e.node);
    if (Q_UNLIKELY(debug_render()))
        qDebug() << "renderElement:" << elementIndex << gn << e.vboOffset << e.iboOffset << gn->inheritedOpacity() << gn->clipList();

    if (gn->inheritedOpacity() < 0.001f) // pretty much invisible, don't draw it
        return;

    QSGD3D12Material::RenderState::DirtyStates dirtyState = QSGD3D12Material::RenderState::DirtyMatrix;
    m_current_projection_matrix = projectionMatrix();
    qreal scale = 1.0 / m_renderList.size();
    m_current_projection_matrix(2, 2) = scale;
    m_current_projection_matrix(2, 3) = 1.0f - (elementIndex + 1) * scale;
    m_current_model_view_matrix = gn->matrix() ? *gn->matrix() : QMatrix4x4();
    m_current_determinant = m_current_model_view_matrix.determinant();
    if (gn->inheritedOpacity() != m_current_opacity) {
        m_current_opacity = gn->inheritedOpacity();
        dirtyState |= QSGD3D12Material::RenderState::DirtyOpacity;
    }

    const QSGGeometry *g = gn->geometry();
    QSGD3D12Material *m = static_cast<QSGD3D12Material *>(gn->activeMaterial());

    if (m->type() != m_lastMaterialType)
        m->preparePipeline(&m_pipelineState.shaders);

    if (!e.cboPrepared) {
        e.cboPrepared = true;
        dirtyState = QSGD3D12Material::RenderState::DirtyAll;
    }

    quint8 *cboPtr = nullptr;
    if (e.cboSize > 0)
        cboPtr = m_cboData.data() + e.cboOffset;

    qDebug() << "ds" << dirtyState;
    QSGD3D12Material::UpdateResults updRes = m->updatePipeline(QSGD3D12Material::makeRenderState(this, dirtyState),
                                                               &m_pipelineState.shaders,
                                                               cboPtr);
    // For now there is no way to have extra SRVs and such. Once texturing is
    // introduced, the above update call will have to be able to affect the
    // root signature and communicate the need for SRVs or UAVs to the engine.

    if (updRes.testFlag(QSGD3D12Material::UpdatedConstantBuffer))
        m_engine->setConstantBuffer(m_cboData.data(), m_cboData.size());

    // Input element layout
    m_pipelineState.inputElements.resize(g->attributeCount());
    const QSGGeometry::Attribute *attrs = g->attributes();
    quint32 offset = 0;
    for (int i = 0; i < g->attributeCount(); ++i) {
        QSGD3D12InputElement &ie(m_pipelineState.inputElements[i]);
        static const char *semanticNames[] = { "UNKNOWN", "POSITION", "COLOR", "TEXCOORD" };
        Q_ASSERT(attrs[i].semantic >= 1 && attrs[i].semantic < _countof(semanticNames));
        const int tupleSize = attrs[i].tupleSize;
        ie.name = semanticNames[attrs[i].semantic];
        ie.offset = offset;
        // ### move format mapping to engine
        static const QSGD3D12Format formatMap_ub[] = { FmtUnknown,
                                                       FmtUNormByte,
                                                       FmtUNormByte2,
                                                       FmtUnknown,
                                                       FmtUNormByte4 };
        static const QSGD3D12Format formatMap_f[] = { FmtUnknown,
                                                      FmtFloat,
                                                      FmtFloat2,
                                                      FmtFloat3,
                                                      FmtFloat4 };
        switch (attrs[i].type) {
        case QSGGeometry::TypeUnsignedByte:
            ie.format = formatMap_ub[tupleSize];
            offset += tupleSize;
            break;
        case QSGGeometry::TypeFloat:
            ie.format = formatMap_f[tupleSize];
            offset += sizeof(float) * tupleSize;
            break;
        case QSGGeometry::TypeByte:
        case QSGGeometry::TypeInt:
        case QSGGeometry::TypeUnsignedInt:
        case QSGGeometry::TypeShort:
        case QSGGeometry::TypeUnsignedShort:
            qFatal("QSGD3D12Renderer: attribute type 0x%x is not currently supported", attrs[i].type);
            break;
        }
        if (ie.format == FmtUnknown)
            qFatal("QSGD3D12Renderer: unsupported tuple size for attribute type 0x%x", attrs[i].type);

        // There is one buffer with interleaved data so the slot is always 0.
        ie.slot = 0;
    }

    m_lastMaterialType = m->type();

    // ### line width / point size ??

    m_engine->setPipelineState(m_pipelineState);

    if (e.iboOffset >= 0) {
        // ### move format mapping to engine
        QSGD3D12Format indexFormat;
        const QSGGeometry::Type indexType = QSGGeometry::Type(g->indexType());
        switch (indexType) {
        case QSGGeometry::TypeUnsignedShort:
            indexFormat = FmtUnsignedShort;
            break;
        case QSGGeometry::TypeUnsignedInt:
            indexFormat = FmtUnsignedInt;
            break;
        default:
            qFatal("QSGD3D12Renderer: unsupported index data type 0x%x", indexType);
            break;
        };
        m_engine->queueDraw(QSGGeometry::DrawingMode(g->drawingMode()), g->indexCount(), e.vboOffset, g->sizeOfVertex(),
                            e.cboOffset,
                            e.iboOffset / e.iboStride, indexFormat);
    } else {
        m_engine->queueDraw(QSGGeometry::DrawingMode(g->drawingMode()), g->vertexCount(), e.vboOffset, g->sizeOfVertex(),
                            e.cboOffset);
    }
}

QT_END_NAMESPACE
