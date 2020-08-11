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

#include "qsgbatchrenderer_p.h"
#include <private/qsgshadersourcebuilder_p.h>

#include <QQuickWindow>

#include <qmath.h>

#include <QtCore/QElapsedTimer>
#include <QtCore/QtNumeric>

#include <QtGui/QGuiApplication>
#include <QtGui/QOpenGLFramebufferObject>
#include <QtGui/QOpenGLVertexArrayObject>
#include <QtGui/QOpenGLFunctions_1_0>
#include <QtGui/QOpenGLFunctions_3_2_Core>

#include <private/qnumeric_p.h>
#include <private/qquickprofiler_p.h>
#include "qsgmaterialrhishader_p.h"

#include "qsgopenglvisualizer_p.h"
#include "qsgrhivisualizer_p.h"

#include <qtquick_tracepoints_p.h>

#include <algorithm>

#ifndef GL_DOUBLE
   #define GL_DOUBLE 0x140A
#endif

QT_BEGIN_NAMESPACE

#ifndef QT_NO_DEBUG
Q_QUICK_PRIVATE_EXPORT bool qsg_test_and_clear_material_failure();
#endif

extern QByteArray qsgShaderRewriter_insertZAttributes(const char *input, QSurfaceFormat::OpenGLContextProfile profile);

int qt_sg_envInt(const char *name, int defaultValue);

namespace QSGBatchRenderer
{

#define DECLARE_DEBUG_VAR(variable) \
    static bool debug_ ## variable() \
    { static bool value = qgetenv("QSG_RENDERER_DEBUG").contains(QT_STRINGIFY(variable)); return value; }
DECLARE_DEBUG_VAR(render)
DECLARE_DEBUG_VAR(build)
DECLARE_DEBUG_VAR(change)
DECLARE_DEBUG_VAR(upload)
DECLARE_DEBUG_VAR(roots)
DECLARE_DEBUG_VAR(dump)
DECLARE_DEBUG_VAR(noalpha)
DECLARE_DEBUG_VAR(noopaque)
DECLARE_DEBUG_VAR(noclip)
#undef DECLARE_DEBUG_VAR

static QElapsedTimer qsg_renderer_timer;

#define QSGNODE_TRAVERSE(NODE) for (QSGNode *child = NODE->firstChild(); child; child = child->nextSibling())
#define SHADOWNODE_TRAVERSE(NODE) for (Node *child = NODE->firstChild(); child; child = child->sibling())

static inline int size_of_type(GLenum type)
{
    static int sizes[] = {
        sizeof(char),
        sizeof(unsigned char),
        sizeof(short),
        sizeof(unsigned short),
        sizeof(int),
        sizeof(unsigned int),
        sizeof(float),
        2,
        3,
        4,
        sizeof(double)
    };
    Q_ASSERT(type >= QSGGeometry::ByteType && type <= QSGGeometry::DoubleType);
    return sizes[type - QSGGeometry::ByteType];
}

bool qsg_sort_element_increasing_order(Element *a, Element *b) { return a->order < b->order; }
bool qsg_sort_element_decreasing_order(Element *a, Element *b) { return a->order > b->order; }
bool qsg_sort_batch_is_valid(Batch *a, Batch *b) { return a->first && !b->first; }
bool qsg_sort_batch_increasing_order(Batch *a, Batch *b) { return a->first->order < b->first->order; }
bool qsg_sort_batch_decreasing_order(Batch *a, Batch *b) { return a->first->order > b->first->order; }

QSGMaterial::Flag QSGMaterial_FullMatrix = (QSGMaterial::Flag) (QSGMaterial::RequiresFullMatrix & ~QSGMaterial::RequiresFullMatrixExceptTranslate);

struct QMatrix4x4_Accessor
{
    float m[4][4];
    int flagBits;

    static bool isTranslate(const QMatrix4x4 &m) { return ((const QMatrix4x4_Accessor &) m).flagBits <= 0x1; }
    static bool isScale(const QMatrix4x4 &m) { return ((const QMatrix4x4_Accessor &) m).flagBits <= 0x2; }
    static bool is2DSafe(const QMatrix4x4 &m) { return ((const QMatrix4x4_Accessor &) m).flagBits < 0x8; }
};

const float OPAQUE_LIMIT                = 0.999f;

const uint DYNAMIC_VERTEX_INDEX_BUFFER_THRESHOLD = 4;
const int VERTEX_BUFFER_BINDING = 0;
const int ZORDER_BUFFER_BINDING = VERTEX_BUFFER_BINDING + 1;

static inline uint aligned(uint v, uint byteAlign)
{
    return (v + byteAlign - 1) & ~(byteAlign - 1);
}

QRhiVertexInputAttribute::Format qsg_vertexInputFormat(const QSGGeometry::Attribute &a)
{
    switch (a.type) {
    case QSGGeometry::FloatType:
        if (a.tupleSize == 4)
            return QRhiVertexInputAttribute::Float4;
        if (a.tupleSize == 3)
            return QRhiVertexInputAttribute::Float3;
        if (a.tupleSize == 2)
            return QRhiVertexInputAttribute::Float2;
        if (a.tupleSize == 1)
            return QRhiVertexInputAttribute::Float;
        break;
    case QSGGeometry::UnsignedByteType:
        if (a.tupleSize == 4)
            return QRhiVertexInputAttribute::UNormByte4;
        if (a.tupleSize == 2)
            return QRhiVertexInputAttribute::UNormByte2;
        if (a.tupleSize == 1)
            return QRhiVertexInputAttribute::UNormByte;
        break;
    default:
        break;
    }
    qWarning("Unsupported attribute type 0x%x with %d components", a.type, a.tupleSize);
    Q_UNREACHABLE();
    return QRhiVertexInputAttribute::Float;
}

static QRhiVertexInputLayout calculateVertexInputLayout(const QSGMaterialRhiShader *s, const QSGGeometry *geometry, bool batchable)
{
    Q_ASSERT(geometry);
    const QSGMaterialRhiShaderPrivate *sd = QSGMaterialRhiShaderPrivate::get(s);
    if (!sd->vertexShader) {
        qWarning("No vertex shader in QSGMaterialRhiShader %p", s);
        return QRhiVertexInputLayout();
    }

    const int attrCount = geometry->attributeCount();
    QVarLengthArray<QRhiVertexInputAttribute, 8> inputAttributes;
    inputAttributes.reserve(attrCount + 1);
    int offset = 0;
    for (int i = 0; i < attrCount; ++i) {
        const QSGGeometry::Attribute &a = geometry->attributes()[i];
        if (!sd->vertexShader->vertexInputLocations.contains(a.position)) {
            qWarning("Vertex input %d is present in material but not in shader. This is wrong.",
                     a.position);
        }
        inputAttributes.append(QRhiVertexInputAttribute(VERTEX_BUFFER_BINDING, a.position, qsg_vertexInputFormat(a), offset));
        offset += a.tupleSize * size_of_type(a.type);
    }
    if (batchable) {
        inputAttributes.append(QRhiVertexInputAttribute(ZORDER_BUFFER_BINDING, sd->vertexShader->qt_order_attrib_location,
                                                        QRhiVertexInputAttribute::Float, 0));
    }

    Q_ASSERT(VERTEX_BUFFER_BINDING == 0 && ZORDER_BUFFER_BINDING == 1); // not very flexible
    QVarLengthArray<QRhiVertexInputBinding, 2> inputBindings;
    inputBindings.append(QRhiVertexInputBinding(geometry->sizeOfVertex()));
    if (batchable)
        inputBindings.append(QRhiVertexInputBinding(sizeof(float)));

    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings(inputBindings.cbegin(), inputBindings.cend());
    inputLayout.setAttributes(inputAttributes.cbegin(), inputAttributes.cend());

    return inputLayout;
}

QRhiCommandBuffer::IndexFormat qsg_indexFormat(const QSGGeometry *geometry)
{
    switch (geometry->indexType()) {
    case QSGGeometry::UnsignedShortType:
        return QRhiCommandBuffer::IndexUInt16;
        break;
    case QSGGeometry::UnsignedIntType:
        return QRhiCommandBuffer::IndexUInt32;
        break;
    default:
        Q_UNREACHABLE();
        return QRhiCommandBuffer::IndexUInt16;
    }
}

QRhiGraphicsPipeline::Topology qsg_topology(int geomDrawMode)
{
    QRhiGraphicsPipeline::Topology topology = QRhiGraphicsPipeline::Triangles;
    switch (geomDrawMode) {
    case QSGGeometry::DrawPoints:
        topology = QRhiGraphicsPipeline::Points;
        break;
    case QSGGeometry::DrawLines:
        topology = QRhiGraphicsPipeline::Lines;
        break;
    case QSGGeometry::DrawLineStrip:
        topology = QRhiGraphicsPipeline::LineStrip;
        break;
    case QSGGeometry::DrawTriangles:
        topology = QRhiGraphicsPipeline::Triangles;
        break;
    case QSGGeometry::DrawTriangleStrip:
        topology = QRhiGraphicsPipeline::TriangleStrip;
        break;
    default:
        qWarning("Primitive topology 0x%x not supported", geomDrawMode);
        break;
    }
    return topology;
}

ShaderManager::Shader *ShaderManager::prepareMaterial(QSGMaterial *material, bool enableRhiShaders, const QSGGeometry *geometry)
{
    QSGMaterialType *type = material->type();
    Shader *shader = rewrittenShaders.value(type, 0);
    if (shader)
        return shader;

    if (enableRhiShaders && !material->flags().testFlag(QSGMaterial::SupportsRhiShader)) {
        qWarning("The material failed to provide a working QShader pack");
        return nullptr;
    }

    Q_TRACE_SCOPE(QSG_prepareMaterial);
    if (QSG_LOG_TIME_COMPILATION().isDebugEnabled())
        qsg_renderer_timer.start();
    Q_QUICK_SG_PROFILE_START(QQuickProfiler::SceneGraphContextFrame);

    shader = new Shader;
    if (enableRhiShaders) {
        material->setFlag(QSGMaterial::RhiShaderWanted, true);
        QSGMaterialRhiShader *s = static_cast<QSGMaterialRhiShader *>(material->createShader());
        material->setFlag(QSGMaterial::RhiShaderWanted, false);
        context->initializeRhiShader(s, QShader::BatchableVertexShader);
        shader->programRhi.program = s;
        shader->programRhi.inputLayout = calculateVertexInputLayout(s, geometry, true);
        QSGMaterialRhiShaderPrivate *sD = QSGMaterialRhiShaderPrivate::get(s);
        shader->programRhi.shaderStages = {
            { QRhiGraphicsShaderStage::Vertex, sD->shader(QShader::VertexStage), QShader::BatchableVertexShader },
            { QRhiGraphicsShaderStage::Fragment, sD->shader(QShader::FragmentStage) }
        };
    } else {
        QSGMaterialShader *s = material->createShader();
        QOpenGLContext *ctx = context->openglContext();
        QSurfaceFormat::OpenGLContextProfile profile = ctx->format().profile();
        QOpenGLShaderProgram *p = s->program();
        char const *const *attr = s->attributeNames();
        int i;
        for (i = 0; attr[i]; ++i) {
            if (*attr[i])
                p->bindAttributeLocation(attr[i], i);
        }
        p->bindAttributeLocation("_qt_order", i);
        context->compileShader(s, material, qsgShaderRewriter_insertZAttributes(s->vertexShader(), profile), nullptr);
        context->initializeShader(s);
        if (!p->isLinked()) {
            delete shader;
            return nullptr;
        }
        shader->programGL.program = s;
        shader->programGL.pos_order = i;
    }

    shader->lastOpacity = 0;

    qCDebug(QSG_LOG_TIME_COMPILATION, "material shaders prepared in %dms", (int) qsg_renderer_timer.elapsed());

    Q_QUICK_SG_PROFILE_END(QQuickProfiler::SceneGraphContextFrame,
                           QQuickProfiler::SceneGraphContextMaterialCompile);

    rewrittenShaders[type] = shader;
    return shader;
}

ShaderManager::Shader *ShaderManager::prepareMaterialNoRewrite(QSGMaterial *material, bool enableRhiShaders, const QSGGeometry *geometry)
{
    QSGMaterialType *type = material->type();
    Shader *shader = stockShaders.value(type, 0);
    if (shader)
        return shader;

    if (enableRhiShaders && !material->flags().testFlag(QSGMaterial::SupportsRhiShader)) {
        qWarning("The material failed to provide a working QShader pack");
        return nullptr;
    }

    Q_TRACE_SCOPE(QSG_prepareMaterial);
    if (QSG_LOG_TIME_COMPILATION().isDebugEnabled())
        qsg_renderer_timer.start();
    Q_QUICK_SG_PROFILE_START(QQuickProfiler::SceneGraphContextFrame);

    shader = new Shader;
    if (enableRhiShaders) {
        material->setFlag(QSGMaterial::RhiShaderWanted, true);
        QSGMaterialRhiShader *s = static_cast<QSGMaterialRhiShader *>(material->createShader());
        material->setFlag(QSGMaterial::RhiShaderWanted, false);
        context->initializeRhiShader(s, QShader::StandardShader);
        shader->programRhi.program = s;
        shader->programRhi.inputLayout = calculateVertexInputLayout(s, geometry, false);
        QSGMaterialRhiShaderPrivate *sD = QSGMaterialRhiShaderPrivate::get(s);
        shader->programRhi.shaderStages = {
            { QRhiGraphicsShaderStage::Vertex, sD->shader(QShader::VertexStage) },
            { QRhiGraphicsShaderStage::Fragment, sD->shader(QShader::FragmentStage) }
        };
    } else {
        QSGMaterialShader *s = material->createShader();
        context->compileShader(s, material);
        context->initializeShader(s);
        shader->programGL.program = s;
        shader->programGL.pos_order = -1;
    }

    shader->lastOpacity = 0;

    stockShaders[type] = shader;

    qCDebug(QSG_LOG_TIME_COMPILATION, "shader compiled in %dms (no rewrite)", (int) qsg_renderer_timer.elapsed());

    Q_QUICK_SG_PROFILE_END(QQuickProfiler::SceneGraphContextFrame,
                           QQuickProfiler::SceneGraphContextMaterialCompile);
    return shader;
}

void ShaderManager::invalidated()
{
    qDeleteAll(stockShaders);
    stockShaders.clear();
    qDeleteAll(rewrittenShaders);
    rewrittenShaders.clear();
    delete blitProgram;
    blitProgram = nullptr;

    qDeleteAll(srbCache);
    srbCache.clear();

    qDeleteAll(pipelineCache);
    pipelineCache.clear();
}

void ShaderManager::clearCachedRendererData()
{
    for (ShaderManager::Shader *sms : stockShaders) {
        QSGMaterialRhiShader *s = sms->programRhi.program;
        if (s) {
            QSGMaterialRhiShaderPrivate *sd = QSGMaterialRhiShaderPrivate::get(s);
            sd->clearCachedRendererData();
        }
    }
    for (ShaderManager::Shader *sms : rewrittenShaders) {
        QSGMaterialRhiShader *s = sms->programRhi.program;
        if (s) {
            QSGMaterialRhiShaderPrivate *sd = QSGMaterialRhiShaderPrivate::get(s);
            sd->clearCachedRendererData();
        }
    }
}

QRhiShaderResourceBindings *ShaderManager::srb(const ShaderResourceBindingList &bindings)
{
    auto it = srbCache.constFind(bindings);
    if (it != srbCache.constEnd())
        return *it;

    QRhiShaderResourceBindings *srb = context->rhi()->newShaderResourceBindings();
    srb->setBindings(bindings.cbegin(), bindings.cend());
    if (srb->build()) {
        srbCache.insert(bindings, srb);
    } else {
        qWarning("Failed to build srb");
        delete srb;
        srb = nullptr;
    }
    return srb;
}

void qsg_dumpShadowRoots(BatchRootInfo *i, int indent)
{
    static int extraIndent = 0;
    ++extraIndent;

    QByteArray ind(indent + extraIndent + 10, ' ');

    if (!i) {
        qDebug("%s - no info", ind.constData());
    } else {
        qDebug() << ind.constData() << "- parent:" << i->parentRoot << "orders" << i->firstOrder << "->" << i->lastOrder << ", avail:" << i->availableOrders;
        for (QSet<Node *>::const_iterator it = i->subRoots.constBegin();
             it != i->subRoots.constEnd(); ++it) {
            qDebug() << ind.constData() << "-" << *it;
            qsg_dumpShadowRoots((*it)->rootInfo(), indent);
        }
    }

    --extraIndent;
}

void qsg_dumpShadowRoots(Node *n)
{
#ifndef QT_NO_DEBUG_OUTPUT
    static int indent = 0;
    ++indent;

    QByteArray ind(indent, ' ');

    if (n->type() == QSGNode::ClipNodeType || n->isBatchRoot) {
        qDebug() << ind.constData() << "[X]" << n->sgNode << Qt::hex << uint(n->sgNode->flags());
        qsg_dumpShadowRoots(n->rootInfo(), indent);
    } else {
        QDebug d = qDebug();
        d << ind.constData() << "[ ]" << n->sgNode << Qt::hex << uint(n->sgNode->flags());
        if (n->type() == QSGNode::GeometryNodeType)
            d << "order" << Qt::dec << n->element()->order;
    }

    SHADOWNODE_TRAVERSE(n)
            qsg_dumpShadowRoots(child);

    --indent;
#else
    Q_UNUSED(n)
#endif
}

Updater::Updater(Renderer *r)
    : renderer(r)
    , m_roots(32)
    , m_rootMatrices(8)
{
    m_roots.add(0);
    m_combined_matrix_stack.add(&m_identityMatrix);
    m_rootMatrices.add(m_identityMatrix);

    Q_ASSERT(sizeof(QMatrix4x4_Accessor) == sizeof(QMatrix4x4));
}

void Updater::updateStates(QSGNode *n)
{
    m_current_clip = nullptr;

    m_added = 0;
    m_transformChange = 0;
    m_opacityChange = 0;

    Node *sn = renderer->m_nodes.value(n, 0);
    Q_ASSERT(sn);

    if (Q_UNLIKELY(debug_roots()))
        qsg_dumpShadowRoots(sn);

    if (Q_UNLIKELY(debug_build())) {
        qDebug("Updater::updateStates()");
        if (sn->dirtyState & (QSGNode::DirtyNodeAdded << 16))
            qDebug(" - nodes have been added");
        if (sn->dirtyState & (QSGNode::DirtyMatrix << 16))
            qDebug(" - transforms have changed");
        if (sn->dirtyState & (QSGNode::DirtyOpacity << 16))
            qDebug(" - opacity has changed");
        if (uint(sn->dirtyState) & uint(QSGNode::DirtyForceUpdate << 16))
            qDebug(" - forceupdate");
    }

    if (Q_UNLIKELY(renderer->m_visualizer->mode() == Visualizer::VisualizeChanges))
        renderer->m_visualizer->visualizeChangesPrepare(sn);

    visitNode(sn);
}

void Updater::visitNode(Node *n)
{
    if (m_added == 0 && n->dirtyState == 0 && m_force_update == 0 && m_transformChange == 0 && m_opacityChange == 0)
        return;

    int count = m_added;
    if (n->dirtyState & QSGNode::DirtyNodeAdded)
        ++m_added;

    int force = m_force_update;
    if (n->dirtyState & QSGNode::DirtyForceUpdate)
        ++m_force_update;

    switch (n->type()) {
    case QSGNode::OpacityNodeType:
        visitOpacityNode(n);
        break;
    case QSGNode::TransformNodeType:
        visitTransformNode(n);
        break;
    case QSGNode::GeometryNodeType:
        visitGeometryNode(n);
        break;
    case QSGNode::ClipNodeType:
        visitClipNode(n);
        break;
    case QSGNode::RenderNodeType:
        if (m_added)
            n->renderNodeElement()->root = m_roots.last();
        Q_FALLTHROUGH();    // to visit children
    default:
        SHADOWNODE_TRAVERSE(n) visitNode(child);
        break;
    }

    m_added = count;
    m_force_update = force;
    n->dirtyState = {};
}

void Updater::visitClipNode(Node *n)
{
    ClipBatchRootInfo *extra = n->clipInfo();

    QSGClipNode *cn = static_cast<QSGClipNode *>(n->sgNode);

    if (m_roots.last() && m_added > 0)
        renderer->registerBatchRoot(n, m_roots.last());

    cn->setRendererClipList(m_current_clip);
    m_current_clip = cn;
    m_roots << n;
    m_rootMatrices.add(m_rootMatrices.last() * *m_combined_matrix_stack.last());
    extra->matrix = m_rootMatrices.last();
    cn->setRendererMatrix(&extra->matrix);
    m_combined_matrix_stack << &m_identityMatrix;

    SHADOWNODE_TRAVERSE(n) visitNode(child);

    m_current_clip = cn->clipList();
    m_rootMatrices.pop_back();
    m_combined_matrix_stack.pop_back();
    m_roots.pop_back();
}

void Updater::visitOpacityNode(Node *n)
{
    QSGOpacityNode *on = static_cast<QSGOpacityNode *>(n->sgNode);

    qreal combined = m_opacity_stack.last() * on->opacity();
    on->setCombinedOpacity(combined);
    m_opacity_stack.add(combined);

    if (m_added == 0 && n->dirtyState & QSGNode::DirtyOpacity) {
        bool was = n->isOpaque;
        bool is = on->opacity() > OPAQUE_LIMIT;
        if (was != is) {
            renderer->m_rebuild = Renderer::FullRebuild;
            n->isOpaque = is;
        }
        ++m_opacityChange;
        SHADOWNODE_TRAVERSE(n) visitNode(child);
        --m_opacityChange;
    } else {
        if (m_added > 0)
            n->isOpaque = on->opacity() > OPAQUE_LIMIT;
        SHADOWNODE_TRAVERSE(n) visitNode(child);
    }

    m_opacity_stack.pop_back();
}

void Updater::visitTransformNode(Node *n)
{
    bool popMatrixStack = false;
    bool popRootStack = false;
    bool dirty = n->dirtyState & QSGNode::DirtyMatrix;

    QSGTransformNode *tn = static_cast<QSGTransformNode *>(n->sgNode);

    if (n->isBatchRoot) {
        if (m_added > 0 && m_roots.last())
            renderer->registerBatchRoot(n, m_roots.last());
        tn->setCombinedMatrix(m_rootMatrices.last() * *m_combined_matrix_stack.last() * tn->matrix());

        // The only change in this subtree is ourselves and we are a batch root, so
        // only update subroots and return, saving tons of child-processing (flickable-panning)

        if (!n->becameBatchRoot && m_added == 0 && m_force_update == 0 && m_opacityChange == 0 && dirty && (n->dirtyState & ~QSGNode::DirtyMatrix) == 0) {
            BatchRootInfo *info = renderer->batchRootInfo(n);
            for (QSet<Node *>::const_iterator it = info->subRoots.constBegin();
                 it != info->subRoots.constEnd(); ++it) {
                updateRootTransforms(*it, n, tn->combinedMatrix());
            }
            return;
        }

        n->becameBatchRoot = false;

        m_combined_matrix_stack.add(&m_identityMatrix);
        m_roots.add(n);
        m_rootMatrices.add(tn->combinedMatrix());

        popMatrixStack = true;
        popRootStack = true;
    } else if (!tn->matrix().isIdentity()) {
        tn->setCombinedMatrix(*m_combined_matrix_stack.last() * tn->matrix());
        m_combined_matrix_stack.add(&tn->combinedMatrix());
        popMatrixStack = true;
    } else {
        tn->setCombinedMatrix(*m_combined_matrix_stack.last());
    }

    if (dirty)
        ++m_transformChange;

    SHADOWNODE_TRAVERSE(n) visitNode(child);

    if (dirty)
        --m_transformChange;
    if (popMatrixStack)
        m_combined_matrix_stack.pop_back();
    if (popRootStack) {
        m_roots.pop_back();
        m_rootMatrices.pop_back();
    }
}

void Updater::visitGeometryNode(Node *n)
{
    QSGGeometryNode *gn = static_cast<QSGGeometryNode *>(n->sgNode);

    gn->setRendererMatrix(m_combined_matrix_stack.last());
    gn->setRendererClipList(m_current_clip);
    gn->setInheritedOpacity(m_opacity_stack.last());

    if (m_added) {
        Element *e = n->element();
        e->root = m_roots.last();
        e->translateOnlyToRoot = QMatrix4x4_Accessor::isTranslate(*gn->matrix());

        if (e->root) {
            BatchRootInfo *info = renderer->batchRootInfo(e->root);
            while (info != nullptr) {
                info->availableOrders--;
                if (info->availableOrders < 0) {
                    renderer->m_rebuild |= Renderer::BuildRenderLists;
                } else {
                    renderer->m_rebuild |= Renderer::BuildRenderListsForTaggedRoots;
                    renderer->m_taggedRoots << e->root;
                }
                if (info->parentRoot != nullptr)
                    info = renderer->batchRootInfo(info->parentRoot);
                else
                    info = nullptr;
            }
        } else {
            renderer->m_rebuild |= Renderer::FullRebuild;
        }
    } else {
        if (m_transformChange) {
            Element *e = n->element();
            e->translateOnlyToRoot = QMatrix4x4_Accessor::isTranslate(*gn->matrix());
        }
        if (m_opacityChange) {
            Element *e = n->element();
            if (e->batch)
                renderer->invalidateBatchAndOverlappingRenderOrders(e->batch);
        }
    }

    SHADOWNODE_TRAVERSE(n) visitNode(child);
}

void Updater::updateRootTransforms(Node *node, Node *root, const QMatrix4x4 &combined)
{
    BatchRootInfo *info = renderer->batchRootInfo(node);
    QMatrix4x4 m;
    Node *n = node;

    while (n != root) {
        if (n->type() == QSGNode::TransformNodeType)
            m = static_cast<QSGTransformNode *>(n->sgNode)->matrix() * m;
        n = n->parent();
    }

    m = combined * m;

    if (node->type() == QSGNode::ClipNodeType) {
        static_cast<ClipBatchRootInfo *>(info)->matrix = m;
    } else {
        Q_ASSERT(node->type() == QSGNode::TransformNodeType);
        static_cast<QSGTransformNode *>(node->sgNode)->setCombinedMatrix(m);
    }

    for (QSet<Node *>::const_iterator it = info->subRoots.constBegin();
         it != info->subRoots.constEnd(); ++it) {
        updateRootTransforms(*it, node, m);
    }
}

int qsg_positionAttribute(QSGGeometry *g)
{
    int vaOffset = 0;
    for (int a=0; a<g->attributeCount(); ++a) {
        const QSGGeometry::Attribute &attr = g->attributes()[a];
        if (attr.isVertexCoordinate && attr.tupleSize == 2 && attr.type == QSGGeometry::FloatType) {
            return vaOffset;
        }
        vaOffset += attr.tupleSize * size_of_type(attr.type);
    }
    return -1;
}


void Rect::map(const QMatrix4x4 &matrix)
{
    const float *m = matrix.constData();
    if (QMatrix4x4_Accessor::isScale(matrix)) {
        tl.x = tl.x * m[0] + m[12];
        tl.y = tl.y * m[5] + m[13];
        br.x = br.x * m[0] + m[12];
        br.y = br.y * m[5] + m[13];
        if (tl.x > br.x)
            qSwap(tl.x, br.x);
        if (tl.y > br.y)
            qSwap(tl.y, br.y);
    } else {
        Pt mtl = tl;
        Pt mtr = { br.x, tl.y };
        Pt mbl = { tl.x, br.y };
        Pt mbr = br;

        mtl.map(matrix);
        mtr.map(matrix);
        mbl.map(matrix);
        mbr.map(matrix);

        set(FLT_MAX, FLT_MAX, -FLT_MAX, -FLT_MAX);
        (*this) |= mtl;
        (*this) |= mtr;
        (*this) |= mbl;
        (*this) |= mbr;
    }
}

void Element::computeBounds()
{
    Q_ASSERT(!boundsComputed);
    boundsComputed = true;

    QSGGeometry *g = node->geometry();
    int offset = qsg_positionAttribute(g);
    if (offset == -1) {
        // No position attribute means overlaps with everything..
        bounds.set(-FLT_MAX, -FLT_MAX, FLT_MAX, FLT_MAX);
        return;
    }

    bounds.set(FLT_MAX, FLT_MAX, -FLT_MAX, -FLT_MAX);
    char *vd = (char *) g->vertexData() + offset;
    for (int i=0; i<g->vertexCount(); ++i) {
        bounds |= *(Pt *) vd;
        vd += g->sizeOfVertex();
    }
    bounds.map(*node->matrix());

    if (!qt_is_finite(bounds.tl.x) || bounds.tl.x == FLT_MAX)
        bounds.tl.x = -FLT_MAX;
    if (!qt_is_finite(bounds.tl.y) || bounds.tl.y == FLT_MAX)
        bounds.tl.y = -FLT_MAX;
    if (!qt_is_finite(bounds.br.x) || bounds.br.x == -FLT_MAX)
        bounds.br.x = FLT_MAX;
    if (!qt_is_finite(bounds.br.y) || bounds.br.y == -FLT_MAX)
        bounds.br.y = FLT_MAX;

    Q_ASSERT(bounds.tl.x <= bounds.br.x);
    Q_ASSERT(bounds.tl.y <= bounds.br.y);

    boundsOutsideFloatRange = bounds.isOutsideFloatRange();
}

BatchCompatibility Batch::isMaterialCompatible(Element *e) const
{
    Element *n = first;
    // Skip to the first node other than e which has not been removed
    while (n && (n == e || n->removed))
        n = n->nextInBatch;

    // Only 'e' in this batch, so a material change doesn't change anything as long as
    // its blending is still in sync with this batch...
    if (!n)
        return BatchIsCompatible;

    QSGMaterial *m = e->node->activeMaterial();
    QSGMaterial *nm = n->node->activeMaterial();
    return (nm->type() == m->type() && nm->compare(m) == 0)
            ? BatchIsCompatible
            : BatchBreaksOnCompare;
}

/*
 * Marks this batch as dirty or in the case where the geometry node has
 * changed to be incompatible with this batch, return false so that
 * the caller can mark the entire sg for a full rebuild...
 */
bool Batch::geometryWasChanged(QSGGeometryNode *gn)
{
    Element *e = first;
    Q_ASSERT_X(e, "Batch::geometryWasChanged", "Batch is expected to 'valid' at this time");
    // 'gn' is the first node in the batch, compare against the next one.
    while (e && (e->node == gn || e->removed))
        e = e->nextInBatch;
    if (!e || e->node->geometry()->attributes() == gn->geometry()->attributes()) {
        needsUpload = true;
        return true;
    } else {
        return false;
    }
}

void Batch::cleanupRemovedElements()
{
    if (!needsPurge)
        return;

    // remove from front of batch..
    while (first && first->removed) {
        first = first->nextInBatch;
    }

    // Then continue and remove other nodes further out in the batch..
    if (first) {
        Element *e = first;
        while (e->nextInBatch) {
            if (e->nextInBatch->removed)
                e->nextInBatch = e->nextInBatch->nextInBatch;
            else
                e = e->nextInBatch;

        }
    }

    needsPurge = false;
}

/*
 * Iterates through all geometry nodes in this batch and unsets their batch,
 * thus forcing them to be rebuilt
 */
void Batch::invalidate()
{
    // If doing removal here is a performance issue, we might add a "hasRemoved" bit to
    // the batch to do an early out..
    cleanupRemovedElements();
    Element *e = first;
    first = nullptr;
    root = nullptr;
    while (e) {
        e->batch = nullptr;
        Element *n = e->nextInBatch;
        e->nextInBatch = nullptr;
        e = n;
    }
}

bool Batch::isTranslateOnlyToRoot() const {
    bool only = true;
    Element *e = first;
    while (e && only) {
        only &= e->translateOnlyToRoot;
        e = e->nextInBatch;
    }
    return only;
}

/*
 * Iterates through all the nodes in the batch and returns true if the
 * nodes are all safe to batch. There are two separate criteria:
 *
 * - The matrix is such that the z component of the result is of no
 *   consequence.
 *
 * - The bounds are inside the stable floating point range. This applies
 *   to desktop only where we in this case can trigger a fallback to
 *   unmerged in which case we pass the geometry straight through and
 *   just apply the matrix.
 *
 *   NOTE: This also means a slight performance impact for geometries which
 *   are defined to be outside the stable floating point range and still
 *   use single precision float, but given that this implicitly fixes
 *   huge lists and tables, it is worth it.
 */
bool Batch::isSafeToBatch() const {
    Element *e = first;
    while (e) {
        if (e->boundsOutsideFloatRange)
            return false;
        if (!QMatrix4x4_Accessor::is2DSafe(*e->node->matrix()))
            return false;
        e = e->nextInBatch;
    }
    return true;
}

static int qsg_countNodesInBatch(const Batch *batch)
{
    int sum = 0;
    Element *e = batch->first;
    while (e) {
        ++sum;
        e = e->nextInBatch;
    }
    return sum;
}

static int qsg_countNodesInBatches(const QDataBuffer<Batch *> &batches)
{
    int sum = 0;
    for (int i=0; i<batches.size(); ++i) {
        sum += qsg_countNodesInBatch(batches.at(i));
    }
    return sum;
}

Renderer::Renderer(QSGDefaultRenderContext *ctx)
    : QSGRenderer(ctx)
    , m_context(ctx)
    , m_opaqueRenderList(64)
    , m_alphaRenderList(64)
    , m_nextRenderOrder(0)
    , m_partialRebuild(false)
    , m_partialRebuildRoot(nullptr)
    , m_useDepthBuffer(true)
    , m_opaqueBatches(16)
    , m_alphaBatches(16)
    , m_batchPool(16)
    , m_elementsToDelete(64)
    , m_tmpAlphaElements(16)
    , m_tmpOpaqueElements(16)
    , m_rebuild(FullRebuild)
    , m_zRange(0)
    , m_renderOrderRebuildLower(-1)
    , m_renderOrderRebuildUpper(-1)
    , m_currentMaterial(nullptr)
    , m_currentShader(nullptr)
    , m_currentStencilValue(0)
    , m_clipMatrixId(0)
    , m_currentClip(nullptr)
    , m_currentClipType(ClipState::NoClip)
    , m_vertexUploadPool(256)
    , m_indexUploadPool(64)
    , m_vao(nullptr)
{
    m_rhi = m_context->rhi();
    if (m_rhi) {
        m_ubufAlignment = m_rhi->ubufAlignment();
        m_uint32IndexForRhi = !m_rhi->isFeatureSupported(QRhi::NonFourAlignedEffectiveIndexBufferOffset);
        if (qEnvironmentVariableIntValue("QSG_RHI_UINT32_INDEX"))
            m_uint32IndexForRhi = true;
        m_visualizer = new RhiVisualizer(this);
    } else {
        initializeOpenGLFunctions();
        m_uint32IndexForRhi = false;
        m_visualizer = new OpenGLVisualizer(this);
    }

    setNodeUpdater(new Updater(this));

    // The shader manager is shared between renderers (think for example Item
    // layers that create a new Renderer each) with the same rendercontext
    // (i.e. QRhi or QOpenGLContext).
    m_shaderManager = ctx->findChild<ShaderManager *>(QStringLiteral("__qt_ShaderManager"), Qt::FindDirectChildrenOnly);
    if (!m_shaderManager) {
        m_shaderManager = new ShaderManager(ctx);
        m_shaderManager->setObjectName(QStringLiteral("__qt_ShaderManager"));
        m_shaderManager->setParent(ctx);
        QObject::connect(ctx, SIGNAL(invalidated()), m_shaderManager, SLOT(invalidated()), Qt::DirectConnection);
    }

    m_bufferStrategy = GL_STATIC_DRAW;
    if (Q_UNLIKELY(qEnvironmentVariableIsSet("QSG_RENDERER_BUFFER_STRATEGY"))) {
        const QByteArray strategy = qgetenv("QSG_RENDERER_BUFFER_STRATEGY");
        if (strategy == "dynamic")
            m_bufferStrategy = GL_DYNAMIC_DRAW;
        else if (strategy == "stream")
            m_bufferStrategy = GL_STREAM_DRAW;
    }

    m_batchNodeThreshold = qt_sg_envInt("QSG_RENDERER_BATCH_NODE_THRESHOLD", 64);
    m_batchVertexThreshold = qt_sg_envInt("QSG_RENDERER_BATCH_VERTEX_THRESHOLD", 1024);

    if (Q_UNLIKELY(debug_build() || debug_render())) {
        qDebug("Batch thresholds: nodes: %d vertices: %d",
               m_batchNodeThreshold, m_batchVertexThreshold);
        qDebug("Using buffer strategy: %s",
               (m_bufferStrategy == GL_STATIC_DRAW
                ? "static" : (m_bufferStrategy == GL_DYNAMIC_DRAW ? "dynamic" : "stream")));
    }

    static const bool useDepth = qEnvironmentVariableIsEmpty("QSG_NO_DEPTH_BUFFER");
    if (!m_rhi) {
        // If rendering with an OpenGL Core profile context, we need to create a VAO
        // to hold our vertex specification state.
        if (m_context->openglContext()->format().profile() == QSurfaceFormat::CoreProfile) {
            m_vao = new QOpenGLVertexArrayObject(this);
            m_vao->create();
        }
        m_useDepthBuffer = useDepth && ctx->openglContext()->format().depthBufferSize() > 0;
    } else {
        m_useDepthBuffer = useDepth;
    }
}

static void qsg_wipeBuffer(Buffer *buffer, QOpenGLFunctions *funcs)
{
    if (buffer->buf) {
        //qDebug("releasing rhibuf %p", buffer->buf);
        delete buffer->buf;
    }

    if (buffer->id)
        funcs->glDeleteBuffers(1, &buffer->id);

    // The free here is ok because we're in one of two situations.
    // 1. We're using the upload pool in which case unmap will have set the
    //    data pointer to 0 and calling free on 0 is ok.
    // 2. We're using dedicated buffers because of visualization or IBO workaround
    //    and the data something we malloced and must be freed.
    free(buffer->data);
}

static void qsg_wipeBatch(Batch *batch, QOpenGLFunctions *funcs, bool separateIndexBuffer)
{
    qsg_wipeBuffer(&batch->vbo, funcs);
    if (separateIndexBuffer)
        qsg_wipeBuffer(&batch->ibo, funcs);
    delete batch->ubuf;
    batch->stencilClipState.reset();
    delete batch;
}

Renderer::~Renderer()
{
    if (m_rhi || QOpenGLContext::currentContext()) {
        // Clean up batches and buffers
        const bool separateIndexBuffer = m_context->separateIndexBuffer();
        for (int i = 0; i < m_opaqueBatches.size(); ++i)
            qsg_wipeBatch(m_opaqueBatches.at(i), this, separateIndexBuffer);
        for (int i = 0; i < m_alphaBatches.size(); ++i)
            qsg_wipeBatch(m_alphaBatches.at(i), this, separateIndexBuffer);
        for (int i = 0; i < m_batchPool.size(); ++i)
            qsg_wipeBatch(m_batchPool.at(i), this, separateIndexBuffer);
    }

    for (Node *n : qAsConst(m_nodes))
        m_nodeAllocator.release(n);

    // Remaining elements...
    for (int i=0; i<m_elementsToDelete.size(); ++i) {
        Element *e = m_elementsToDelete.at(i);
        if (e->isRenderNode)
            delete static_cast<RenderNodeElement *>(e);
        else
            m_elementAllocator.release(e);
    }

    destroyGraphicsResources();

    delete m_visualizer;
}

void Renderer::destroyGraphicsResources()
{
    // If this is from the dtor, then the shader manager and its already
    // prepared shaders will stay around for other renderers -> the cached data
    // in the rhi shaders have to be purged as it may refer to samplers we
    // are going to destroy.
    m_shaderManager->clearCachedRendererData();

    qDeleteAll(m_samplers);
    m_stencilClipCommon.reset();
    delete m_dummyTexture;
    m_visualizer->releaseResources();
}

void Renderer::releaseCachedResources()
{
    m_shaderManager->invalidated();

    destroyGraphicsResources();

    m_samplers.clear();
    m_dummyTexture = nullptr;

    if (m_rhi)
        m_rhi->releaseCachedResources();
}

void Renderer::invalidateAndRecycleBatch(Batch *b)
{
    b->invalidate();
    for (int i=0; i<m_batchPool.size(); ++i)
        if (b == m_batchPool.at(i))
            return;
    m_batchPool.add(b);
}

/* The code here does a CPU-side allocation which might seem like a performance issue
 * compared to using glMapBuffer or glMapBufferRange which would give me back
 * potentially GPU allocated memory and saving me one deep-copy, but...
 *
 * Because we do a lot of CPU-side transformations, we need random-access memory
 * and the memory returned from glMapBuffer/glMapBufferRange is typically
 * uncached and thus very slow for our purposes.
 *
 * ref: http://www.opengl.org/wiki/Buffer_Object
 */
void Renderer::map(Buffer *buffer, int byteSize, bool isIndexBuf)
{
    if (!m_context->hasBrokenIndexBufferObjects() && m_visualizer->mode() == Visualizer::VisualizeNothing) {
        // Common case, use a shared memory pool for uploading vertex data to avoid
        // excessive reevaluation
        QDataBuffer<char> &pool = m_context->separateIndexBuffer() && isIndexBuf
                ? m_indexUploadPool : m_vertexUploadPool;
        if (byteSize > pool.size())
            pool.resize(byteSize);
        buffer->data = pool.data();
    } else if (buffer->size != byteSize) {
        free(buffer->data);
        buffer->data = (char *) malloc(byteSize);
        Q_CHECK_PTR(buffer->data);
    }
    buffer->size = byteSize;
}

void Renderer::unmap(Buffer *buffer, bool isIndexBuf)
{
    if (m_rhi) {
        // Batches are pooled and reused which means the QRhiBuffer will be
        // still valid in a recycled Batch. We only hit the newBuffer() path
        // for brand new Batches.
        if (!buffer->buf) {
            buffer->buf = m_rhi->newBuffer(QRhiBuffer::Immutable,
                                           isIndexBuf ? QRhiBuffer::IndexBuffer : QRhiBuffer::VertexBuffer,
                                           buffer->size);
            if (!buffer->buf->build())
                qWarning("Failed to build vertex/index buffer of size %d", buffer->size);
//            else
//                qDebug("created rhibuf %p size %d", buffer->buf, buffer->size);
        } else {
            bool needsRebuild = false;
            if (buffer->buf->size() < buffer->size) {
                buffer->buf->setSize(buffer->size);
                needsRebuild = true;
            }
            if (buffer->buf->type() != QRhiBuffer::Dynamic
                    && buffer->nonDynamicChangeCount > DYNAMIC_VERTEX_INDEX_BUFFER_THRESHOLD)
            {
                buffer->buf->setType(QRhiBuffer::Dynamic);
                buffer->nonDynamicChangeCount = 0;
                needsRebuild = true;
            }
            if (needsRebuild) {
                //qDebug("rebuilding rhibuf %p size %d type Dynamic", buffer->buf, buffer->size);
                buffer->buf->build();
            }
        }
        if (buffer->buf->type() != QRhiBuffer::Dynamic) {
            m_resourceUpdates->uploadStaticBuffer(buffer->buf,
                                                  QByteArray::fromRawData(buffer->data, buffer->size));
            buffer->nonDynamicChangeCount += 1;
        } else {
            m_resourceUpdates->updateDynamicBuffer(buffer->buf, 0, buffer->size,
                                                   QByteArray::fromRawData(buffer->data, buffer->size));
        }
        if (m_visualizer->mode() == Visualizer::VisualizeNothing)
            buffer->data = nullptr;
    } else {
        if (buffer->id == 0)
            glGenBuffers(1, &buffer->id);
        GLenum target = isIndexBuf ? GL_ELEMENT_ARRAY_BUFFER : GL_ARRAY_BUFFER;
        glBindBuffer(target, buffer->id);
        glBufferData(target, buffer->size, buffer->data, m_bufferStrategy);
        if (!m_context->hasBrokenIndexBufferObjects() && m_visualizer->mode() == Visualizer::VisualizeNothing)
            buffer->data = nullptr;
    }
}

BatchRootInfo *Renderer::batchRootInfo(Node *node)
{
    BatchRootInfo *info = node->rootInfo();
    if (!info) {
        if (node->type() == QSGNode::ClipNodeType)
            info = new ClipBatchRootInfo;
        else {
            Q_ASSERT(node->type() == QSGNode::TransformNodeType);
            info = new BatchRootInfo;
        }
        node->data = info;
    }
    return info;
}

void Renderer::removeBatchRootFromParent(Node *childRoot)
{
    BatchRootInfo *childInfo = batchRootInfo(childRoot);
    if (!childInfo->parentRoot)
        return;
    BatchRootInfo *parentInfo = batchRootInfo(childInfo->parentRoot);

    Q_ASSERT(parentInfo->subRoots.contains(childRoot));
    parentInfo->subRoots.remove(childRoot);
    childInfo->parentRoot = nullptr;
}

void Renderer::registerBatchRoot(Node *subRoot, Node *parentRoot)
{
    BatchRootInfo *subInfo = batchRootInfo(subRoot);
    BatchRootInfo *parentInfo = batchRootInfo(parentRoot);
    subInfo->parentRoot = parentRoot;
    parentInfo->subRoots << subRoot;
}

bool Renderer::changeBatchRoot(Node *node, Node *root)
{
    BatchRootInfo *subInfo = batchRootInfo(node);
    if (subInfo->parentRoot == root)
        return false;
    if (subInfo->parentRoot) {
        BatchRootInfo *oldRootInfo = batchRootInfo(subInfo->parentRoot);
        oldRootInfo->subRoots.remove(node);
    }
    BatchRootInfo *newRootInfo = batchRootInfo(root);
    newRootInfo->subRoots << node;
    subInfo->parentRoot = root;
    return true;
}

void Renderer::nodeChangedBatchRoot(Node *node, Node *root)
{
    if (node->type() == QSGNode::ClipNodeType || node->isBatchRoot) {
        // When we reach a batchroot, we only need to update it. Its subtree
        // is relative to that root, so no need to recurse further.
        changeBatchRoot(node, root);
        return;
    } else if (node->type() == QSGNode::GeometryNodeType) {
        // Only need to change the root as nodeChanged anyway flags a full update.
        Element *e = node->element();
        if (e) {
            e->root = root;
            e->boundsComputed = false;
        }
    } else if (node->type() == QSGNode::RenderNodeType) {
        RenderNodeElement *e = node->renderNodeElement();
        if (e)
            e->root = root;
    }

    SHADOWNODE_TRAVERSE(node)
            nodeChangedBatchRoot(child, root);
}

void Renderer::nodeWasTransformed(Node *node, int *vertexCount)
{
    if (node->type() == QSGNode::GeometryNodeType) {
        QSGGeometryNode *gn = static_cast<QSGGeometryNode *>(node->sgNode);
        *vertexCount += gn->geometry()->vertexCount();
        Element *e  = node->element();
        if (e) {
            e->boundsComputed = false;
            if (e->batch) {
                if (!e->batch->isOpaque) {
                    invalidateBatchAndOverlappingRenderOrders(e->batch);
                } else if (e->batch->merged) {
                    e->batch->needsUpload = true;
                }
            }
        }
    }

    SHADOWNODE_TRAVERSE(node)
        nodeWasTransformed(child, vertexCount);
}

void Renderer::nodeWasAdded(QSGNode *node, Node *shadowParent)
{
    Q_ASSERT(!m_nodes.contains(node));
    if (node->isSubtreeBlocked())
        return;

    Node *snode = m_nodeAllocator.allocate();
    snode->sgNode = node;
    m_nodes.insert(node, snode);
    if (shadowParent)
        shadowParent->append(snode);

    if (node->type() == QSGNode::GeometryNodeType) {
        snode->data = m_elementAllocator.allocate();
        snode->element()->setNode(static_cast<QSGGeometryNode *>(node));

    } else if (node->type() == QSGNode::ClipNodeType) {
        snode->data = new ClipBatchRootInfo;
        m_rebuild |= FullRebuild;

    } else if (node->type() == QSGNode::RenderNodeType) {
        QSGRenderNode *rn = static_cast<QSGRenderNode *>(node);
        RenderNodeElement *e = new RenderNodeElement(rn);
        snode->data = e;
        Q_ASSERT(!m_renderNodeElements.contains(rn));
        m_renderNodeElements.insert(e->renderNode, e);
        if (!rn->flags().testFlag(QSGRenderNode::DepthAwareRendering))
            m_useDepthBuffer = false;
        m_rebuild |= FullRebuild;
    }

    QSGNODE_TRAVERSE(node)
            nodeWasAdded(child, snode);
}

void Renderer::nodeWasRemoved(Node *node)
{
    // Prefix traversal as removeBatchRootFromParent below removes nodes
    // in a bottom-up manner. Note that we *cannot* use SHADOWNODE_TRAVERSE
    // here, because we delete 'child' (when recursed, down below), so we'd
    // have a use-after-free.
    {
        Node *child = node->firstChild();
        while (child) {
            // Remove (and delete) child
            node->remove(child);
            nodeWasRemoved(child);
            child = node->firstChild();
        }
    }

    if (node->type() == QSGNode::GeometryNodeType) {
        Element *e = node->element();
        if (e) {
            e->removed = true;
            m_elementsToDelete.add(e);
            e->node = nullptr;
            if (e->root) {
                BatchRootInfo *info = batchRootInfo(e->root);
                info->availableOrders++;
            }
            if (e->batch) {
                e->batch->needsUpload = true;
                e->batch->needsPurge = true;
            }

        }

    } else if (node->type() == QSGNode::ClipNodeType) {
        removeBatchRootFromParent(node);
        delete node->clipInfo();
        m_rebuild |= FullRebuild;
        m_taggedRoots.remove(node);

    } else if (node->isBatchRoot) {
        removeBatchRootFromParent(node);
        delete node->rootInfo();
        m_rebuild |= FullRebuild;
        m_taggedRoots.remove(node);

    } else if (node->type() == QSGNode::RenderNodeType) {
        RenderNodeElement *e = m_renderNodeElements.take(static_cast<QSGRenderNode *>(node->sgNode));
        if (e) {
            e->removed = true;
            m_elementsToDelete.add(e);
            if (m_renderNodeElements.isEmpty()) {
                static const bool useDepth = qEnvironmentVariableIsEmpty("QSG_NO_DEPTH_BUFFER");
                if (m_rhi)
                    m_useDepthBuffer = useDepth;
                else
                    m_useDepthBuffer = useDepth && m_context->openglContext()->format().depthBufferSize() > 0;
            }

            if (e->batch != nullptr)
                e->batch->needsPurge = true;
        }
    }

    Q_ASSERT(m_nodes.contains(node->sgNode));

    m_nodeAllocator.release(m_nodes.take(node->sgNode));
}

void Renderer::turnNodeIntoBatchRoot(Node *node)
{
    if (Q_UNLIKELY(debug_change())) qDebug(" - new batch root");
    m_rebuild |= FullRebuild;
    node->isBatchRoot = true;
    node->becameBatchRoot = true;

    Node *p = node->parent();
    while (p) {
        if (p->type() == QSGNode::ClipNodeType || p->isBatchRoot) {
            registerBatchRoot(node, p);
            break;
        }
        p = p->parent();
    }

    SHADOWNODE_TRAVERSE(node)
            nodeChangedBatchRoot(child, node);
}


void Renderer::nodeChanged(QSGNode *node, QSGNode::DirtyState state)
{
#ifndef QT_NO_DEBUG_OUTPUT
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
#endif
    // As this function calls nodeChanged recursively, we do it at the top
    // to avoid that any of the others are processed twice.
    if (state & QSGNode::DirtySubtreeBlocked) {
        Node *sn = m_nodes.value(node);

        // Force a batch rebuild if this includes an opacity change
        if (state & QSGNode::DirtyOpacity)
            m_rebuild |= FullRebuild;

        bool blocked = node->isSubtreeBlocked();
        if (blocked && sn) {
            nodeChanged(node, QSGNode::DirtyNodeRemoved);
            Q_ASSERT(m_nodes.value(node) == 0);
        } else if (!blocked && !sn) {
            nodeChanged(node, QSGNode::DirtyNodeAdded);
        }
        return;
    }

    if (state & QSGNode::DirtyNodeAdded) {
        if (nodeUpdater()->isNodeBlocked(node, rootNode())) {
            QSGRenderer::nodeChanged(node, state);
            return;
        }
        if (node == rootNode())
            nodeWasAdded(node, nullptr);
        else
            nodeWasAdded(node, m_nodes.value(node->parent()));
    }

    // Mark this node dirty in the shadow tree.
    Node *shadowNode = m_nodes.value(node);

    // Blocked subtrees won't have shadow nodes, so we can safely abort
    // here..
    if (!shadowNode) {
        QSGRenderer::nodeChanged(node, state);
        return;
    }

    shadowNode->dirtyState |= state;

    if (state & QSGNode::DirtyMatrix && !shadowNode->isBatchRoot) {
        Q_ASSERT(node->type() == QSGNode::TransformNodeType);
        if (node->m_subtreeRenderableCount > m_batchNodeThreshold) {
            turnNodeIntoBatchRoot(shadowNode);
        } else {
            int vertices = 0;
            nodeWasTransformed(shadowNode, &vertices);
            if (vertices > m_batchVertexThreshold) {
                turnNodeIntoBatchRoot(shadowNode);
            }
        }
    }

    if (state & QSGNode::DirtyGeometry && node->type() == QSGNode::GeometryNodeType) {
        QSGGeometryNode *gn = static_cast<QSGGeometryNode *>(node);
        Element *e = shadowNode->element();
        if (e) {
            e->boundsComputed = false;
            Batch *b = e->batch;
            if (b) {
                if (!e->batch->geometryWasChanged(gn) || !e->batch->isOpaque) {
                    invalidateBatchAndOverlappingRenderOrders(e->batch);
                } else {
                    b->needsUpload = true;
                }
            }
        }
    }

    if (state & QSGNode::DirtyMaterial && node->type() == QSGNode::GeometryNodeType) {
        Element *e = shadowNode->element();
        if (e) {
            bool blended = hasMaterialWithBlending(static_cast<QSGGeometryNode *>(node));
            if (e->isMaterialBlended != blended) {
                m_rebuild |= Renderer::FullRebuild;
                e->isMaterialBlended = blended;
            } else if (e->batch) {
                if (e->batch->isMaterialCompatible(e) == BatchBreaksOnCompare)
                    invalidateBatchAndOverlappingRenderOrders(e->batch);
            } else {
                m_rebuild |= Renderer::BuildBatches;
            }
        }
    }

    // Mark the shadow tree dirty all the way back to the root...
    QSGNode::DirtyState dirtyChain = state & (QSGNode::DirtyNodeAdded
                                              | QSGNode::DirtyOpacity
                                              | QSGNode::DirtyMatrix
                                              | QSGNode::DirtySubtreeBlocked
                                              | QSGNode::DirtyForceUpdate);
    if (dirtyChain != 0) {
        dirtyChain = QSGNode::DirtyState(dirtyChain << 16);
        Node *sn = shadowNode->parent();
        while (sn) {
            sn->dirtyState |= dirtyChain;
            sn = sn->parent();
        }
    }

    // Delete happens at the very end because it deletes the shadownode.
    if (state & QSGNode::DirtyNodeRemoved) {
        Node *parent = shadowNode->parent();
        if (parent)
            parent->remove(shadowNode);
        nodeWasRemoved(shadowNode);
        Q_ASSERT(m_nodes.value(node) == 0);
    }

    QSGRenderer::nodeChanged(node, state);
}

/*
 * Traverses the tree and builds two list of geometry nodes. One for
 * the opaque and one for the translucent. These are populated
 * in the order they should visually appear in, meaning first
 * to the back and last to the front.
 *
 * We split opaque and translucent as we can perform different
 * types of reordering / batching strategies on them, depending
 *
 * Note: It would be tempting to use the shadow nodes instead of the QSGNodes
 * for traversal to avoid hash lookups, but the order of the children
 * is important and they are not preserved in the shadow tree, so we must
 * use the actual QSGNode tree.
 */
void Renderer::buildRenderLists(QSGNode *node)
{
    if (node->isSubtreeBlocked())
        return;

    Node *shadowNode = m_nodes.value(node);
    Q_ASSERT(shadowNode);

    if (node->type() == QSGNode::GeometryNodeType) {
        QSGGeometryNode *gn = static_cast<QSGGeometryNode *>(node);

        Element *e = shadowNode->element();
        Q_ASSERT(e);

        bool opaque = gn->inheritedOpacity() > OPAQUE_LIMIT && !(gn->activeMaterial()->flags() & QSGMaterial::Blending);
        if (opaque && m_useDepthBuffer)
            m_opaqueRenderList << e;
        else
            m_alphaRenderList << e;

        e->order = ++m_nextRenderOrder;
        // Used while rebuilding partial roots.
        if (m_partialRebuild)
            e->orphaned = false;

    } else if (node->type() == QSGNode::ClipNodeType || shadowNode->isBatchRoot) {
        Q_ASSERT(m_nodes.contains(node));
        BatchRootInfo *info = batchRootInfo(shadowNode);
        if (node == m_partialRebuildRoot) {
            m_nextRenderOrder = info->firstOrder;
            QSGNODE_TRAVERSE(node)
                    buildRenderLists(child);
            m_nextRenderOrder = info->lastOrder + 1;
        } else {
            int currentOrder = m_nextRenderOrder;
            QSGNODE_TRAVERSE(node)
                buildRenderLists(child);
            int padding = (m_nextRenderOrder - currentOrder) >> 2;
            info->firstOrder = currentOrder;
            info->availableOrders = padding;
            info->lastOrder = m_nextRenderOrder + padding;
            m_nextRenderOrder = info->lastOrder;
        }
        return;
    } else if (node->type() == QSGNode::RenderNodeType) {
        RenderNodeElement *e = shadowNode->renderNodeElement();
        m_alphaRenderList << e;
        e->order = ++m_nextRenderOrder;
        Q_ASSERT(e);
    }

    QSGNODE_TRAVERSE(node)
        buildRenderLists(child);
}

void Renderer::tagSubRoots(Node *node)
{
    BatchRootInfo *i = batchRootInfo(node);
    m_taggedRoots << node;
    for (QSet<Node *>::const_iterator it = i->subRoots.constBegin();
         it != i->subRoots.constEnd(); ++it) {
        tagSubRoots(*it);
    }
}

static void qsg_addOrphanedElements(QDataBuffer<Element *> &orphans, const QDataBuffer<Element *> &renderList)
{
    orphans.reset();
    for (int i=0; i<renderList.size(); ++i) {
        Element *e = renderList.at(i);
        if (e && !e->removed) {
            e->orphaned = true;
            orphans.add(e);
        }
    }
}

static void qsg_addBackOrphanedElements(QDataBuffer<Element *> &orphans, QDataBuffer<Element *> &renderList)
{
    for (int i=0; i<orphans.size(); ++i) {
        Element *e = orphans.at(i);
        if (e->orphaned)
            renderList.add(e);
    }
    orphans.reset();
}

/*
 * To rebuild the tagged roots, we start by putting all subroots of tagged
 * roots into the list of tagged roots. This is to make the rest of the
 * algorithm simpler.
 *
 * Second, we invalidate all batches which belong to tagged roots, which now
 * includes the entire subtree under a given root
 *
 * Then we call buildRenderLists for all tagged subroots which do not have
 * parents which are tagged, aka, we traverse only the topmosts roots.
 *
 * Then we sort the render lists based on their render order, to restore the
 * right order for rendering.
 */
void Renderer::buildRenderListsForTaggedRoots()
{
    // Flag any element that is currently in the render lists, but which
    // is not in a batch. This happens when we have a partial rebuild
    // in one sub tree while we have a BuildBatches change in another
    // isolated subtree. So that batch-building takes into account
    // these "orphaned" nodes, we flag them now. The ones under tagged
    // roots will be cleared again. The remaining ones are added into the
    // render lists so that they contain all visual nodes after the
    // function completes.
    qsg_addOrphanedElements(m_tmpOpaqueElements, m_opaqueRenderList);
    qsg_addOrphanedElements(m_tmpAlphaElements, m_alphaRenderList);

    // Take a copy now, as we will be adding to this while traversing..
    QSet<Node *> roots = m_taggedRoots;
    for (QSet<Node *>::const_iterator it = roots.constBegin();
         it != roots.constEnd(); ++it) {
        tagSubRoots(*it);
    }

    for (int i=0; i<m_opaqueBatches.size(); ++i) {
        Batch *b = m_opaqueBatches.at(i);
        if (m_taggedRoots.contains(b->root))
            invalidateAndRecycleBatch(b);

    }
    for (int i=0; i<m_alphaBatches.size(); ++i) {
        Batch *b = m_alphaBatches.at(i);
        if (m_taggedRoots.contains(b->root))
            invalidateAndRecycleBatch(b);
    }

    m_opaqueRenderList.reset();
    m_alphaRenderList.reset();
    int maxRenderOrder = m_nextRenderOrder;
    m_partialRebuild = true;
    // Traverse each root, assigning it
    for (QSet<Node *>::const_iterator it = m_taggedRoots.constBegin();
         it != m_taggedRoots.constEnd(); ++it) {
        Node *root = *it;
        BatchRootInfo *i = batchRootInfo(root);
        if ((!i->parentRoot || !m_taggedRoots.contains(i->parentRoot))
             && !nodeUpdater()->isNodeBlocked(root->sgNode, rootNode())) {
            m_nextRenderOrder = i->firstOrder;
            m_partialRebuildRoot = root->sgNode;
            buildRenderLists(root->sgNode);
        }
    }
    m_partialRebuild = false;
    m_partialRebuildRoot = nullptr;
    m_taggedRoots.clear();
    m_nextRenderOrder = qMax(m_nextRenderOrder, maxRenderOrder);

    // Add orphaned elements back into the list and then sort it..
    qsg_addBackOrphanedElements(m_tmpOpaqueElements, m_opaqueRenderList);
    qsg_addBackOrphanedElements(m_tmpAlphaElements, m_alphaRenderList);

    if (m_opaqueRenderList.size())
        std::sort(&m_opaqueRenderList.first(), &m_opaqueRenderList.last() + 1, qsg_sort_element_decreasing_order);
    if (m_alphaRenderList.size())
        std::sort(&m_alphaRenderList.first(), &m_alphaRenderList.last() + 1, qsg_sort_element_increasing_order);

}

void Renderer::buildRenderListsFromScratch()
{
    m_opaqueRenderList.reset();
    m_alphaRenderList.reset();

    for (int i=0; i<m_opaqueBatches.size(); ++i)
        invalidateAndRecycleBatch(m_opaqueBatches.at(i));
    for (int i=0; i<m_alphaBatches.size(); ++i)
        invalidateAndRecycleBatch(m_alphaBatches.at(i));
    m_opaqueBatches.reset();
    m_alphaBatches.reset();

    m_nextRenderOrder = 0;

    buildRenderLists(rootNode());
}

void Renderer::invalidateBatchAndOverlappingRenderOrders(Batch *batch)
{
    Q_ASSERT(batch);
    Q_ASSERT(batch->first);

    if (m_renderOrderRebuildLower < 0 || batch->first->order < m_renderOrderRebuildLower)
        m_renderOrderRebuildLower = batch->first->order;
    if (m_renderOrderRebuildUpper < 0 || batch->lastOrderInBatch > m_renderOrderRebuildUpper)
        m_renderOrderRebuildUpper = batch->lastOrderInBatch;

    batch->invalidate();

    for (int i=0; i<m_alphaBatches.size(); ++i) {
        Batch *b = m_alphaBatches.at(i);
        if (b->first) {
            int bf = b->first->order;
            int bl = b->lastOrderInBatch;
            if (bl > m_renderOrderRebuildLower && bf < m_renderOrderRebuildUpper)
                b->invalidate();
        }
    }

    m_rebuild |= BuildBatches;
}

/* Clean up batches by making it a consecutive list of "valid"
 * batches and moving all invalidated batches to the batches pool.
 */
void Renderer::cleanupBatches(QDataBuffer<Batch *> *batches) {
    if (batches->size()) {
        std::stable_sort(&batches->first(), &batches->last() + 1, qsg_sort_batch_is_valid);
        int count = 0;
        while (count < batches->size() && batches->at(count)->first)
            ++count;
        for (int i=count; i<batches->size(); ++i)
            invalidateAndRecycleBatch(batches->at(i));
        batches->resize(count);
    }
}

void Renderer::prepareOpaqueBatches()
{
    for (int i=m_opaqueRenderList.size() - 1; i >= 0; --i) {
        Element *ei = m_opaqueRenderList.at(i);
        if (!ei || ei->batch || ei->node->geometry()->vertexCount() == 0)
            continue;
        Batch *batch = newBatch();
        batch->first = ei;
        batch->root = ei->root;
        batch->isOpaque = true;
        batch->needsUpload = true;
        batch->positionAttribute = qsg_positionAttribute(ei->node->geometry());

        m_opaqueBatches.add(batch);

        ei->batch = batch;
        Element *next = ei;

        QSGGeometryNode *gni = ei->node;

        for (int j = i - 1; j >= 0; --j) {
            Element *ej = m_opaqueRenderList.at(j);
            if (!ej)
                continue;
            if (ej->root != ei->root)
                break;
            if (ej->batch || ej->node->geometry()->vertexCount() == 0)
                continue;

            QSGGeometryNode *gnj = ej->node;

            if (gni->clipList() == gnj->clipList()
                    && gni->geometry()->drawingMode() == gnj->geometry()->drawingMode()
                    && (gni->geometry()->drawingMode() != QSGGeometry::DrawLines || gni->geometry()->lineWidth() == gnj->geometry()->lineWidth())
                    && gni->geometry()->attributes() == gnj->geometry()->attributes()
                    && gni->inheritedOpacity() == gnj->inheritedOpacity()
                    && gni->activeMaterial()->type() == gnj->activeMaterial()->type()
                    && gni->activeMaterial()->compare(gnj->activeMaterial()) == 0) {
                ej->batch = batch;
                next->nextInBatch = ej;
                next = ej;
            }
        }

        batch->lastOrderInBatch = next->order;
    }
}

bool Renderer::checkOverlap(int first, int last, const Rect &bounds)
{
    for (int i=first; i<=last; ++i) {
        Element *e = m_alphaRenderList.at(i);
        if (!e || e->batch)
            continue;
        Q_ASSERT(e->boundsComputed);
        if (e->bounds.intersects(bounds))
            return true;
    }
    return false;
}

/*
 *
 * To avoid the O(n^2) checkOverlap check in most cases, we have the
 * overlapBounds which is the union of all bounding rects to check overlap
 * for. We know that if it does not overlap, then none of the individual
 * ones will either. For the typical list case, this results in no calls
 * to checkOverlap what-so-ever. This also ensures that when all consecutive
 * items are matching (such as a table of text), we don't build up an
 * overlap bounds and thus do not require full overlap checks.
 */

void Renderer::prepareAlphaBatches()
{
    for (int i=0; i<m_alphaRenderList.size(); ++i) {
        Element *e = m_alphaRenderList.at(i);
        if (!e || e->isRenderNode)
            continue;
        Q_ASSERT(!e->removed);
        e->ensureBoundsValid();
    }

    for (int i=0; i<m_alphaRenderList.size(); ++i) {
        Element *ei = m_alphaRenderList.at(i);
        if (!ei || ei->batch)
            continue;

        if (ei->isRenderNode) {
            Batch *rnb = newBatch();
            rnb->first = ei;
            rnb->root = ei->root;
            rnb->isOpaque = false;
            rnb->isRenderNode = true;
            ei->batch = rnb;
            m_alphaBatches.add(rnb);
            continue;
        }

        if (ei->node->geometry()->vertexCount() == 0)
            continue;

        Batch *batch = newBatch();
        batch->first = ei;
        batch->root = ei->root;
        batch->isOpaque = false;
        batch->needsUpload = true;
        m_alphaBatches.add(batch);
        ei->batch = batch;

        QSGGeometryNode *gni = ei->node;
        batch->positionAttribute = qsg_positionAttribute(gni->geometry());

        Rect overlapBounds;
        overlapBounds.set(FLT_MAX, FLT_MAX, -FLT_MAX, -FLT_MAX);

        Element *next = ei;

        for (int j = i + 1; j < m_alphaRenderList.size(); ++j) {
            Element *ej = m_alphaRenderList.at(j);
            if (!ej)
                continue;
            if (ej->root != ei->root || ej->isRenderNode)
                break;
            if (ej->batch)
                continue;

            QSGGeometryNode *gnj = ej->node;
            if (gnj->geometry()->vertexCount() == 0)
                continue;

            if (gni->clipList() == gnj->clipList()
                    && gni->geometry()->drawingMode() == gnj->geometry()->drawingMode()
                    && (gni->geometry()->drawingMode() != QSGGeometry::DrawLines || gni->geometry()->lineWidth() == gnj->geometry()->lineWidth())
                    && gni->geometry()->attributes() == gnj->geometry()->attributes()
                    && gni->inheritedOpacity() == gnj->inheritedOpacity()
                    && gni->activeMaterial()->type() == gnj->activeMaterial()->type()
                    && gni->activeMaterial()->compare(gnj->activeMaterial()) == 0) {
                if (!overlapBounds.intersects(ej->bounds) || !checkOverlap(i+1, j - 1, ej->bounds)) {
                    ej->batch = batch;
                    next->nextInBatch = ej;
                    next = ej;
                } else {
                    /* When we come across a compatible element which hits an overlap, we
                     * need to stop the batch right away. We cannot add more elements
                     * to the current batch as they will be rendered before the batch that the
                     * current 'ej' will be added to.
                     */
                    break;
                }
            } else {
                overlapBounds |= ej->bounds;
            }
        }

        batch->lastOrderInBatch = next->order;
    }


}

static inline int qsg_fixIndexCount(int iCount, int drawMode)
{
    switch (drawMode) {
    case QSGGeometry::DrawTriangleStrip:
        // Merged triangle strips need to contain degenerate triangles at the beginning and end.
        // One could save 2 uploaded ushorts here by ditching the padding for the front of the
        // first and the end of the last, but for simplicity, we simply don't care.
        // Those extra triangles will be skipped while drawing to preserve the strip's parity
        // anyhow.
        return iCount + 2;
    case QSGGeometry::DrawLines:
        // For lines we drop the last vertex if the number of vertices is uneven.
        return iCount - (iCount % 2);
    case QSGGeometry::DrawTriangles:
        // For triangles we drop trailing vertices until the result is divisible by 3.
        return iCount - (iCount % 3);
    default:
        return iCount;
    }
}

/* These parameters warrant some explanation...
 *
 * vaOffset: The byte offset into the vertex data to the location of the
 *           2D float point vertex attributes.
 *
 * vertexData: destination where the geometry's vertex data should go
 *
 * zData: destination of geometries injected Z positioning
 *
 * indexData: destination of the indices for this element
 *
 * iBase: The starting index for this element in the batch
 */

void Renderer::uploadMergedElement(Element *e, int vaOffset, char **vertexData, char **zData, char **indexData, void *iBasePtr, int *indexCount)
{
    if (Q_UNLIKELY(debug_upload())) qDebug() << "  - uploading element:" << e << e->node << (void *) *vertexData << (qintptr) (*zData - *vertexData) << (qintptr) (*indexData - *vertexData);
    QSGGeometry *g = e->node->geometry();

    const QMatrix4x4 &localx = *e->node->matrix();

    const int vCount = g->vertexCount();
    const int vSize = g->sizeOfVertex();
    memcpy(*vertexData, g->vertexData(), vSize * vCount);

    // apply vertex transform..
    char *vdata = *vertexData + vaOffset;
    if (((const QMatrix4x4_Accessor &) localx).flagBits == 1) {
        for (int i=0; i<vCount; ++i) {
            Pt *p = (Pt *) vdata;
            p->x += ((const QMatrix4x4_Accessor &) localx).m[3][0];
            p->y += ((const QMatrix4x4_Accessor &) localx).m[3][1];
            vdata += vSize;
        }
    } else if (((const QMatrix4x4_Accessor &) localx).flagBits > 1) {
        for (int i=0; i<vCount; ++i) {
            ((Pt *) vdata)->map(localx);
            vdata += vSize;
        }
    }

    if (m_useDepthBuffer) {
        float *vzorder = (float *) *zData;
        float zorder = 1.0f - e->order * m_zRange;
        for (int i=0; i<vCount; ++i)
            vzorder[i] = zorder;
        *zData += vCount * sizeof(float);
    }

    int iCount = g->indexCount();
    if (m_uint32IndexForRhi) {
        // can only happen when using the rhi
        quint32 *iBase = (quint32 *) iBasePtr;
        quint32 *indices = (quint32 *) *indexData;
        if (iCount == 0) {
            iCount = vCount;
            if (g->drawingMode() == QSGGeometry::DrawTriangleStrip)
                *indices++ = *iBase;
            else
                iCount = qsg_fixIndexCount(iCount, g->drawingMode());

            for (int i=0; i<iCount; ++i)
                indices[i] = *iBase + i;
        } else {
            // source index data in QSGGeometry is always ushort (we would not merge otherwise)
            const quint16 *srcIndices = g->indexDataAsUShort();
            if (g->drawingMode() == QSGGeometry::DrawTriangleStrip)
                *indices++ = *iBase + srcIndices[0];
            else
                iCount = qsg_fixIndexCount(iCount, g->drawingMode());

            for (int i=0; i<iCount; ++i)
                indices[i] = *iBase + srcIndices[i];
        }
        if (g->drawingMode() == QSGGeometry::DrawTriangleStrip) {
            indices[iCount] = indices[iCount - 1];
            iCount += 2;
        }
        *iBase += vCount;
    } else {
        // normally batching is only done for ushort index data
        quint16 *iBase = (quint16 *) iBasePtr;
        quint16 *indices = (quint16 *) *indexData;
        if (iCount == 0) {
            iCount = vCount;
            if (g->drawingMode() == QSGGeometry::DrawTriangleStrip)
                *indices++ = *iBase;
            else
                iCount = qsg_fixIndexCount(iCount, g->drawingMode());

            for (int i=0; i<iCount; ++i)
                indices[i] = *iBase + i;
        } else {
            const quint16 *srcIndices = g->indexDataAsUShort();
            if (g->drawingMode() == QSGGeometry::DrawTriangleStrip)
                *indices++ = *iBase + srcIndices[0];
            else
                iCount = qsg_fixIndexCount(iCount, g->drawingMode());

            for (int i=0; i<iCount; ++i)
                indices[i] = *iBase + srcIndices[i];
        }
        if (g->drawingMode() == QSGGeometry::DrawTriangleStrip) {
            indices[iCount] = indices[iCount - 1];
            iCount += 2;
        }
        *iBase += vCount;
    }

    *vertexData += vCount * vSize;
    *indexData += iCount * mergedIndexElemSize();
    *indexCount += iCount;
}

QMatrix4x4 qsg_matrixForRoot(Node *node)
{
    if (node->type() == QSGNode::TransformNodeType)
        return static_cast<QSGTransformNode *>(node->sgNode)->combinedMatrix();
    Q_ASSERT(node->type() == QSGNode::ClipNodeType);
    QSGClipNode *c = static_cast<QSGClipNode *>(node->sgNode);
    return *c->matrix();
}

void Renderer::uploadBatch(Batch *b)
{
    // Early out if nothing has changed in this batch..
    if (!b->needsUpload) {
        if (Q_UNLIKELY(debug_upload())) qDebug() << " Batch:" << b << "already uploaded...";
        return;
    }

    if (!b->first) {
        if (Q_UNLIKELY(debug_upload())) qDebug() << " Batch:" << b << "is invalid...";
        return;
    }

    if (b->isRenderNode) {
        if (Q_UNLIKELY(debug_upload())) qDebug() << " Batch: " << b << "is a render node...";
        return;
    }

    // Figure out if we can merge or not, if not, then just render the batch as is..
    Q_ASSERT(b->first);
    Q_ASSERT(b->first->node);

    QSGGeometryNode *gn = b->first->node;
    QSGGeometry *g =  gn->geometry();
    QSGMaterial::Flags flags = gn->activeMaterial()->flags();
    bool canMerge = (g->drawingMode() == QSGGeometry::DrawTriangles || g->drawingMode() == QSGGeometry::DrawTriangleStrip ||
                     g->drawingMode() == QSGGeometry::DrawLines || g->drawingMode() == QSGGeometry::DrawPoints)
            && b->positionAttribute >= 0
            && g->indexType() == QSGGeometry::UnsignedShortType
            && (flags & (QSGMaterial::CustomCompileStep | QSGMaterial_FullMatrix)) == 0
            && ((flags & QSGMaterial::RequiresFullMatrixExceptTranslate) == 0 || b->isTranslateOnlyToRoot())
            && b->isSafeToBatch();

    b->merged = canMerge;

    // Figure out how much memory we need...
    b->vertexCount = 0;
    b->indexCount = 0;
    int unmergedIndexSize = 0;
    Element *e = b->first;

    while (e) {
        QSGGeometry *eg = e->node->geometry();
        b->vertexCount += eg->vertexCount();
        int iCount = eg->indexCount();
        if (b->merged) {
            if (iCount == 0)
                iCount = eg->vertexCount();
            iCount = qsg_fixIndexCount(iCount, g->drawingMode());
        } else {
            const int effectiveIndexSize = m_uint32IndexForRhi ? sizeof(quint32) : eg->sizeOfIndex();
            unmergedIndexSize += iCount * effectiveIndexSize;
        }
        b->indexCount += iCount;
        e = e->nextInBatch;
    }

    // Abort if there are no vertices in this batch.. We abort this late as
    // this is a broken usecase which we do not care to optimize for...
    if (b->vertexCount == 0 || (b->merged && b->indexCount == 0))
        return;

    /* Allocate memory for this batch. Merged batches are divided into three separate blocks
           1. Vertex data for all elements, as they were in the QSGGeometry object, but
              with the tranform relative to this batch's root applied. The vertex data
              is otherwise unmodified.
           2. Z data for all elements, derived from each elements "render order".
              This is present for merged data only.
           3. Indices for all elements, as they were in the QSGGeometry object, but
              adjusted so that each index matches its.
              And for TRIANGLE_STRIPs, we need to insert degenerate between each
              primitive. These are unsigned shorts for merged and arbitrary for
              non-merged.
         */
    int bufferSize =  b->vertexCount * g->sizeOfVertex();
    int ibufferSize = 0;
    if (b->merged) {
        ibufferSize = b->indexCount * mergedIndexElemSize();
        if (m_useDepthBuffer)
            bufferSize += b->vertexCount * sizeof(float);
    } else {
        ibufferSize = unmergedIndexSize;
    }

    const bool separateIndexBuffer = m_context->separateIndexBuffer();
    if (separateIndexBuffer)
        map(&b->ibo, ibufferSize, true);
    else
        bufferSize += ibufferSize;
    map(&b->vbo, bufferSize);

    if (Q_UNLIKELY(debug_upload())) qDebug() << " - batch" << b << " first:" << b->first << " root:"
                                             << b->root << " merged:" << b->merged << " positionAttribute" << b->positionAttribute
                                             << " vbo:" << b->vbo.id << ":" << b->vbo.size;

    if (b->merged) {
        char *vertexData = b->vbo.data;
        char *zData = vertexData + b->vertexCount * g->sizeOfVertex();
        char *indexData = separateIndexBuffer
                ? b->ibo.data
                : zData + (int(m_useDepthBuffer) * b->vertexCount * sizeof(float));

        quint16 iOffset16 = 0;
        quint32 iOffset32 = 0;
        e = b->first;
        uint verticesInSet = 0;
        // Start a new set already after 65534 vertices because 0xFFFF may be
        // used for an always-on primitive restart with some apis (adapt for
        // uint32 indices as appropriate).
        const uint verticesInSetLimit = m_uint32IndexForRhi ? 0xfffffffe : 0xfffe;
        int indicesInSet = 0;
        b->drawSets.reset();
        int drawSetIndices = separateIndexBuffer ? 0 : indexData - vertexData;
        const char *indexBase = separateIndexBuffer ? b->ibo.data : b->vbo.data;
        b->drawSets << DrawSet(0, zData - vertexData, drawSetIndices);
        while (e) {
            verticesInSet += e->node->geometry()->vertexCount();
            if (verticesInSet > verticesInSetLimit) {
                b->drawSets.last().indexCount = indicesInSet;
                if (g->drawingMode() == QSGGeometry::DrawTriangleStrip) {
                    b->drawSets.last().indices += 1 * mergedIndexElemSize();
                    b->drawSets.last().indexCount -= 2;
                }
                drawSetIndices = indexData - indexBase;
                b->drawSets << DrawSet(vertexData - b->vbo.data,
                                       zData - b->vbo.data,
                                       drawSetIndices);
                iOffset16 = 0;
                iOffset32 = 0;
                verticesInSet = e->node->geometry()->vertexCount();
                indicesInSet = 0;
            }
            void *iBasePtr = &iOffset16;
            if (m_uint32IndexForRhi)
                iBasePtr = &iOffset32;
            uploadMergedElement(e, b->positionAttribute, &vertexData, &zData, &indexData, iBasePtr, &indicesInSet);
            e = e->nextInBatch;
        }
        b->drawSets.last().indexCount = indicesInSet;
        // We skip the very first and very last degenerate triangles since they aren't needed
        // and the first one would reverse the vertex ordering of the merged strips.
        if (g->drawingMode() == QSGGeometry::DrawTriangleStrip) {
            b->drawSets.last().indices += 1 * mergedIndexElemSize();
            b->drawSets.last().indexCount -= 2;
        }
    } else {
        char *vboData = b->vbo.data;
        char *iboData = separateIndexBuffer ? b->ibo.data
                                            : vboData + b->vertexCount * g->sizeOfVertex();
        Element *e = b->first;
        while (e) {
            QSGGeometry *g = e->node->geometry();
            int vbs = g->vertexCount() * g->sizeOfVertex();
            memcpy(vboData, g->vertexData(), vbs);
            vboData = vboData + vbs;
            const int indexCount = g->indexCount();
            if (indexCount) {
                if (!m_rhi) {
                    int ibs = g->indexCount() * g->sizeOfIndex();
                    memcpy(iboData, g->indexData(), ibs);
                    iboData += ibs;
                } else {
                    const int effectiveIndexSize = m_uint32IndexForRhi ? sizeof(quint32) : g->sizeOfIndex();
                    const int ibs = indexCount * effectiveIndexSize;
                    if (g->sizeOfIndex() == effectiveIndexSize) {
                        memcpy(iboData, g->indexData(), ibs);
                    } else {
                        if (g->sizeOfIndex() == sizeof(quint16) && effectiveIndexSize == sizeof(quint32)) {
                            quint16 *src = g->indexDataAsUShort();
                            quint32 *dst = (quint32 *) iboData;
                            for (int i = 0; i < indexCount; ++i)
                                dst[i] = src[i];
                        } else {
                            Q_ASSERT_X(false, "uploadBatch (unmerged)", "uint index with ushort effective index - cannot happen");
                        }
                    }
                    iboData += ibs;
                }
            }
            e = e->nextInBatch;
        }
    }
#ifndef QT_NO_DEBUG_OUTPUT
    if (Q_UNLIKELY(debug_upload())) {
        const char *vd = b->vbo.data;
        qDebug() << "  -- Vertex Data, count:" << b->vertexCount << " - " << g->sizeOfVertex() << "bytes/vertex";
        for (int i=0; i<b->vertexCount; ++i) {
            QDebug dump = qDebug().nospace();
            dump << "  --- " << i << ": ";
            int offset = 0;
            for (int a=0; a<g->attributeCount(); ++a) {
                const QSGGeometry::Attribute &attr = g->attributes()[a];
                dump << attr.position << ":(" << attr.tupleSize << ",";
                if (attr.type == QSGGeometry::FloatType) {
                    dump << "float ";
                    if (attr.isVertexCoordinate)
                        dump << "* ";
                    for (int t=0; t<attr.tupleSize; ++t)
                        dump << *(const float *)(vd + offset + t * sizeof(float)) << " ";
                } else if (attr.type == QSGGeometry::UnsignedByteType) {
                    dump << "ubyte ";
                    for (int t=0; t<attr.tupleSize; ++t)
                        dump << *(const unsigned char *)(vd + offset + t * sizeof(unsigned char)) << " ";
                }
                dump << ") ";
                offset += attr.tupleSize * size_of_type(attr.type);
            }
            if (b->merged && m_useDepthBuffer) {
                float zorder = ((float*)(b->vbo.data + b->vertexCount * g->sizeOfVertex()))[i];
                dump << " Z:(" << zorder << ")";
            }
            vd += g->sizeOfVertex();
        }

        if (!b->drawSets.isEmpty()) {
            if (m_uint32IndexForRhi) {
                const quint32 *id = (const quint32 *)(separateIndexBuffer
                                                      ? b->ibo.data
                                                      : b->vbo.data + b->drawSets.at(0).indices);
                {
                    QDebug iDump = qDebug();
                    iDump << "  -- Index Data, count:" << b->indexCount;
                    for (int i=0; i<b->indexCount; ++i) {
                        if ((i % 24) == 0)
                            iDump << Qt::endl << "  --- ";
                        iDump << id[i];
                    }
                }
            } else {
                const quint16 *id = (const quint16 *)(separateIndexBuffer
                                                      ? b->ibo.data
                                                      : b->vbo.data + b->drawSets.at(0).indices);
                {
                    QDebug iDump = qDebug();
                    iDump << "  -- Index Data, count:" << b->indexCount;
                    for (int i=0; i<b->indexCount; ++i) {
                        if ((i % 24) == 0)
                            iDump << Qt::endl << "  --- ";
                        iDump << id[i];
                    }
                }
            }

            for (int i=0; i<b->drawSets.size(); ++i) {
                const DrawSet &s = b->drawSets.at(i);
                qDebug() << "  -- DrawSet: indexCount:" << s.indexCount << " vertices:" << s.vertices << " z:" << s.zorders << " indices:" << s.indices;
            }
        }
    }
#endif // QT_NO_DEBUG_OUTPUT

    unmap(&b->vbo);
    if (separateIndexBuffer)
        unmap(&b->ibo, true);

    if (Q_UNLIKELY(debug_upload())) qDebug() << "  --- vertex/index buffers unmapped, batch upload completed...";

    b->needsUpload = false;

    if (Q_UNLIKELY(debug_render()))
        b->uploadedThisFrame = true;
}

/*!
 * Convenience function to set up the stencil buffer for clipping based on \a clip.
 *
 * If the clip is a pixel aligned rectangle, this function will use glScissor instead
 * of stencil.
 */
ClipState::ClipType Renderer::updateStencilClip(const QSGClipNode *clip)
{
    if (!clip) {
        glDisable(GL_STENCIL_TEST);
        glDisable(GL_SCISSOR_TEST);
        return ClipState::NoClip;
    }

    ClipState::ClipType clipType = ClipState::NoClip;
    GLuint vbo = 0;
    int vboSize = 0;

    bool useVBO = false;
    QOpenGLContext *ctx = m_context->openglContext();
    QSurfaceFormat::OpenGLContextProfile profile = ctx->format().profile();

    if (!ctx->isOpenGLES() && profile == QSurfaceFormat::CoreProfile) {
        // VBO are more expensive, so only use them if we must.
        useVBO = true;
    }

    glDisable(GL_SCISSOR_TEST);

    m_currentStencilValue = 0;
    m_currentScissorRect = QRect();
    while (clip) {
        QMatrix4x4 m = m_current_projection_matrix;
        if (clip->matrix())
            m *= *clip->matrix();

        // TODO: Check for multisampling and pixel grid alignment.
        bool isRectangleWithNoPerspective = clip->isRectangular()
                && qFuzzyIsNull(m(3, 0)) && qFuzzyIsNull(m(3, 1));
        auto noRotate = [] (const QMatrix4x4 &m) { return qFuzzyIsNull(m(0, 1)) && qFuzzyIsNull(m(1, 0)); };
        auto isRotate90 = [] (const QMatrix4x4 &m) { return qFuzzyIsNull(m(0, 0)) && qFuzzyIsNull(m(1, 1)); };
        auto scissorRect = [&] (const QRectF &bbox, const QMatrix4x4 &m) {
            qreal invW = 1 / m(3, 3);
            qreal fx1, fy1, fx2, fy2;
            if (noRotate(m)) {
                fx1 = (bbox.left() * m(0, 0) + m(0, 3)) * invW;
                fy1 = (bbox.bottom() * m(1, 1) + m(1, 3)) * invW;
                fx2 = (bbox.right() * m(0, 0) + m(0, 3)) * invW;
                fy2 = (bbox.top() * m(1, 1) + m(1, 3)) * invW;
            } else {
                Q_ASSERT(isRotate90(m));
                fx1 = (bbox.bottom() * m(0, 1) + m(0, 3)) * invW;
                fy1 = (bbox.left() * m(1, 0) + m(1, 3)) * invW;
                fx2 = (bbox.top() * m(0, 1) + m(0, 3)) * invW;
                fy2 = (bbox.right() * m(1, 0) + m(1, 3)) * invW;
            }

            if (fx1 > fx2)
                qSwap(fx1, fx2);
            if (fy1 > fy2)
                qSwap(fy1, fy2);

            QRect deviceRect = this->deviceRect();

            GLint ix1 = qRound((fx1 + 1) * deviceRect.width() * qreal(0.5));
            GLint iy1 = qRound((fy1 + 1) * deviceRect.height() * qreal(0.5));
            GLint ix2 = qRound((fx2 + 1) * deviceRect.width() * qreal(0.5));
            GLint iy2 = qRound((fy2 + 1) * deviceRect.height() * qreal(0.5));

            return QRect(ix1, iy1, ix2 - ix1, iy2 - iy1);
        };

        if (isRectangleWithNoPerspective && (noRotate(m) || isRotate90(m))) {
            auto rect = scissorRect(clip->clipRect(), m);

            if (!(clipType & ClipState::ScissorClip)) {
                m_currentScissorRect = rect;
                glEnable(GL_SCISSOR_TEST);
                clipType |= ClipState::ScissorClip;
            } else {
                m_currentScissorRect &= rect;
            }
            glScissor(m_currentScissorRect.x(), m_currentScissorRect.y(),
                      m_currentScissorRect.width(), m_currentScissorRect.height());
        } else {
            if (!(clipType & ClipState::StencilClip)) {
                if (!m_clipProgram.isLinked()) {
                    QSGShaderSourceBuilder::initializeProgramFromFiles(
                        &m_clipProgram,
                        QStringLiteral(":/qt-project.org/scenegraph/shaders/stencilclip.vert"),
                        QStringLiteral(":/qt-project.org/scenegraph/shaders/stencilclip.frag"));
                    m_clipProgram.bindAttributeLocation("vCoord", 0);
                    m_clipProgram.link();
                    m_clipMatrixId = m_clipProgram.uniformLocation("matrix");
                }
                const QSGClipNode *clipNext = clip->clipList();
                if (clipNext) {
                    QMatrix4x4 mNext = m_current_projection_matrix;
                    if (clipNext->matrix())
                        mNext *= *clipNext->matrix();

                    auto rect = scissorRect(clipNext->clipRect(), mNext);

                    ClipState::ClipType clipTypeNext = clipType ;
                    clipTypeNext |= ClipState::StencilClip;
                    QRect m_next_scissor_rect = m_currentScissorRect;
                    if (!(clipTypeNext & ClipState::ScissorClip)) {
                        m_next_scissor_rect = rect;
                        glEnable(GL_SCISSOR_TEST);
                    } else {
                        m_next_scissor_rect =
                           m_currentScissorRect & rect;
                    }
                    glScissor(m_next_scissor_rect.x(), m_next_scissor_rect.y(),
                              m_next_scissor_rect.width(), m_next_scissor_rect.height());
                }

                glClearStencil(0);
                glClear(GL_STENCIL_BUFFER_BIT);
                glDisable(GL_SCISSOR_TEST);
                glEnable(GL_STENCIL_TEST);
                glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
                glDepthMask(GL_FALSE);

                m_clipProgram.bind();
                m_clipProgram.enableAttributeArray(0);

                clipType |= ClipState::StencilClip;
            }

            glStencilFunc(GL_EQUAL, m_currentStencilValue, 0xff); // stencil test, ref, test mask
            glStencilOp(GL_KEEP, GL_KEEP, GL_INCR); // stencil fail, z fail, z pass

            const QSGGeometry *g = clip->geometry();
            Q_ASSERT(g->attributeCount() > 0);
            const QSGGeometry::Attribute *a = g->attributes();

            const GLvoid *pointer;
            if (!useVBO) {
                pointer = g->vertexData();
            } else {
                if (!vbo)
                    glGenBuffers(1, &vbo);

                glBindBuffer(GL_ARRAY_BUFFER, vbo);

                const int vertexByteSize = g->sizeOfVertex() * g->vertexCount();
                if (vboSize < vertexByteSize) {
                    vboSize = vertexByteSize;
                    glBufferData(GL_ARRAY_BUFFER, vertexByteSize, g->vertexData(), GL_STATIC_DRAW);
                } else {
                    glBufferSubData(GL_ARRAY_BUFFER, 0, vertexByteSize, g->vertexData());
                }

                pointer = nullptr;
            }

            glVertexAttribPointer(0, a->tupleSize, a->type, GL_FALSE, g->sizeOfVertex(), pointer);

            m_clipProgram.setUniformValue(m_clipMatrixId, m);
            if (g->indexCount()) {
                glDrawElements(g->drawingMode(), g->indexCount(), g->indexType(), g->indexData());
            } else {
                glDrawArrays(g->drawingMode(), 0, g->vertexCount());
            }

            if (useVBO)
                glBindBuffer(GL_ARRAY_BUFFER, 0);

            ++m_currentStencilValue;
        }

        clip = clip->clipList();
    }

    if (vbo)
        glDeleteBuffers(1, &vbo);

    if (clipType & ClipState::StencilClip) {
        m_clipProgram.disableAttributeArray(0);
        glStencilFunc(GL_EQUAL, m_currentStencilValue, 0xff); // stencil test, ref, test mask
        glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP); // stencil fail, z fail, z pass
        bindable()->reactivate();
    } else {
        glDisable(GL_STENCIL_TEST);
    }

    return clipType;
}

void Renderer::updateClip(const QSGClipNode *clipList, const Batch *batch) // legacy (GL-only)
{
    if (clipList != m_currentClip && Q_LIKELY(!debug_noclip())) {
        m_currentClip = clipList;
        // updateClip sets another program, so force-reactivate our own
        if (m_currentShader)
            setActiveShader(nullptr, nullptr);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        if (batch->isOpaque)
            glDisable(GL_DEPTH_TEST);
        m_currentClipType = updateStencilClip(m_currentClip);
        if (batch->isOpaque) {
            glEnable(GL_DEPTH_TEST);
            if (m_currentClipType & ClipState::StencilClip)
                glDepthMask(true);
        }
    }
}

/*!
 * Look at the attribute arrays and potentially the injected z attribute to figure out
 * which vertex attribute arrays need to be enabled and not. Then update the current
 * Shader and current QSGMaterialShader.
 */
void Renderer::setActiveShader(QSGMaterialShader *program, ShaderManager::Shader *shader) // legacy (GL-only)
{
    Q_ASSERT(!m_rhi);
    const char * const *c = m_currentProgram ? m_currentProgram->attributeNames() : nullptr;
    const char * const *n = program ? program->attributeNames() : nullptr;

    int cza = m_currentShader ? m_currentShader->programGL.pos_order : -1;
    int nza = shader ? shader->programGL.pos_order : -1;

    int i = 0;
    while (c || n) {

        bool was = c;
        if (cza == i) {
            was = true;
            c = nullptr;
        } else if (c && !c[i]) { // end of the attribute array names
            c = nullptr;
            was = false;
        }

        bool is = n;
        if (nza == i) {
            is = true;
            n = nullptr;
        } else if (n && !n[i]) {
            n = nullptr;
            is = false;
        }

        if (is && !was)
            glEnableVertexAttribArray(i);
        else if (was && !is)
            glDisableVertexAttribArray(i);

        ++i;
    }

    if (m_currentProgram)
        m_currentProgram->deactivate();
    m_currentProgram = program;
    m_currentShader = shader;
    m_currentMaterial = nullptr;
    if (m_currentProgram) {
        m_currentProgram->program()->bind();
        m_currentProgram->activate();
    }
}

void Renderer::applyClipStateToGraphicsState() // RHI only
{
    m_gstate.usesScissor = (m_currentClipState.type & ClipState::ScissorClip);
    m_gstate.stencilTest = (m_currentClipState.type & ClipState::StencilClip);
}

QRhiGraphicsPipeline *Renderer::buildStencilPipeline(const Batch *batch, bool firstStencilClipInBatch)
{
    QRhiGraphicsPipeline *ps = m_rhi->newGraphicsPipeline();
    ps->setFlags(QRhiGraphicsPipeline::UsesStencilRef);
    QRhiGraphicsPipeline::TargetBlend blend;
    blend.colorWrite = {};
    ps->setTargetBlends({ blend });
    ps->setSampleCount(renderTarget()->sampleCount());
    ps->setStencilTest(true);
    QRhiGraphicsPipeline::StencilOpState stencilOp;
    if (firstStencilClipInBatch) {
        stencilOp.compareOp = QRhiGraphicsPipeline::Always;
        stencilOp.failOp = QRhiGraphicsPipeline::Keep;
        stencilOp.depthFailOp = QRhiGraphicsPipeline::Keep;
        stencilOp.passOp = QRhiGraphicsPipeline::Replace;
    } else {
        stencilOp.compareOp = QRhiGraphicsPipeline::Equal;
        stencilOp.failOp = QRhiGraphicsPipeline::Keep;
        stencilOp.depthFailOp = QRhiGraphicsPipeline::Keep;
        stencilOp.passOp = QRhiGraphicsPipeline::IncrementAndClamp;
    }
    ps->setStencilFront(stencilOp);
    ps->setStencilBack(stencilOp);

    ps->setTopology(m_stencilClipCommon.topology);

    ps->setShaderStages({ QRhiGraphicsShaderStage(QRhiGraphicsShaderStage::Vertex, m_stencilClipCommon.vs),
                          QRhiGraphicsShaderStage(QRhiGraphicsShaderStage::Fragment, m_stencilClipCommon.fs) });
    ps->setVertexInputLayout(m_stencilClipCommon.inputLayout);
    ps->setShaderResourceBindings(batch->stencilClipState.srb); // use something, it just needs to be layout-compatible
    ps->setRenderPassDescriptor(renderPassDescriptor());

    if (!ps->build()) {
        qWarning("Failed to build stencil clip pipeline");
        delete ps;
        return nullptr;
    }

    return ps;
}

void Renderer::updateClipState(const QSGClipNode *clipList, Batch *batch) // RHI only
{
    // Note: No use of the clip-related speparate m_current* vars is allowed
    // here. All stored in batch->clipState instead. To collect state during
    // the prepare steps, m_currentClipState is used. It should not be used in
    // the render steps afterwards.

    // The stenciling logic is slightly different from the legacy GL path as we
    // cannot just randomly clear the stencil buffer. We now put all clip
    // shapes into the stencil buffer for all batches in the frame. This means
    // that the number of total clips in a scene is reduced (since the stencil
    // value cannot exceed 255) but we do not need any clears inbetween.

    Q_ASSERT(m_rhi);
    batch->stencilClipState.updateStencilBuffer = false;
    if (clipList == m_currentClipState.clipList || Q_UNLIKELY(debug_noclip())) {
        applyClipStateToGraphicsState();
        batch->clipState = m_currentClipState;
        return;
    }

    ClipState::ClipType clipType = ClipState::NoClip;
    QRect scissorRect;
    QVarLengthArray<const QSGClipNode *, 4> stencilClipNodes;
    const QSGClipNode *clip = clipList;

    batch->stencilClipState.drawCalls.reset();
    int totalVSize = 0;
    int totalISize = 0;
    int totalUSize = 0;
    const int StencilClipUbufSize = 64;

    while (clip) {
        QMatrix4x4 m = m_current_projection_matrix_native_ndc;
        if (clip->matrix())
            m *= *clip->matrix();

        bool isRectangleWithNoPerspective = clip->isRectangular()
                && qFuzzyIsNull(m(3, 0)) && qFuzzyIsNull(m(3, 1));
        bool noRotate = qFuzzyIsNull(m(0, 1)) && qFuzzyIsNull(m(1, 0));
        bool isRotate90 = qFuzzyIsNull(m(0, 0)) && qFuzzyIsNull(m(1, 1));

        if (isRectangleWithNoPerspective && (noRotate || isRotate90)) {
            QRectF bbox = clip->clipRect();
            qreal invW = 1 / m(3, 3);
            qreal fx1, fy1, fx2, fy2;
            if (noRotate) {
                fx1 = (bbox.left() * m(0, 0) + m(0, 3)) * invW;
                fy1 = (bbox.bottom() * m(1, 1) + m(1, 3)) * invW;
                fx2 = (bbox.right() * m(0, 0) + m(0, 3)) * invW;
                fy2 = (bbox.top() * m(1, 1) + m(1, 3)) * invW;
            } else {
                Q_ASSERT(isRotate90);
                fx1 = (bbox.bottom() * m(0, 1) + m(0, 3)) * invW;
                fy1 = (bbox.left() * m(1, 0) + m(1, 3)) * invW;
                fx2 = (bbox.top() * m(0, 1) + m(0, 3)) * invW;
                fy2 = (bbox.right() * m(1, 0) + m(1, 3)) * invW;
            }

            if (fx1 > fx2)
                qSwap(fx1, fx2);
            if (fy1 > fy2)
                qSwap(fy1, fy2);

            QRect deviceRect = this->deviceRect();

            GLint ix1 = qRound((fx1 + 1) * deviceRect.width() * qreal(0.5));
            GLint iy1 = qRound((fy1 + 1) * deviceRect.height() * qreal(0.5));
            GLint ix2 = qRound((fx2 + 1) * deviceRect.width() * qreal(0.5));
            GLint iy2 = qRound((fy2 + 1) * deviceRect.height() * qreal(0.5));

            if (!(clipType & ClipState::ScissorClip)) {
                clipType |= ClipState::ScissorClip;
                scissorRect = QRect(ix1, iy1, ix2 - ix1, iy2 - iy1);
            } else {
                scissorRect &= QRect(ix1, iy1, ix2 - ix1, iy2 - iy1);
            }
        } else {
            clipType |= ClipState::StencilClip;

            const QSGGeometry *g = clip->geometry();
            Q_ASSERT(g->attributeCount() > 0);

            const int vertexByteSize = g->sizeOfVertex() * g->vertexCount();
            // the 4 byte alignment may not actually be needed here
            totalVSize = aligned(totalVSize, 4) + vertexByteSize;
            if (g->indexCount()) {
                const int indexByteSize = g->sizeOfIndex() * g->indexCount();
                // so no need to worry about NonFourAlignedEffectiveIndexBufferOffset
                totalISize = aligned(totalISize, 4) + indexByteSize;
            }
            // ubuf start offsets must be aligned (typically to 256 bytes)
            totalUSize = aligned(totalUSize, m_ubufAlignment) + StencilClipUbufSize;

            stencilClipNodes.append(clip);
        }

        clip = clip->clipList();
    }

    if (clipType & ClipState::StencilClip) {
        bool rebuildVBuf = false;
        if (!batch->stencilClipState.vbuf) {
            batch->stencilClipState.vbuf = m_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::VertexBuffer, totalVSize);
            rebuildVBuf = true;
        } else if (batch->stencilClipState.vbuf->size() < totalVSize) {
            batch->stencilClipState.vbuf->setSize(totalVSize);
            rebuildVBuf = true;
        }
        if (rebuildVBuf) {
            if (!batch->stencilClipState.vbuf->build()) {
                qWarning("Failed to build stencil clip vertex buffer");
                delete batch->stencilClipState.vbuf;
                batch->stencilClipState.vbuf = nullptr;
                return;
            }
        }

        if (totalISize) {
            bool rebuildIBuf = false;
            if (!batch->stencilClipState.ibuf) {
                batch->stencilClipState.ibuf = m_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::IndexBuffer, totalISize);
                rebuildIBuf = true;
            } else if (batch->stencilClipState.ibuf->size() < totalISize) {
                batch->stencilClipState.ibuf->setSize(totalISize);
                rebuildIBuf = true;
            }
            if (rebuildIBuf) {
                if (!batch->stencilClipState.ibuf->build()) {
                    qWarning("Failed to build stencil clip index buffer");
                    delete batch->stencilClipState.ibuf;
                    batch->stencilClipState.ibuf = nullptr;
                    return;
                }
            }
        }

        bool rebuildUBuf = false;
        if (!batch->stencilClipState.ubuf) {
            batch->stencilClipState.ubuf = m_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, totalUSize);
            rebuildUBuf = true;
        } else if (batch->stencilClipState.ubuf->size() < totalUSize) {
            batch->stencilClipState.ubuf->setSize(totalUSize);
            rebuildUBuf = true;
        }
        if (rebuildUBuf) {
            if (!batch->stencilClipState.ubuf->build()) {
                qWarning("Failed to build stencil clip uniform buffer");
                delete batch->stencilClipState.ubuf;
                batch->stencilClipState.ubuf = nullptr;
                return;
            }
        }

        if (!batch->stencilClipState.srb) {
            batch->stencilClipState.srb = m_rhi->newShaderResourceBindings();
            const QRhiShaderResourceBinding ubufBinding = QRhiShaderResourceBinding::uniformBufferWithDynamicOffset(
                        0, QRhiShaderResourceBinding::VertexStage, batch->stencilClipState.ubuf, StencilClipUbufSize);
            batch->stencilClipState.srb->setBindings({ ubufBinding });
            if (!batch->stencilClipState.srb->build()) {
                qWarning("Failed to build stencil clip srb");
                delete batch->stencilClipState.srb;
                batch->stencilClipState.srb = nullptr;
                return;
            }
        }

        int vOffset = 0;
        int iOffset = 0;
        int uOffset = 0;
        for (const QSGClipNode *clip : stencilClipNodes) {
            const QSGGeometry *g = clip->geometry();
            const QSGGeometry::Attribute *a = g->attributes();
            StencilClipState::StencilDrawCall drawCall;
            const bool firstStencilClipInBatch = batch->stencilClipState.drawCalls.isEmpty();

            if (firstStencilClipInBatch) {
                m_stencilClipCommon.inputLayout.setBindings({ QRhiVertexInputBinding(g->sizeOfVertex()) });
                m_stencilClipCommon.inputLayout.setAttributes({ QRhiVertexInputAttribute(0, 0, qsg_vertexInputFormat(*a), 0) });
                m_stencilClipCommon.topology = qsg_topology(g->drawingMode());
            }
#ifndef QT_NO_DEBUG
            else {
                if (qsg_topology(g->drawingMode()) != m_stencilClipCommon.topology)
                    qWarning("updateClipState: Clip list entries have different primitive topologies, this is not currently supported.");
                if (qsg_vertexInputFormat(*a) != m_stencilClipCommon.inputLayout.cbeginAttributes()->format())
                    qWarning("updateClipState: Clip list entries have different vertex input layouts, this is must not happen.");
            }
#endif

            drawCall.vbufOffset = aligned(vOffset, 4);
            const int vertexByteSize = g->sizeOfVertex() * g->vertexCount();
            vOffset = drawCall.vbufOffset + vertexByteSize;

            int indexByteSize = 0;
            if (g->indexCount()) {
                drawCall.ibufOffset = aligned(iOffset, 4);
                indexByteSize = g->sizeOfIndex() * g->indexCount();
                iOffset = drawCall.ibufOffset + indexByteSize;
            }

            drawCall.ubufOffset = aligned(uOffset, m_ubufAlignment);
            uOffset = drawCall.ubufOffset + StencilClipUbufSize;

            QMatrix4x4 matrixYUpNDC = m_current_projection_matrix;
            if (clip->matrix())
                matrixYUpNDC *= *clip->matrix();

            m_resourceUpdates->updateDynamicBuffer(batch->stencilClipState.ubuf, drawCall.ubufOffset, 64, matrixYUpNDC.constData());
            m_resourceUpdates->updateDynamicBuffer(batch->stencilClipState.vbuf, drawCall.vbufOffset, vertexByteSize, g->vertexData());
            if (indexByteSize)
                m_resourceUpdates->updateDynamicBuffer(batch->stencilClipState.ibuf, drawCall.ibufOffset, indexByteSize, g->indexData());

            // stencil ref goes 1, 1, 2, 3, 4, ..., N for the clips in the first batch,
            // then N+1, N+1, N+2, N+3, ... for the next batch,
            // and so on.
            // Note the different stencilOp for the first and the subsequent clips.
            drawCall.stencilRef = firstStencilClipInBatch ? m_currentClipState.stencilRef + 1 : m_currentClipState.stencilRef;
            m_currentClipState.stencilRef += 1;

            drawCall.vertexCount = g->vertexCount();
            drawCall.indexCount = g->indexCount();
            drawCall.indexFormat = qsg_indexFormat(g);
            batch->stencilClipState.drawCalls.add(drawCall);
        }

        if (!m_stencilClipCommon.vs.isValid())
            m_stencilClipCommon.vs = QSGMaterialRhiShaderPrivate::loadShader(QLatin1String(":/qt-project.org/scenegraph/shaders_ng/stencilclip.vert.qsb"));

        if (!m_stencilClipCommon.fs.isValid())
            m_stencilClipCommon.fs = QSGMaterialRhiShaderPrivate::loadShader(QLatin1String(":/qt-project.org/scenegraph/shaders_ng/stencilclip.frag.qsb"));

        if (!m_stencilClipCommon.replacePs)
            m_stencilClipCommon.replacePs = buildStencilPipeline(batch, true);

        if (!m_stencilClipCommon.incrPs)
            m_stencilClipCommon.incrPs = buildStencilPipeline(batch, false);

        batch->stencilClipState.updateStencilBuffer = true;
    }

    m_currentClipState.clipList = clipList;
    m_currentClipState.type = clipType;
    m_currentClipState.scissor = QRhiScissor(scissorRect.x(), scissorRect.y(),
                                             scissorRect.width(), scissorRect.height());

    applyClipStateToGraphicsState();
    batch->clipState = m_currentClipState;
}

void Renderer::enqueueStencilDraw(const Batch *batch) // RHI only
{
    // cliptype stencil + updateStencilBuffer==false means the batch uses
    // stenciling but relies on the stencil data generated by a previous batch
    // (due to the having the same clip node). Do not enqueue draw calls for
    // stencil in this case as the stencil buffer is already up-to-date.
    if (!batch->stencilClipState.updateStencilBuffer)
        return;

    QRhiCommandBuffer *cb = commandBuffer();
    const int count = batch->stencilClipState.drawCalls.size();
    for (int i = 0; i < count; ++i) {
        const StencilClipState::StencilDrawCall &drawCall(batch->stencilClipState.drawCalls.at(i));
        QRhiShaderResourceBindings *srb = batch->stencilClipState.srb;
        QRhiCommandBuffer::DynamicOffset ubufOffset(0, drawCall.ubufOffset);
        if (i == 0) {
            cb->setGraphicsPipeline(m_stencilClipCommon.replacePs);
            cb->setViewport(m_pstate.viewport);
        } else if (i == 1) {
            cb->setGraphicsPipeline(m_stencilClipCommon.incrPs);
            cb->setViewport(m_pstate.viewport);
        }
        // else incrPs is already bound
        cb->setShaderResources(srb, 1, &ubufOffset);
        cb->setStencilRef(drawCall.stencilRef);
        const QRhiCommandBuffer::VertexInput vbufBinding(batch->stencilClipState.vbuf, drawCall.vbufOffset);
        if (drawCall.indexCount) {
            cb->setVertexInput(0, 1, &vbufBinding,
                               batch->stencilClipState.ibuf, drawCall.ibufOffset, drawCall.indexFormat);
            cb->drawIndexed(drawCall.indexCount);
        } else {
            cb->setVertexInput(0, 1, &vbufBinding);
            cb->draw(drawCall.vertexCount);
        }
    }
}

void Renderer::setActiveRhiShader(QSGMaterialRhiShader *program, ShaderManager::Shader *shader) // RHI only
{
    Q_ASSERT(m_rhi);
    m_currentRhiProgram = program;
    m_currentShader = shader;
    m_currentMaterial = nullptr;
}

void Renderer::updateLineWidth(QSGGeometry *g) // legacy (GL-only)
{
    if (g->drawingMode() == GL_LINE_STRIP || g->drawingMode() == GL_LINE_LOOP || g->drawingMode() == GL_LINES)
        glLineWidth(g->lineWidth());
#if !defined(QT_OPENGL_ES_2)
    else {
        QOpenGLContext *ctx = m_context->openglContext();
        if (!ctx->isOpenGLES() && g->drawingMode() == GL_POINTS) {
            QOpenGLFunctions_1_0 *gl1funcs = nullptr;
            QOpenGLFunctions_3_2_Core *gl3funcs = nullptr;
            if (ctx->format().profile() == QSurfaceFormat::CoreProfile)
                gl3funcs = ctx->versionFunctions<QOpenGLFunctions_3_2_Core>();
            else
                gl1funcs = ctx->versionFunctions<QOpenGLFunctions_1_0>();
            Q_ASSERT(gl1funcs || gl3funcs);
            if (gl1funcs)
                gl1funcs->glPointSize(g->lineWidth());
            else
                gl3funcs->glPointSize(g->lineWidth());
        }
    }
#endif
}

void Renderer::renderMergedBatch(const Batch *batch) // legacy (GL-only)
{
    if (batch->vertexCount == 0 || batch->indexCount == 0)
        return;

    Element *e = batch->first;
    Q_ASSERT(e);

#ifndef QT_NO_DEBUG_OUTPUT
    if (Q_UNLIKELY(debug_render())) {
        QDebug debug = qDebug();
        debug << " -"
              << batch
              << (batch->uploadedThisFrame ? "[  upload]" : "[retained]")
              << (e->node->clipList() ? "[  clip]" : "[noclip]")
              << (batch->isOpaque ? "[opaque]" : "[ alpha]")
              << "[  merged]"
              << " Nodes:" << QString::fromLatin1("%1").arg(qsg_countNodesInBatch(batch), 4).toLatin1().constData()
              << " Vertices:" << QString::fromLatin1("%1").arg(batch->vertexCount, 5).toLatin1().constData()
              << " Indices:" << QString::fromLatin1("%1").arg(batch->indexCount, 5).toLatin1().constData()
              << " root:" << batch->root;
        if (batch->drawSets.size() > 1)
            debug << "sets:" << batch->drawSets.size();
        if (!batch->isOpaque)
            debug << "opacity:" << e->node->inheritedOpacity();
        batch->uploadedThisFrame = false;
    }
#endif

    QSGGeometryNode *gn = e->node;

    // We always have dirty matrix as all batches are at a unique z range.
    QSGMaterialShader::RenderState::DirtyStates dirty = QSGMaterialShader::RenderState::DirtyMatrix;
    if (batch->root)
        m_current_model_view_matrix = qsg_matrixForRoot(batch->root);
    else
        m_current_model_view_matrix.setToIdentity();
    m_current_determinant = m_current_model_view_matrix.determinant();
    m_current_projection_matrix = projectionMatrix(); // has potentially been changed by renderUnmergedBatch..

    // updateClip() uses m_current_projection_matrix.
    updateClip(gn->clipList(), batch);

    glBindBuffer(GL_ARRAY_BUFFER, batch->vbo.id);

    char *indexBase = nullptr;
    const Buffer *indexBuf = m_context->separateIndexBuffer() ? &batch->ibo : &batch->vbo;
    if (m_context->hasBrokenIndexBufferObjects()) {
        indexBase = indexBuf->data;
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    } else {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuf->id);
    }


    QSGMaterial *material = gn->activeMaterial();
    ShaderManager::Shader *sms = m_useDepthBuffer ? m_shaderManager->prepareMaterial(material)
                                                  : m_shaderManager->prepareMaterialNoRewrite(material);
    if (!sms)
        return;

    Q_ASSERT(sms->programGL.program);
    if (m_currentShader != sms)
        setActiveShader(sms->programGL.program, sms);

    m_current_opacity = gn->inheritedOpacity();
    if (!qFuzzyCompare(sms->lastOpacity, float(m_current_opacity))) {
        dirty |= QSGMaterialShader::RenderState::DirtyOpacity;
        sms->lastOpacity = m_current_opacity;
    }

    sms->programGL.program->updateState(state(dirty), material, m_currentMaterial);

#ifndef QT_NO_DEBUG
    if (qsg_test_and_clear_material_failure()) {
        qDebug("QSGMaterial::updateState triggered an error (merged), batch will be skipped:");
        Element *ee = e;
        while (ee) {
            qDebug() << "   -" << ee->node;
            ee = ee->nextInBatch;
        }
        QSGNodeDumper::dump(rootNode());
        qFatal("Aborting: scene graph is invalid...");
    }
#endif

    m_currentMaterial = material;

    QSGGeometry *g = gn->geometry();
    updateLineWidth(g);
    char const *const *attrNames = sms->programGL.program->attributeNames();
    for (int i=0; i<batch->drawSets.size(); ++i) {
        const DrawSet &draw = batch->drawSets.at(i);
        int offset = 0;
        for (int j = 0; attrNames[j]; ++j) {
            if (!*attrNames[j])
                continue;
            const QSGGeometry::Attribute &a = g->attributes()[j];
            GLboolean normalize = a.type != GL_FLOAT && a.type != GL_DOUBLE;
            glVertexAttribPointer(a.position, a.tupleSize, a.type, normalize, g->sizeOfVertex(), (void *) (qintptr) (offset + draw.vertices));
            offset += a.tupleSize * size_of_type(a.type);
        }
        if (m_useDepthBuffer)
            glVertexAttribPointer(sms->programGL.pos_order, 1, GL_FLOAT, false, 0, (void *) (qintptr) (draw.zorders));

        glDrawElements(g->drawingMode(), draw.indexCount, GL_UNSIGNED_SHORT, (void *) (qintptr) (indexBase + draw.indices));
    }
}

void Renderer::renderUnmergedBatch(const Batch *batch) // legacy (GL-only)
{
    if (batch->vertexCount == 0)
        return;

    Element *e = batch->first;
    Q_ASSERT(e);

    if (Q_UNLIKELY(debug_render())) {
        qDebug() << " -"
                 << batch
                 << (batch->uploadedThisFrame ? "[  upload]" : "[retained]")
                 << (e->node->clipList() ? "[  clip]" : "[noclip]")
                 << (batch->isOpaque ? "[opaque]" : "[ alpha]")
                 << "[unmerged]"
                 << " Nodes:" << QString::fromLatin1("%1").arg(qsg_countNodesInBatch(batch), 4).toLatin1().constData()
                 << " Vertices:" << QString::fromLatin1("%1").arg(batch->vertexCount, 5).toLatin1().constData()
                 << " Indices:" << QString::fromLatin1("%1").arg(batch->indexCount, 5).toLatin1().constData()
                 << " root:" << batch->root;

        batch->uploadedThisFrame = false;
    }

    QSGGeometryNode *gn = e->node;

    m_current_projection_matrix = projectionMatrix();
    updateClip(gn->clipList(), batch);

    glBindBuffer(GL_ARRAY_BUFFER, batch->vbo.id);
    char *indexBase = nullptr;
    const bool separateIndexBuffer = m_context->separateIndexBuffer();
    const Buffer *indexBuf = separateIndexBuffer ? &batch->ibo : &batch->vbo;
    if (batch->indexCount) {
        if (m_context->hasBrokenIndexBufferObjects()) {
            indexBase = indexBuf->data;
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        } else {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuf->id);
        }
    }

    // We always have dirty matrix as all batches are at a unique z range.
    QSGMaterialShader::RenderState::DirtyStates dirty = QSGMaterialShader::RenderState::DirtyMatrix;

    QSGMaterial *material = gn->activeMaterial();
    ShaderManager::Shader *sms = m_shaderManager->prepareMaterialNoRewrite(material);
    if (!sms)
        return;

    Q_ASSERT(sms->programGL.program);
    if (m_currentShader != sms)
        setActiveShader(sms->programGL.program, sms);

    m_current_opacity = gn->inheritedOpacity();
    if (sms->lastOpacity != m_current_opacity) {
        dirty |= QSGMaterialShader::RenderState::DirtyOpacity;
        sms->lastOpacity = m_current_opacity;
    }

    int vOffset = 0;
    char *iOffset = indexBase;
    if (!separateIndexBuffer)
        iOffset += batch->vertexCount * gn->geometry()->sizeOfVertex();

    QMatrix4x4 rootMatrix = batch->root ? qsg_matrixForRoot(batch->root) : QMatrix4x4();

    while (e) {
        gn = e->node;

        m_current_model_view_matrix = rootMatrix * *gn->matrix();
        m_current_determinant = m_current_model_view_matrix.determinant();

        m_current_projection_matrix = projectionMatrix();
        if (m_useDepthBuffer) {
            m_current_projection_matrix(2, 2) = m_zRange;
            m_current_projection_matrix(2, 3) = 1.0f - e->order * m_zRange;
        }

        sms->programGL.program->updateState(state(dirty), material, m_currentMaterial);

#ifndef QT_NO_DEBUG
    if (qsg_test_and_clear_material_failure()) {
        qDebug("QSGMaterial::updateState() triggered an error (unmerged), batch will be skipped:");
        qDebug() << "   - offending node is" << e->node;
        QSGNodeDumper::dump(rootNode());
        qFatal("Aborting: scene graph is invalid...");
        return;
    }
#endif

        // We don't need to bother with asking each node for its material as they
        // are all identical (compare==0) since they are in the same batch.
        m_currentMaterial = material;

        QSGGeometry *g = gn->geometry();
        char const *const *attrNames = sms->programGL.program->attributeNames();
        int offset = 0;
        for (int j = 0; attrNames[j]; ++j) {
            if (!*attrNames[j])
                continue;
            const QSGGeometry::Attribute &a = g->attributes()[j];
            GLboolean normalize = a.type != GL_FLOAT && a.type != GL_DOUBLE;
            glVertexAttribPointer(a.position, a.tupleSize, a.type, normalize, g->sizeOfVertex(), (void *) (qintptr) (offset + vOffset));
            offset += a.tupleSize * size_of_type(a.type);
        }

        updateLineWidth(g);
        if (g->indexCount())
            glDrawElements(g->drawingMode(), g->indexCount(), g->indexType(), iOffset);
        else
            glDrawArrays(g->drawingMode(), 0, g->vertexCount());

        vOffset += g->sizeOfVertex() * g->vertexCount();
        iOffset += g->indexCount() * g->sizeOfIndex();

        // We only need to push this on the very first iteration...
        dirty &= ~QSGMaterialShader::RenderState::DirtyOpacity;

        e = e->nextInBatch;
    }
}

static inline bool needsBlendConstant(QRhiGraphicsPipeline::BlendFactor f)
{
    return f == QRhiGraphicsPipeline::ConstantColor
            || f == QRhiGraphicsPipeline::OneMinusConstantColor
            || f == QRhiGraphicsPipeline::ConstantAlpha
            || f == QRhiGraphicsPipeline::OneMinusConstantAlpha;
}

// With QRhi renderBatches() is split to two steps: prepare and render.
//
// Prepare goes through the batches and elements, and set up a graphics
// pipeline, srb, uniform buffer, calculates clipping, based on m_gstate, the
// material (shaders), and the batches. This step does not touch the command
// buffer or renderpass-related state (m_pstate).
//
// The render step then starts a renderpass, and goes through all
// batches/elements again and records setGraphicsPipeline, drawIndexed, etc. on
// the command buffer. The prepare step's accumulated global state like
// m_gstate must not be used here. Rather, all data needed for rendering is
// available from Batch/Element at this stage. Bookkeeping of state in the
// renderpass is done via m_pstate.

bool Renderer::ensurePipelineState(Element *e, const ShaderManager::Shader *sms) // RHI only, [prepare step]
{
    // In unmerged batches the srbs in the elements are all compatible
    // layout-wise. Note the key's == and qHash implementations: the rp desc and
    // srb are tested for (layout) compatibility, not pointer equality.
    const GraphicsPipelineStateKey k { m_gstate, sms, renderPassDescriptor(), e->srb };

    // Note: dynamic state (viewport rect, scissor rect, stencil ref, blend
    // constant) is never a part of GraphicsState/QRhiGraphicsPipeline.

    // See if there is an existing, matching pipeline state object.
    auto it = m_shaderManager->pipelineCache.constFind(k);
    if (it != m_shaderManager->pipelineCache.constEnd()) {
        e->ps = *it;
        return true;
    }

    // Build a new one. This is potentially expensive.
    QRhiGraphicsPipeline *ps = m_rhi->newGraphicsPipeline();
    ps->setShaderStages(sms->programRhi.shaderStages.cbegin(), sms->programRhi.shaderStages.cend());
    ps->setVertexInputLayout(sms->programRhi.inputLayout);
    ps->setShaderResourceBindings(e->srb);
    ps->setRenderPassDescriptor(renderPassDescriptor());

    QRhiGraphicsPipeline::Flags flags;
    if (needsBlendConstant(m_gstate.srcColor) || needsBlendConstant(m_gstate.dstColor))
        flags |= QRhiGraphicsPipeline::UsesBlendConstants;
    if (m_gstate.usesScissor)
        flags |= QRhiGraphicsPipeline::UsesScissor;
    if (m_gstate.stencilTest)
        flags |= QRhiGraphicsPipeline::UsesStencilRef;

    ps->setFlags(flags);
    ps->setTopology(qsg_topology(m_gstate.drawMode));
    ps->setCullMode(m_gstate.cullMode);

    QRhiGraphicsPipeline::TargetBlend blend;
    blend.colorWrite = m_gstate.colorWrite;
    blend.enable = m_gstate.blending;
    blend.srcColor = m_gstate.srcColor;
    blend.dstColor = m_gstate.dstColor;
    ps->setTargetBlends({ blend });

    ps->setDepthTest(m_gstate.depthTest);
    ps->setDepthWrite(m_gstate.depthWrite);
    ps->setDepthOp(m_gstate.depthFunc);

    if (m_gstate.stencilTest) {
        ps->setStencilTest(true);
        QRhiGraphicsPipeline::StencilOpState stencilOp;
        stencilOp.compareOp = QRhiGraphicsPipeline::Equal;
        stencilOp.failOp = QRhiGraphicsPipeline::Keep;
        stencilOp.depthFailOp = QRhiGraphicsPipeline::Keep;
        stencilOp.passOp = QRhiGraphicsPipeline::Keep;
        ps->setStencilFront(stencilOp);
        ps->setStencilBack(stencilOp);
    }

    ps->setSampleCount(m_gstate.sampleCount);

    ps->setLineWidth(m_gstate.lineWidth);

    //qDebug("building new ps %p", ps);
    if (!ps->build()) {
        qWarning("Failed to build graphics pipeline state");
        delete ps;
        return false;
    }

    m_shaderManager->pipelineCache.insert(k, ps);
    e->ps = ps;
    return true;
}

static QRhiSampler *newSampler(QRhi *rhi, const QSGSamplerDescription &desc)
{
    QRhiSampler::Filter magFilter;
    QRhiSampler::Filter minFilter;
    QRhiSampler::Filter mipmapMode;
    QRhiSampler::AddressMode u;
    QRhiSampler::AddressMode v;

    switch (desc.filtering) {
    case QSGTexture::None:
        Q_FALLTHROUGH();
    case QSGTexture::Nearest:
        magFilter = minFilter = QRhiSampler::Nearest;
        break;
    case QSGTexture::Linear:
        magFilter = minFilter = QRhiSampler::Linear;
        break;
    default:
        Q_UNREACHABLE();
        magFilter = minFilter = QRhiSampler::Nearest;
        break;
    }

    switch (desc.mipmapFiltering) {
    case QSGTexture::None:
        mipmapMode = QRhiSampler::None;
        break;
    case QSGTexture::Nearest:
        mipmapMode = QRhiSampler::Nearest;
        break;
    case QSGTexture::Linear:
        mipmapMode = QRhiSampler::Linear;
        break;
    default:
        Q_UNREACHABLE();
        mipmapMode = QRhiSampler::None;
        break;
    }

    switch (desc.horizontalWrap) {
    case QSGTexture::Repeat:
        u = QRhiSampler::Repeat;
        break;
    case QSGTexture::ClampToEdge:
        u = QRhiSampler::ClampToEdge;
        break;
    case QSGTexture::MirroredRepeat:
        u = QRhiSampler::Mirror;
        break;
    default:
        Q_UNREACHABLE();
        u = QRhiSampler::ClampToEdge;
        break;
    }

    switch (desc.verticalWrap) {
    case QSGTexture::Repeat:
        v = QRhiSampler::Repeat;
        break;
    case QSGTexture::ClampToEdge:
        v = QRhiSampler::ClampToEdge;
        break;
    case QSGTexture::MirroredRepeat:
        v = QRhiSampler::Mirror;
        break;
    default:
        Q_UNREACHABLE();
        v = QRhiSampler::ClampToEdge;
        break;
    }

    return rhi->newSampler(magFilter, minFilter, mipmapMode, u, v);
}

QRhiTexture *Renderer::dummyTexture()
{
    if (!m_dummyTexture) {
        m_dummyTexture = m_rhi->newTexture(QRhiTexture::RGBA8, QSize(64, 64));
        if (m_dummyTexture->build()) {
            if (m_resourceUpdates) {
                QImage img(m_dummyTexture->pixelSize(), QImage::Format_RGBA8888_Premultiplied);
                img.fill(0);
                m_resourceUpdates->uploadTexture(m_dummyTexture, img);
            }
        }
    }
    return m_dummyTexture;
}

static void rendererToMaterialGraphicsState(QSGMaterialRhiShader::GraphicsPipelineState *dst,
                                            GraphicsState *src)
{
    dst->blendEnable = src->blending;

    // the enum values should match, sanity check it
    Q_ASSERT(int(QSGMaterialRhiShader::GraphicsPipelineState::OneMinusSrc1Alpha) == int(QRhiGraphicsPipeline::OneMinusSrc1Alpha));
    Q_ASSERT(int(QSGMaterialRhiShader::GraphicsPipelineState::A) == int(QRhiGraphicsPipeline::A));
    Q_ASSERT(int(QSGMaterialRhiShader::GraphicsPipelineState::CullBack) == int(QRhiGraphicsPipeline::Back));

    dst->srcColor = QSGMaterialRhiShader::GraphicsPipelineState::BlendFactor(src->srcColor);
    dst->dstColor = QSGMaterialRhiShader::GraphicsPipelineState::BlendFactor(src->dstColor);

    dst->colorWrite = QSGMaterialRhiShader::GraphicsPipelineState::ColorMask(int(src->colorWrite));

    dst->cullMode = QSGMaterialRhiShader::GraphicsPipelineState::CullMode(src->cullMode);
}

static void materialToRendererGraphicsState(GraphicsState *dst,
                                            QSGMaterialRhiShader::GraphicsPipelineState *src)
{
    dst->blending = src->blendEnable;
    dst->srcColor = QRhiGraphicsPipeline::BlendFactor(src->srcColor);
    dst->dstColor = QRhiGraphicsPipeline::BlendFactor(src->dstColor);
    dst->colorWrite = QRhiGraphicsPipeline::ColorMask(int(src->colorWrite));
    dst->cullMode = QRhiGraphicsPipeline::CullMode(src->cullMode);
}

void Renderer::updateMaterialDynamicData(ShaderManager::Shader *sms,
                                         QSGMaterialRhiShader::RenderState &renderState,
                                         QSGMaterial *material,
                                         ShaderManager::ShaderResourceBindingList *bindings,
                                         const Batch *batch,
                                         int ubufOffset,
                                         int ubufRegionSize) // RHI only, [prepare step]
{
    m_current_resource_update_batch = m_resourceUpdates;

    QSGMaterialRhiShader *shader = sms->programRhi.program;
    QSGMaterialRhiShaderPrivate *pd = QSGMaterialRhiShaderPrivate::get(shader);
    if (pd->ubufBinding >= 0) {
        m_current_uniform_data = &pd->masterUniformData;
        const bool changed = shader->updateUniformData(renderState, material, m_currentMaterial);
        m_current_uniform_data = nullptr;

        if (changed || !batch->ubufDataValid)
            m_resourceUpdates->updateDynamicBuffer(batch->ubuf, ubufOffset, ubufRegionSize, pd->masterUniformData.constData());

        bindings->append(QRhiShaderResourceBinding::uniformBuffer(pd->ubufBinding,
                                                                  pd->ubufStages,
                                                                  batch->ubuf,
                                                                  ubufOffset,
                                                                  ubufRegionSize));
    }

    for (int binding = 0; binding < QSGMaterialRhiShaderPrivate::MAX_SHADER_RESOURCE_BINDINGS; ++binding) {
        const QRhiShaderResourceBinding::StageFlags stages = pd->combinedImageSamplerBindings[binding];
        if (!stages)
            continue;

        QSGTexture *prevTex = pd->textureBindingTable[binding];
        QSGTexture *t = prevTex;

        shader->updateSampledImage(renderState, binding, &t, material, m_currentMaterial);
        if (!t) {
            qWarning("No QSGTexture provided from updateSampledImage(). This is wrong.");
            continue;
        }

        QSGTexturePrivate *td = QSGTexturePrivate::get(t);
        // prevTex may be invalid at this point, avoid dereferencing it
        if (t != prevTex || td->hasDirtySamplerOptions()) {
            // The QSGTexture, and so the sampler parameters, may have changed.
            // The rhiTexture is not relevant here.
            td->resetDirtySamplerOptions();
            pd->textureBindingTable[binding] = t; // does not own
            pd->samplerBindingTable[binding] = nullptr;
            if (t->anisotropyLevel() != QSGTexture::AnisotropyNone) // ###
                qWarning("QSGTexture anisotropy levels are not currently supported");

            const QSGSamplerDescription samplerDesc = QSGSamplerDescription::fromTexture(t);
            QRhiSampler *sampler = nullptr;
            auto it = m_samplers.constFind(samplerDesc);
            if (it != m_samplers.constEnd()) {
                sampler = *it;
                Q_ASSERT(sampler);
            } else {
                sampler = newSampler(m_rhi, samplerDesc);
                if (!sampler->build()) {
                    qWarning("Failed to build sampler");
                    delete sampler;
                    continue;
                }
                m_samplers.insert(samplerDesc, sampler);
            }
            pd->samplerBindingTable[binding] = sampler; // does not own
        }

        if (pd->textureBindingTable[binding] && pd->samplerBindingTable[binding]) {
            QRhiTexture *texture = QSGTexturePrivate::get(pd->textureBindingTable[binding])->rhiTexture();
            // texture may be null if the update above failed for any reason,
            // or if the QSGTexture chose to return null intentionally. This is
            // valid and we still need to provide something to the shader.
            if (!texture)
                texture = dummyTexture();
            QRhiSampler *sampler = pd->samplerBindingTable[binding];
            bindings->append(QRhiShaderResourceBinding::sampledTexture(binding,
                                                                       stages,
                                                                       texture,
                                                                       sampler));
        }
    }

#ifndef QT_NO_DEBUG
    if (bindings->isEmpty())
        qWarning("No shader resources for material %p, this is odd.", material);
#endif
}

void Renderer::updateMaterialStaticData(ShaderManager::Shader *sms,
                                        QSGMaterialRhiShader::RenderState &renderState,
                                        QSGMaterial *material,
                                        Batch *batch,
                                        bool *gstateChanged) // RHI only, [prepare step]
{
    QSGMaterialRhiShader *shader = sms->programRhi.program;
    *gstateChanged = false;
    if (shader->flags().testFlag(QSGMaterialRhiShader::UpdatesGraphicsPipelineState)) {
        // generate the public mini-state from m_gstate, invoke the material,
        // write the changes, if any, back to m_gstate, together with a way to
        // roll those back.
        QSGMaterialRhiShader::GraphicsPipelineState shaderPs;
        rendererToMaterialGraphicsState(&shaderPs, &m_gstate);
        const bool changed = shader->updateGraphicsPipelineState(renderState, &shaderPs, material, m_currentMaterial);
        if (changed) {
            m_gstateStack.push(m_gstate);
            materialToRendererGraphicsState(&m_gstate, &shaderPs);
            if (needsBlendConstant(m_gstate.srcColor) || needsBlendConstant(m_gstate.dstColor))
                batch->blendConstant = shaderPs.blendConstant;
            *gstateChanged = true;
        }
    }
}

bool Renderer::prepareRenderMergedBatch(Batch *batch, PreparedRenderBatch *renderBatch) // split prepare-render (RHI only)
{
    if (batch->vertexCount == 0 || batch->indexCount == 0)
        return false;

    Element *e = batch->first;
    Q_ASSERT(e);

#ifndef QT_NO_DEBUG_OUTPUT
    if (Q_UNLIKELY(debug_render())) {
        QDebug debug = qDebug();
        debug << " -"
              << batch
              << (batch->uploadedThisFrame ? "[  upload]" : "[retained]")
              << (e->node->clipList() ? "[  clip]" : "[noclip]")
              << (batch->isOpaque ? "[opaque]" : "[ alpha]")
              << "[  merged]"
              << " Nodes:" << QString::fromLatin1("%1").arg(qsg_countNodesInBatch(batch), 4).toLatin1().constData()
              << " Vertices:" << QString::fromLatin1("%1").arg(batch->vertexCount, 5).toLatin1().constData()
              << " Indices:" << QString::fromLatin1("%1").arg(batch->indexCount, 5).toLatin1().constData()
              << " root:" << batch->root;
        if (batch->drawSets.size() > 1)
            debug << "sets:" << batch->drawSets.size();
        if (!batch->isOpaque)
            debug << "opacity:" << e->node->inheritedOpacity();
        batch->uploadedThisFrame = false;
    }
#endif

    QSGGeometryNode *gn = e->node;

    // We always have dirty matrix as all batches are at a unique z range.
    QSGMaterialShader::RenderState::DirtyStates dirty = QSGMaterialShader::RenderState::DirtyMatrix;
    if (batch->root)
        m_current_model_view_matrix = qsg_matrixForRoot(batch->root);
    else
        m_current_model_view_matrix.setToIdentity();
    m_current_determinant = m_current_model_view_matrix.determinant();
    m_current_projection_matrix = projectionMatrix();
    m_current_projection_matrix_native_ndc = projectionMatrixWithNativeNDC();

    QSGMaterial *material = gn->activeMaterial();
    updateClipState(gn->clipList(), batch);

    const QSGGeometry *g = gn->geometry();
    ShaderManager::Shader *sms = m_useDepthBuffer ? m_shaderManager->prepareMaterial(material, true, g)
                                                  : m_shaderManager->prepareMaterialNoRewrite(material, true, g);
    if (!sms)
        return false;

    Q_ASSERT(sms->programRhi.program);
    if (m_currentShader != sms)
        setActiveRhiShader(sms->programRhi.program, sms);

    m_current_opacity = gn->inheritedOpacity();
    if (!qFuzzyCompare(sms->lastOpacity, float(m_current_opacity))) {
        dirty |= QSGMaterialShader::RenderState::DirtyOpacity;
        sms->lastOpacity = m_current_opacity;
    }

    QSGMaterialRhiShaderPrivate *pd = QSGMaterialRhiShaderPrivate::get(sms->programRhi.program);
    const int ubufSize = pd->masterUniformData.size();
    if (pd->ubufBinding >= 0) {
        bool ubufRebuild = false;
        if (!batch->ubuf) {
            batch->ubuf = m_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, ubufSize);
            ubufRebuild = true;
        } else {
            if (batch->ubuf->size() < ubufSize) {
                batch->ubuf->setSize(ubufSize);
                ubufRebuild = true;
            }
        }
        if (ubufRebuild) {
            batch->ubufDataValid = false;
            if (!batch->ubuf->build()) {
                qWarning("Failed to build uniform buffer of size %d bytes", ubufSize);
                delete batch->ubuf;
                batch->ubuf = nullptr;
                return false;
            }
        }
    }

    QSGMaterialRhiShader::RenderState renderState = rhiState(QSGMaterialRhiShader::RenderState::DirtyStates(int(dirty)));

    bool pendingGStatePop = false;
    updateMaterialStaticData(sms, renderState, material, batch, &pendingGStatePop);

    ShaderManager::ShaderResourceBindingList bindings;
    updateMaterialDynamicData(sms, renderState, material, &bindings, batch, 0, ubufSize);

#ifndef QT_NO_DEBUG
    if (qsg_test_and_clear_material_failure()) {
        qDebug("QSGMaterial::updateState triggered an error (merged), batch will be skipped:");
        Element *ee = e;
        while (ee) {
            qDebug() << "   -" << ee->node;
            ee = ee->nextInBatch;
        }
        QSGNodeDumper::dump(rootNode());
        qFatal("Aborting: scene graph is invalid...");
    }
#endif

    e->srb = m_shaderManager->srb(bindings);

    m_gstate.drawMode = QSGGeometry::DrawingMode(g->drawingMode());
    m_gstate.lineWidth = g->lineWidth();

    const bool hasPipeline = ensurePipelineState(e, sms);

    if (pendingGStatePop)
        m_gstate = m_gstateStack.pop();

    if (!hasPipeline)
        return false;

    batch->ubufDataValid = true;

    m_currentMaterial = material;

    renderBatch->batch = batch;
    renderBatch->sms = sms;

    return true;
}

void Renderer::checkLineWidth(QSGGeometry *g)
{
    if (g->drawingMode() == QSGGeometry::DrawLines || g->drawingMode() == QSGGeometry::DrawLineLoop
            || g->drawingMode() == QSGGeometry::DrawLineStrip)
    {
        if (g->lineWidth() != 1.0f) {
            static bool checkedWideLineSupport = false;
            if (!checkedWideLineSupport) {
                checkedWideLineSupport = true;
                if (!m_rhi->isFeatureSupported(QRhi::WideLines))
                    qWarning("Line widths other than 1 are not supported by the graphics API");
            }
        }
    } else if (g->drawingMode() == QSGGeometry::DrawPoints) {
        if (g->lineWidth() != 1.0f) {
            static bool warnedPointSize = false;
            if (!warnedPointSize) {
                warnedPointSize = true;
                qWarning("Point size is not controllable by QSGGeometry. "
                         "Set gl_PointSize from the vertex shader instead.");
            }
        }
    }
}

void Renderer::renderMergedBatch(PreparedRenderBatch *renderBatch) // split prepare-render (RHI only)
{
    const Batch *batch = renderBatch->batch;
    Element *e = batch->first;
    QSGGeometryNode *gn = e->node;
    QSGGeometry *g = gn->geometry();
    checkLineWidth(g);

    if (batch->clipState.type & ClipState::StencilClip)
        enqueueStencilDraw(batch);

    QRhiCommandBuffer *cb = commandBuffer();
    setGraphicsPipeline(cb, batch, e);

    for (int i = 0, ie = batch->drawSets.size(); i != ie; ++i) {
        const DrawSet &draw = batch->drawSets.at(i);
        const QRhiCommandBuffer::VertexInput vbufBindings[] = {
            { batch->vbo.buf, quint32(draw.vertices) },
            { batch->vbo.buf, quint32(draw.zorders) }
        };
        cb->setVertexInput(VERTEX_BUFFER_BINDING, m_useDepthBuffer ? 2 : 1, vbufBindings,
                           batch->ibo.buf, draw.indices,
                           m_uint32IndexForRhi ? QRhiCommandBuffer::IndexUInt32 : QRhiCommandBuffer::IndexUInt16);
        cb->drawIndexed(draw.indexCount);
    }
}

bool Renderer::prepareRenderUnmergedBatch(Batch *batch, PreparedRenderBatch *renderBatch) // split prepare-render (RHI only)
{
    if (batch->vertexCount == 0)
        return false;

    Element *e = batch->first;
    Q_ASSERT(e);

    if (Q_UNLIKELY(debug_render())) {
        qDebug() << " -"
                 << batch
                 << (batch->uploadedThisFrame ? "[  upload]" : "[retained]")
                 << (e->node->clipList() ? "[  clip]" : "[noclip]")
                 << (batch->isOpaque ? "[opaque]" : "[ alpha]")
                 << "[unmerged]"
                 << " Nodes:" << QString::fromLatin1("%1").arg(qsg_countNodesInBatch(batch), 4).toLatin1().constData()
                 << " Vertices:" << QString::fromLatin1("%1").arg(batch->vertexCount, 5).toLatin1().constData()
                 << " Indices:" << QString::fromLatin1("%1").arg(batch->indexCount, 5).toLatin1().constData()
                 << " root:" << batch->root;

        batch->uploadedThisFrame = false;
    }

    m_current_projection_matrix = projectionMatrix();
    m_current_projection_matrix_native_ndc = projectionMatrixWithNativeNDC();

    QSGGeometryNode *gn = e->node;
    updateClipState(gn->clipList(), batch);

    // We always have dirty matrix as all batches are at a unique z range.
    QSGMaterialShader::RenderState::DirtyStates dirty = QSGMaterialShader::RenderState::DirtyMatrix;

    // The vertex attributes are assumed to be the same for all elements in the
    // unmerged batch since the material (and so the shaders) is the same.
    QSGGeometry *g = gn->geometry();
    QSGMaterial *material = gn->activeMaterial();
    ShaderManager::Shader *sms = m_shaderManager->prepareMaterialNoRewrite(material, m_rhi, g);
    if (!sms)
        return false;

    Q_ASSERT(sms->programRhi.program);
    if (m_currentShader != sms)
        setActiveRhiShader(sms->programRhi.program, sms);

    m_current_opacity = gn->inheritedOpacity();
    if (sms->lastOpacity != m_current_opacity) {
        dirty |= QSGMaterialShader::RenderState::DirtyOpacity;
        sms->lastOpacity = m_current_opacity;
    }

    QMatrix4x4 rootMatrix = batch->root ? qsg_matrixForRoot(batch->root) : QMatrix4x4();

    QSGMaterialRhiShaderPrivate *pd = QSGMaterialRhiShaderPrivate::get(sms->programRhi.program);
    const int ubufSize = pd->masterUniformData.size();
    if (pd->ubufBinding >= 0) {
        int totalUBufSize = 0;
        while (e) {
            totalUBufSize += aligned(ubufSize, m_ubufAlignment);
            e = e->nextInBatch;
        }
        bool ubufRebuild = false;
        if (!batch->ubuf) {
            batch->ubuf = m_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, totalUBufSize);
            ubufRebuild = true;
        } else {
            if (batch->ubuf->size() < totalUBufSize) {
                batch->ubuf->setSize(totalUBufSize);
                ubufRebuild = true;
            }
        }
        if (ubufRebuild) {
            batch->ubufDataValid = false;
            if (!batch->ubuf->build()) {
                qWarning("Failed to build uniform buffer of size %d bytes", totalUBufSize);
                delete batch->ubuf;
                batch->ubuf = nullptr;
                return false;
            }
        }
    }

    QSGMaterialRhiShader::RenderState renderState = rhiState(QSGMaterialRhiShader::RenderState::DirtyStates(int(dirty)));
    bool pendingGStatePop = false;
    updateMaterialStaticData(sms, renderState,
                             material, batch, &pendingGStatePop);

    int ubufOffset = 0;
    QRhiGraphicsPipeline *ps = nullptr;
    e = batch->first;
    while (e) {
        gn = e->node;

        m_current_model_view_matrix = rootMatrix * *gn->matrix();
        m_current_determinant = m_current_model_view_matrix.determinant();

        m_current_projection_matrix = projectionMatrix();
        m_current_projection_matrix_native_ndc = projectionMatrixWithNativeNDC();
        if (m_useDepthBuffer) {
            m_current_projection_matrix(2, 2) = m_zRange;
            m_current_projection_matrix(2, 3) = 1.0f - e->order * m_zRange;
        }

        QSGMaterialRhiShader::RenderState renderState = rhiState(QSGMaterialRhiShader::RenderState::DirtyStates(int(dirty)));
        ShaderManager::ShaderResourceBindingList bindings;
        updateMaterialDynamicData(sms, renderState,
                                  material, &bindings, batch, ubufOffset, ubufSize);

#ifndef QT_NO_DEBUG
        if (qsg_test_and_clear_material_failure()) {
            qDebug("QSGMaterial::updateState() triggered an error (unmerged), batch will be skipped:");
            qDebug() << "   - offending node is" << e->node;
            QSGNodeDumper::dump(rootNode());
            qFatal("Aborting: scene graph is invalid...");
            return false;
        }
#endif

        e->srb = m_shaderManager->srb(bindings);

        ubufOffset += aligned(ubufSize, m_ubufAlignment);

        const QSGGeometry::DrawingMode prevDrawMode = m_gstate.drawMode;
        const float prevLineWidth = m_gstate.lineWidth;
        m_gstate.drawMode = QSGGeometry::DrawingMode(g->drawingMode());
        m_gstate.lineWidth = g->lineWidth();

        // Do not bother even looking up the ps if the topology has not changed
        // since everything else is the same for all elements in the batch.
        // (except if the material modified blend state)
        if (!ps || m_gstate.drawMode != prevDrawMode || m_gstate.lineWidth != prevLineWidth || pendingGStatePop) {
            if (!ensurePipelineState(e, sms)) {
                if (pendingGStatePop)
                    m_gstate = m_gstateStack.pop();
                return false;
            }
            ps = e->ps;
        } else {
            e->ps = ps;
        }

        // We don't need to bother with asking each node for its material as they
        // are all identical (compare==0) since they are in the same batch.
        m_currentMaterial = material;

        // We only need to push this on the very first iteration...
        dirty &= ~QSGMaterialShader::RenderState::DirtyOpacity;

        e = e->nextInBatch;
    }

    if (pendingGStatePop)
        m_gstate = m_gstateStack.pop();

    batch->ubufDataValid = true;

    renderBatch->batch = batch;
    renderBatch->sms = sms;

    return true;
}

void Renderer::renderUnmergedBatch(PreparedRenderBatch *renderBatch) // split prepare-render (RHI only)
{
    const Batch *batch = renderBatch->batch;
    Element *e = batch->first;
    QSGGeometryNode *gn = e->node;

    if (batch->clipState.type & ClipState::StencilClip)
        enqueueStencilDraw(batch);

    int vOffset = 0;
    int iOffset = 0;
    QRhiCommandBuffer *cb = commandBuffer();

    while (e) {
        gn = e->node;
        QSGGeometry *g = gn->geometry();
        checkLineWidth(g);
        const int effectiveIndexSize = m_uint32IndexForRhi ? sizeof(quint32) : g->sizeOfIndex();

        setGraphicsPipeline(cb, batch, e);

        const QRhiCommandBuffer::VertexInput vbufBinding(batch->vbo.buf, vOffset);
        if (g->indexCount()) {
            cb->setVertexInput(VERTEX_BUFFER_BINDING, 1, &vbufBinding,
                               batch->ibo.buf, iOffset,
                               effectiveIndexSize == sizeof(quint32) ? QRhiCommandBuffer::IndexUInt32
                                                                     : QRhiCommandBuffer::IndexUInt16);
            cb->drawIndexed(g->indexCount());
        } else {
            cb->setVertexInput(VERTEX_BUFFER_BINDING, 1, &vbufBinding);
            cb->draw(g->vertexCount());
        }

        vOffset += g->sizeOfVertex() * g->vertexCount();
        iOffset += g->indexCount() * effectiveIndexSize;

        e = e->nextInBatch;
    }
}

void Renderer::setGraphicsPipeline(QRhiCommandBuffer *cb, const Batch *batch, Element *e) // RHI only, [render step]
{
    cb->setGraphicsPipeline(e->ps);

    if (!m_pstate.viewportSet) {
        m_pstate.viewportSet = true;
        cb->setViewport(m_pstate.viewport);
    }
    if (batch->clipState.type & ClipState::ScissorClip) {
        Q_ASSERT(e->ps->flags().testFlag(QRhiGraphicsPipeline::UsesScissor));
        m_pstate.scissorSet = true;
        cb->setScissor(batch->clipState.scissor);
    } else {
        Q_ASSERT(!e->ps->flags().testFlag(QRhiGraphicsPipeline::UsesScissor));
        // Regardless of the ps not using scissor, the scissor may need to be
        // reset, depending on the backend. So set the viewport again, which in
        // turn also sets the scissor on backends where a scissor rect is
        // always-on (Vulkan).
        if (m_pstate.scissorSet) {
            m_pstate.scissorSet = false;
            cb->setViewport(m_pstate.viewport);
        }
    }
    if (batch->clipState.type & ClipState::StencilClip) {
        Q_ASSERT(e->ps->flags().testFlag(QRhiGraphicsPipeline::UsesStencilRef));
        cb->setStencilRef(batch->clipState.stencilRef);
    }
    if (e->ps->flags().testFlag(QRhiGraphicsPipeline::UsesBlendConstants))
        cb->setBlendConstants(batch->blendConstant);

    cb->setShaderResources(e->srb);
}

void Renderer::renderBatches()
{
    if (Q_UNLIKELY(debug_render())) {
        qDebug().nospace() << "Rendering:" << Qt::endl
                           << " -> Opaque: " << qsg_countNodesInBatches(m_opaqueBatches) << " nodes in " << m_opaqueBatches.size() << " batches..." << Qt::endl
                           << " -> Alpha: " << qsg_countNodesInBatches(m_alphaBatches) << " nodes in " << m_alphaBatches.size() << " batches...";
    }

    m_current_opacity = 1;
    m_currentMaterial = nullptr;
    m_currentShader = nullptr;
    m_currentProgram = nullptr;
    m_currentRhiProgram = nullptr;
    m_currentClip = nullptr;
    m_currentClipState.reset();

    const QRect viewport = viewportRect();

    bool renderOpaque = !debug_noopaque();
    bool renderAlpha = !debug_noalpha();

    if (!m_rhi) {
        // legacy, GL-only path

        glViewport(viewport.x(), deviceRect().bottom() - viewport.bottom(), viewport.width(), viewport.height());
        glClearColor(clearColor().redF(), clearColor().greenF(), clearColor().blueF(), clearColor().alphaF());

        if (m_useDepthBuffer) {
            glClearDepthf(1); // calls glClearDepth() under the hood for desktop OpenGL
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_LESS);
            glDepthMask(true);
            glDisable(GL_BLEND);
        } else {
            glDisable(GL_DEPTH_TEST);
            glDepthMask(false);
        }
        glDisable(GL_CULL_FACE);
        glColorMask(true, true, true, true);
        glDisable(GL_SCISSOR_TEST);
        glDisable(GL_STENCIL_TEST);

        bindable()->clear(clearMode());

        if (m_renderPassRecordingCallbacks.start)
            m_renderPassRecordingCallbacks.start(m_renderPassRecordingCallbacks.userData);

        if (Q_LIKELY(renderOpaque)) {
            for (int i=0; i<m_opaqueBatches.size(); ++i) {
                Batch *b = m_opaqueBatches.at(i);
                if (b->merged)
                    renderMergedBatch(b);
                else
                    renderUnmergedBatch(b);
            }
        }

        glEnable(GL_BLEND);
        if (m_useDepthBuffer)
            glDepthMask(false);
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

        if (Q_LIKELY(renderAlpha)) {
            for (int i=0; i<m_alphaBatches.size(); ++i) {
                Batch *b = m_alphaBatches.at(i);
                if (b->merged)
                    renderMergedBatch(b);
                else if (b->isRenderNode)
                    renderRenderNode(b);
                else
                    renderUnmergedBatch(b);
            }
        }

        if (m_currentShader)
            setActiveShader(nullptr, nullptr);

        updateStencilClip(nullptr);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glDepthMask(true);

        if (m_renderPassRecordingCallbacks.end)
            m_renderPassRecordingCallbacks.end(m_renderPassRecordingCallbacks.userData);

    } else {
        // RHI path

        m_pstate.viewport = QRhiViewport(viewport.x(), deviceRect().bottom() - viewport.bottom(), viewport.width(), viewport.height());
        m_pstate.clearColor = clearColor();
        m_pstate.dsClear = QRhiDepthStencilClearValue(1.0f, 0);
        m_pstate.viewportSet = false;
        m_pstate.scissorSet = false;

        m_gstate.depthTest = m_useDepthBuffer;
        m_gstate.depthWrite = m_useDepthBuffer;
        m_gstate.depthFunc = QRhiGraphicsPipeline::Less;
        m_gstate.blending = false;

        m_gstate.cullMode = QRhiGraphicsPipeline::None;
        m_gstate.colorWrite = QRhiGraphicsPipeline::R
                | QRhiGraphicsPipeline::G
                | QRhiGraphicsPipeline::B
                | QRhiGraphicsPipeline::A;
        m_gstate.usesScissor = false;
        m_gstate.stencilTest = false;

        m_gstate.sampleCount = renderTarget()->sampleCount();

        QVarLengthArray<PreparedRenderBatch, 64> opaqueRenderBatches;
        if (Q_LIKELY(renderOpaque)) {
            for (int i = 0, ie = m_opaqueBatches.size(); i != ie; ++i) {
                Batch *b = m_opaqueBatches.at(i);
                PreparedRenderBatch renderBatch;
                bool ok;
                if (b->merged)
                    ok = prepareRenderMergedBatch(b, &renderBatch);
                else
                    ok = prepareRenderUnmergedBatch(b, &renderBatch);
                if (ok)
                    opaqueRenderBatches.append(renderBatch);
            }
        }

        m_gstate.blending = true;
        // factors never change, always set for premultiplied alpha based blending

        // depth test stays enabled (if m_useDepthBuffer, that is) but no need
        // to write out depth from the transparent (back-to-front) pass
        m_gstate.depthWrite = false;

        QVarLengthArray<PreparedRenderBatch, 64> alphaRenderBatches;
        if (Q_LIKELY(renderAlpha)) {
            for (int i = 0, ie = m_alphaBatches.size(); i != ie; ++i) {
                Batch *b = m_alphaBatches.at(i);
                PreparedRenderBatch renderBatch;
                bool ok;
                if (b->merged)
                    ok = prepareRenderMergedBatch(b, &renderBatch);
                else if (b->isRenderNode)
                    ok = prepareRhiRenderNode(b, &renderBatch);
                else
                    ok = prepareRenderUnmergedBatch(b, &renderBatch);
                if (ok)
                    alphaRenderBatches.append(renderBatch);
            }
        }

        if (m_visualizer->mode() != Visualizer::VisualizeNothing)
            m_visualizer->prepareVisualize();

        QRhiCommandBuffer *cb = commandBuffer();
        cb->beginPass(renderTarget(), m_pstate.clearColor, m_pstate.dsClear, m_resourceUpdates);
        m_resourceUpdates = nullptr;

        if (m_renderPassRecordingCallbacks.start)
            m_renderPassRecordingCallbacks.start(m_renderPassRecordingCallbacks.userData);

        for (int i = 0, ie = opaqueRenderBatches.count(); i != ie; ++i) {
            PreparedRenderBatch *renderBatch = &opaqueRenderBatches[i];
            if (renderBatch->batch->merged)
                renderMergedBatch(renderBatch);
            else
                renderUnmergedBatch(renderBatch);
        }

        for (int i = 0, ie = alphaRenderBatches.count(); i != ie; ++i) {
            PreparedRenderBatch *renderBatch = &alphaRenderBatches[i];
            if (renderBatch->batch->merged)
                renderMergedBatch(renderBatch);
            else if (renderBatch->batch->isRenderNode)
                renderRhiRenderNode(renderBatch->batch);
            else
                renderUnmergedBatch(renderBatch);
        }

        if (m_currentShader)
            setActiveRhiShader(nullptr, nullptr);

        if (m_renderPassRecordingCallbacks.end)
            m_renderPassRecordingCallbacks.end(m_renderPassRecordingCallbacks.userData);

        if (m_visualizer->mode() == Visualizer::VisualizeNothing)
            cb->endPass();
    }
}

void Renderer::deleteRemovedElements()
{
    if (!m_elementsToDelete.size())
        return;

    for (int i=0; i<m_opaqueRenderList.size(); ++i) {
        Element **e = m_opaqueRenderList.data() + i;
        if (*e && (*e)->removed)
            *e = nullptr;
    }
    for (int i=0; i<m_alphaRenderList.size(); ++i) {
        Element **e = m_alphaRenderList.data() + i;
        if (*e && (*e)->removed)
            *e = nullptr;
    }

    for (int i=0; i<m_elementsToDelete.size(); ++i) {
        Element *e = m_elementsToDelete.at(i);
        if (e->isRenderNode)
            delete static_cast<RenderNodeElement *>(e);
        else
            m_elementAllocator.release(e);
    }
    m_elementsToDelete.reset();
}

void Renderer::render()
{
    if (Q_UNLIKELY(debug_dump())) {
        qDebug("\n");
        QSGNodeDumper::dump(rootNode());
    }

    QElapsedTimer timer;
    quint64 timeRenderLists = 0;
    quint64 timePrepareOpaque = 0;
    quint64 timePrepareAlpha = 0;
    quint64 timeSorting = 0;
    quint64 timeUploadOpaque = 0;
    quint64 timeUploadAlpha = 0;

    if (Q_UNLIKELY(debug_render() || debug_build())) {
        QByteArray type("rebuild:");
        if (m_rebuild == 0)
            type += " none";
        if (m_rebuild == FullRebuild)
            type += " full";
        else {
            if (m_rebuild & BuildRenderLists)
                type += " renderlists";
            else if (m_rebuild & BuildRenderListsForTaggedRoots)
                type += " partial";
            else if (m_rebuild & BuildBatches)
                type += " batches";
        }

        qDebug() << "Renderer::render()" << this << type;
        timer.start();
    }

    if (!m_rhi) {
        Q_ASSERT(m_context->openglContext() == QOpenGLContext::currentContext());
        if (m_vao)
            m_vao->bind();
    } else {
        m_resourceUpdates = m_rhi->nextResourceUpdateBatch();
    }

    if (m_rebuild & (BuildRenderLists | BuildRenderListsForTaggedRoots)) {
        bool complete = (m_rebuild & BuildRenderLists) != 0;
        if (complete)
            buildRenderListsFromScratch();
        else
            buildRenderListsForTaggedRoots();
        m_rebuild |= BuildBatches;

        if (Q_UNLIKELY(debug_build())) {
            qDebug("Opaque render lists %s:", (complete ? "(complete)" : "(partial)"));
            for (int i=0; i<m_opaqueRenderList.size(); ++i) {
                Element *e = m_opaqueRenderList.at(i);
                qDebug() << " - element:" << e << " batch:" << e->batch << " node:" << e->node << " order:" << e->order;
            }
            qDebug("Alpha render list %s:", complete ? "(complete)" : "(partial)");
            for (int i=0; i<m_alphaRenderList.size(); ++i) {
                Element *e = m_alphaRenderList.at(i);
                qDebug() << " - element:" << e << " batch:" << e->batch << " node:" << e->node << " order:" << e->order;
            }
        }
    }
    if (Q_UNLIKELY(debug_render())) timeRenderLists = timer.restart();

    for (int i=0; i<m_opaqueBatches.size(); ++i)
        m_opaqueBatches.at(i)->cleanupRemovedElements();
    for (int i=0; i<m_alphaBatches.size(); ++i)
        m_alphaBatches.at(i)->cleanupRemovedElements();
    deleteRemovedElements();

    cleanupBatches(&m_opaqueBatches);
    cleanupBatches(&m_alphaBatches);

    if (m_rebuild & BuildBatches) {
        prepareOpaqueBatches();
        if (Q_UNLIKELY(debug_render())) timePrepareOpaque = timer.restart();
        prepareAlphaBatches();
        if (Q_UNLIKELY(debug_render())) timePrepareAlpha = timer.restart();

        if (Q_UNLIKELY(debug_build())) {
            qDebug("Opaque Batches:");
            for (int i=0; i<m_opaqueBatches.size(); ++i) {
                Batch *b = m_opaqueBatches.at(i);
                qDebug() << " - Batch " << i << b << (b->needsUpload ? "upload" : "") << " root:" << b->root;
                for (Element *e = b->first; e; e = e->nextInBatch) {
                    qDebug() << "   - element:" << e << " node:" << e->node << e->order;
                }
            }
            qDebug("Alpha Batches:");
            for (int i=0; i<m_alphaBatches.size(); ++i) {
                Batch *b = m_alphaBatches.at(i);
                qDebug() << " - Batch " << i << b << (b->needsUpload ? "upload" : "") << " root:" << b->root;
                for (Element *e = b->first; e; e = e->nextInBatch) {
                    qDebug() << "   - element:" << e << e->bounds << " node:" << e->node << " order:" << e->order;
                }
            }
        }
    } else {
        if (Q_UNLIKELY(debug_render())) timePrepareOpaque = timePrepareAlpha = timer.restart();
    }


    deleteRemovedElements();

    if (m_rebuild != 0) {
        // Then sort opaque batches so that we're drawing the batches with the highest
        // order first, maximizing the benefit of front-to-back z-ordering.
        if (m_opaqueBatches.size())
            std::sort(&m_opaqueBatches.first(), &m_opaqueBatches.last() + 1, qsg_sort_batch_decreasing_order);

        // Sort alpha batches back to front so that they render correctly.
        if (m_alphaBatches.size())
            std::sort(&m_alphaBatches.first(), &m_alphaBatches.last() + 1, qsg_sort_batch_increasing_order);

        m_zRange = m_nextRenderOrder != 0
                 ? 1.0 / (m_nextRenderOrder)
                 : 0;
    }

    if (Q_UNLIKELY(debug_render())) timeSorting = timer.restart();

    int largestVBO = 0;
    int largestIBO = 0;

    if (Q_UNLIKELY(debug_upload())) qDebug("Uploading Opaque Batches:");
    for (int i=0; i<m_opaqueBatches.size(); ++i) {
        Batch *b = m_opaqueBatches.at(i);
        largestVBO = qMax(b->vbo.size, largestVBO);
        largestIBO = qMax(b->ibo.size, largestIBO);
        uploadBatch(b);
    }
    if (Q_UNLIKELY(debug_render())) timeUploadOpaque = timer.restart();


    if (Q_UNLIKELY(debug_upload())) qDebug("Uploading Alpha Batches:");
    for (int i=0; i<m_alphaBatches.size(); ++i) {
        Batch *b = m_alphaBatches.at(i);
        uploadBatch(b);
        largestVBO = qMax(b->vbo.size, largestVBO);
        largestIBO = qMax(b->ibo.size, largestIBO);
    }
    if (Q_UNLIKELY(debug_render())) timeUploadAlpha = timer.restart();

    if (largestVBO * 2 < m_vertexUploadPool.size())
        m_vertexUploadPool.resize(largestVBO * 2);
    if (m_context->separateIndexBuffer() && largestIBO * 2 < m_indexUploadPool.size())
        m_indexUploadPool.resize(largestIBO * 2);

    renderBatches();

    if (Q_UNLIKELY(debug_render())) {
        qDebug(" -> times: build: %d, prepare(opaque/alpha): %d/%d, sorting: %d, upload(opaque/alpha): %d/%d, render: %d",
               (int) timeRenderLists,
               (int) timePrepareOpaque, (int) timePrepareAlpha,
               (int) timeSorting,
               (int) timeUploadOpaque, (int) timeUploadAlpha,
               (int) timer.elapsed());
    }

    m_rebuild = 0;
    m_renderOrderRebuildLower = -1;
    m_renderOrderRebuildUpper = -1;

    if (m_visualizer->mode() != Visualizer::VisualizeNothing)
        m_visualizer->visualize();

    if (!m_rhi) {
        if (m_vao)
            m_vao->release();
    } else {
        if (m_visualizer->mode() != Visualizer::VisualizeNothing)
            commandBuffer()->endPass();

        if (m_resourceUpdates) {
            m_resourceUpdates->release();
            m_resourceUpdates = nullptr;
        }
    }
}

struct RenderNodeState : public QSGRenderNode::RenderState
{
    const QMatrix4x4 *projectionMatrix() const override { return m_projectionMatrix; }
    QRect scissorRect() const override { return m_scissorRect; }
    bool scissorEnabled() const override { return m_scissorEnabled; }
    int stencilValue() const override { return m_stencilValue; }
    bool stencilEnabled() const override { return m_stencilEnabled; }
    const QRegion *clipRegion() const override { return nullptr; }

    const QMatrix4x4 *m_projectionMatrix;
    QRect m_scissorRect;
    int m_stencilValue;
    bool m_scissorEnabled;
    bool m_stencilEnabled;
};

void Renderer::renderRenderNode(Batch *batch) // legacy (GL-only)
{
    if (Q_UNLIKELY(debug_render()))
        qDebug() << " -" << batch << "rendernode";

    Q_ASSERT(batch->first->isRenderNode);
    RenderNodeElement *e = (RenderNodeElement *) batch->first;

    setActiveShader(nullptr, nullptr);

    QSGNode *clip = e->renderNode->parent();
    QSGRenderNodePrivate *rd = QSGRenderNodePrivate::get(e->renderNode);
    rd->m_clip_list = nullptr;
    while (clip != rootNode()) {
        if (clip->type() == QSGNode::ClipNodeType) {
            rd->m_clip_list = static_cast<QSGClipNode *>(clip);
            break;
        }
        clip = clip->parent();
    }

    updateClip(rd->m_clip_list, batch);

    QMatrix4x4 pm = projectionMatrix();
    if (m_useDepthBuffer) {
        pm(2, 2) = m_zRange;
        pm(2, 3) = 1.0f - e->order * m_zRange;
    }

    RenderNodeState state;
    state.m_projectionMatrix = &pm;
    state.m_scissorEnabled = m_currentClipType & ClipState::ScissorClip;
    state.m_stencilEnabled = m_currentClipType & ClipState::StencilClip;
    state.m_scissorRect = m_currentScissorRect;
    state.m_stencilValue = m_currentStencilValue;

    QSGNode *xform = e->renderNode->parent();
    QMatrix4x4 matrix;
    QSGNode *root = rootNode();
    if (e->root) {
        matrix = qsg_matrixForRoot(e->root);
        root = e->root->sgNode;
    }
    while (xform != root) {
        if (xform->type() == QSGNode::TransformNodeType) {
            matrix = matrix * static_cast<QSGTransformNode *>(xform)->combinedMatrix();
            break;
        }
        xform = xform->parent();
    }
    rd->m_matrix = &matrix;

    QSGNode *opacity = e->renderNode->parent();
    rd->m_opacity = 1.0;
    while (opacity != rootNode()) {
        if (opacity->type() == QSGNode::OpacityNodeType) {
            rd->m_opacity = static_cast<QSGOpacityNode *>(opacity)->combinedOpacity();
            break;
        }
        opacity = opacity->parent();
    }

    glDisable(GL_STENCIL_TEST);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_DEPTH_TEST);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    QSGRenderNode::StateFlags changes = e->renderNode->changedStates();

    GLuint prevFbo = 0;
    if (changes & QSGRenderNode::RenderTargetState)
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, (GLint *) &prevFbo);

    e->renderNode->render(&state);

    rd->m_matrix = nullptr;
    rd->m_clip_list = nullptr;

    if (changes & QSGRenderNode::ViewportState) {
        QRect r = viewportRect();
        glViewport(r.x(), deviceRect().bottom() - r.bottom(), r.width(), r.height());
    }

    if (changes & QSGRenderNode::StencilState) {
        glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
        glStencilMask(0xff);
        glDisable(GL_STENCIL_TEST);
    }

    if (changes & (QSGRenderNode::StencilState | QSGRenderNode::ScissorState)) {
        glDisable(GL_SCISSOR_TEST);
        m_currentClip = nullptr;
        m_currentClipType = ClipState::NoClip;
    }

    if (changes & QSGRenderNode::DepthState)
        glDisable(GL_DEPTH_TEST);

    if (changes & QSGRenderNode::ColorState)
        bindable()->reactivate();

    if (changes & QSGRenderNode::BlendState) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    }

    if (changes & QSGRenderNode::CullState) {
        glFrontFace(isMirrored() ? GL_CW : GL_CCW);
        glDisable(GL_CULL_FACE);
    }

    if (changes & QSGRenderNode::RenderTargetState)
        glBindFramebuffer(GL_FRAMEBUFFER, prevFbo);
}

bool Renderer::prepareRhiRenderNode(Batch *batch, PreparedRenderBatch *renderBatch) // split prepare-render (RHI only)
{
    if (Q_UNLIKELY(debug_render()))
        qDebug() << " -" << batch << "rendernode";

    Q_ASSERT(batch->first->isRenderNode);
    RenderNodeElement *e = static_cast<RenderNodeElement *>(batch->first);

    setActiveRhiShader(nullptr, nullptr);

    QSGNode *clip = e->renderNode->parent();
    QSGRenderNodePrivate *rd = QSGRenderNodePrivate::get(e->renderNode);
    rd->m_clip_list = nullptr;
    while (clip != rootNode()) {
        if (clip->type() == QSGNode::ClipNodeType) {
            rd->m_clip_list = static_cast<QSGClipNode *>(clip);
            break;
        }
        clip = clip->parent();
    }

    updateClipState(rd->m_clip_list, batch);

    QSGNode *xform = e->renderNode->parent();
    QMatrix4x4 matrix;
    QSGNode *root = rootNode();
    if (e->root) {
        matrix = qsg_matrixForRoot(e->root);
        root = e->root->sgNode;
    }
    while (xform != root) {
        if (xform->type() == QSGNode::TransformNodeType) {
            matrix = matrix * static_cast<QSGTransformNode *>(xform)->combinedMatrix();
            break;
        }
        xform = xform->parent();
    }
    rd->m_matrix = &matrix;

    QSGNode *opacity = e->renderNode->parent();
    rd->m_opacity = 1.0;
    while (opacity != rootNode()) {
        if (opacity->type() == QSGNode::OpacityNodeType) {
            rd->m_opacity = static_cast<QSGOpacityNode *>(opacity)->combinedOpacity();
            break;
        }
        opacity = opacity->parent();
    }

    if (rd->m_prepareCallback)
        rd->m_prepareCallback();

    renderBatch->batch = batch;
    renderBatch->sms = nullptr;

    return true;
}

void Renderer::renderRhiRenderNode(const Batch *batch) // split prepare-render (RHI only)
{
    if (batch->clipState.type & ClipState::StencilClip)
        enqueueStencilDraw(batch);

    RenderNodeElement *e = static_cast<RenderNodeElement *>(batch->first);
    QSGRenderNodePrivate *rd = QSGRenderNodePrivate::get(e->renderNode);

    QMatrix4x4 pm = projectionMatrix();
    if (m_useDepthBuffer) {
        pm(2, 2) = m_zRange;
        pm(2, 3) = 1.0f - e->order * m_zRange;
    }

    RenderNodeState state;
    state.m_projectionMatrix = &pm;
    const std::array<int, 4> scissor = batch->clipState.scissor.scissor();
    state.m_scissorRect = QRect(scissor[0], scissor[1], scissor[2], scissor[3]);
    state.m_stencilValue = batch->clipState.stencilRef;
    state.m_scissorEnabled = batch->clipState.type & ClipState::ScissorClip;
    state.m_stencilEnabled = batch->clipState.type & ClipState::StencilClip;

    const QSGRenderNode::StateFlags changes = e->renderNode->changedStates();

    QRhiCommandBuffer *cb = commandBuffer();
    const bool needsExternal = rd->m_needsExternalRendering;
    if (needsExternal)
        cb->beginExternal();
    e->renderNode->render(&state);
    if (needsExternal)
        cb->endExternal();

    rd->m_matrix = nullptr;
    rd->m_clip_list = nullptr;

    if ((changes & QSGRenderNode::ViewportState)
            || (changes & QSGRenderNode::ScissorState))
    {
        // Reset both flags if either is reported as changed, since with the rhi
        // it could be setViewport() that will record the resetting of the scissor.
        m_pstate.viewportSet = false;
        m_pstate.scissorSet = false;
    }

    // Do not bother with RenderTargetState. Where applicable, endExternal()
    // ensures the correct target is rebound. For others (like Vulkan) it makes
    // no sense since render() could not possibly do that on our command buffer
    // which is in renderpass recording state.
}

void Renderer::setCustomRenderMode(const QByteArray &mode)
{
    if (mode.isEmpty())
        m_visualizer->setMode(Visualizer::VisualizeNothing);
    else if (mode == "clip")
        m_visualizer->setMode(Visualizer::VisualizeClipping);
    else if (mode == "overdraw")
        m_visualizer->setMode(Visualizer::VisualizeOverdraw);
    else if (mode == "batches")
        m_visualizer->setMode(Visualizer::VisualizeBatches);
    else if (mode == "changes")
        m_visualizer->setMode(Visualizer::VisualizeChanges);
}

bool Renderer::hasCustomRenderModeWithContinuousUpdate() const
{
    return m_visualizer->mode() == Visualizer::VisualizeOverdraw;
}

bool operator==(const GraphicsState &a, const GraphicsState &b) Q_DECL_NOTHROW
{
    return a.depthTest == b.depthTest
            && a.depthWrite == b.depthWrite
            && a.depthFunc == b.depthFunc
            && a.blending == b.blending
            && a.srcColor == b.srcColor
            && a.dstColor == b.dstColor
            && a.colorWrite == b.colorWrite
            && a.cullMode == b.cullMode
            && a.usesScissor == b.usesScissor
            && a.stencilTest == b.stencilTest
            && a.sampleCount == b.sampleCount
            && a.drawMode == b.drawMode
            && a.lineWidth == b.lineWidth;
}

bool operator!=(const GraphicsState &a, const GraphicsState &b) Q_DECL_NOTHROW
{
    return !(a == b);
}

uint qHash(const GraphicsState &s, uint seed) Q_DECL_NOTHROW
{
    // do not bother with all fields
    return seed
            + s.depthTest * 1000
            + s.depthWrite * 100
            + s.depthFunc
            + s.blending * 10
            + s.srcColor
            + s.cullMode
            + s.usesScissor
            + s.stencilTest
            + s.sampleCount;
}

bool operator==(const GraphicsPipelineStateKey &a, const GraphicsPipelineStateKey &b) Q_DECL_NOTHROW
{
    return a.state == b.state
            && a.sms->programRhi.program == b.sms->programRhi.program
            && a.compatibleRenderPassDescriptor->isCompatible(b.compatibleRenderPassDescriptor)
            && a.layoutCompatibleSrb->isLayoutCompatible(b.layoutCompatibleSrb);
}

bool operator!=(const GraphicsPipelineStateKey &a, const GraphicsPipelineStateKey &b) Q_DECL_NOTHROW
{
    return !(a == b);
}

uint qHash(const GraphicsPipelineStateKey &k, uint seed) Q_DECL_NOTHROW
{
    // no srb and rp included due to their special comparison semantics and lack of hash keys
    return qHash(k.state, seed) + qHash(k.sms->programRhi.program, seed);
}

Visualizer::Visualizer(Renderer *renderer)
    : m_renderer(renderer),
      m_visualizeMode(VisualizeNothing)
{
}

Visualizer::~Visualizer()
{
}

#define QSGNODE_DIRTY_PARENT (QSGNode::DirtyNodeAdded \
                              | QSGNode::DirtyOpacity \
                              | QSGNode::DirtyMatrix \
                              | QSGNode::DirtyNodeRemoved)

void Visualizer::visualizeChangesPrepare(Node *n, uint parentChanges)
{
    uint childDirty = (parentChanges | n->dirtyState) & QSGNODE_DIRTY_PARENT;
    uint selfDirty = n->dirtyState | parentChanges;
    if (n->type() == QSGNode::GeometryNodeType && selfDirty != 0)
        m_visualizeChangeSet.insert(n, selfDirty);
    SHADOWNODE_TRAVERSE(n) {
        visualizeChangesPrepare(child, childDirty);
    }
}

} // namespace QSGBatchRenderer

QT_END_NAMESPACE

#include "moc_qsgbatchrenderer_p.cpp"
