// Copyright (C) 2019 The Qt Company Ltd.
// Copyright (C) 2016 Jolla Ltd, author: <gunnar.sletta@jollamobile.com>
// Copyright (C) 2016 Robin Burchell <robin.burchell@viroteck.net>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsgbatchrenderer_p.h"

#include <qmath.h>

#include <QtCore/QElapsedTimer>
#include <QtCore/QtNumeric>

#include <QtGui/QGuiApplication>

#include <private/qnumeric_p.h>
#include "qsgmaterialshader_p.h"

#include "qsgrhivisualizer_p.h"

#include <algorithm>

QT_BEGIN_NAMESPACE

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
DECLARE_DEBUG_VAR(pools)
DECLARE_DEBUG_VAR(noalpha)
DECLARE_DEBUG_VAR(noopaque)
DECLARE_DEBUG_VAR(noclip)
#undef DECLARE_DEBUG_VAR

#define QSGNODE_TRAVERSE(NODE) for (QSGNode *child = NODE->firstChild(); child; child = child->nextSibling())
#define SHADOWNODE_TRAVERSE(NODE) for (Node *child = NODE->firstChild(); child; child = child->sibling())

static inline int size_of_type(int type)
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

static bool isTranslate(const QMatrix4x4 &m) { return m.flags() <= QMatrix4x4::Translation; }
static bool isScale(const QMatrix4x4 &m) { return m.flags() <= QMatrix4x4::Scale; }
static bool is2DSafe(const QMatrix4x4 &m) { return m.flags() < QMatrix4x4::Rotation; }

const float OPAQUE_LIMIT                = 0.999f;

const uint DYNAMIC_VERTEX_INDEX_BUFFER_THRESHOLD = 4;
const int VERTEX_BUFFER_BINDING = 0;
const int ZORDER_BUFFER_BINDING = VERTEX_BUFFER_BINDING + 1;

const float VIEWPORT_MIN_DEPTH = 0.0f;
const float VIEWPORT_MAX_DEPTH = 1.0f;

const quint32 DEFAULT_BUFFER_POOL_SIZE_LIMIT = 2 * 1024 * 1024; // 2 MB for m_vboPool and m_iboPool each

template <class Int>
inline Int aligned(Int v, Int byteAlign)
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
    Q_UNREACHABLE_RETURN(QRhiVertexInputAttribute::Float);
}

static QRhiVertexInputLayout calculateVertexInputLayout(const QSGMaterialShader *s, const QSGGeometry *geometry, bool batchable)
{
    Q_ASSERT(geometry);
    const QSGMaterialShaderPrivate *sd = QSGMaterialShaderPrivate::get(s);
    if (!sd->vertexShader) {
        qWarning("No vertex shader in QSGMaterialShader %p", s);
        return QRhiVertexInputLayout();
    }

    const int attrCount = geometry->attributeCount();
    QVarLengthArray<QRhiVertexInputAttribute, 8> inputAttributes;
    inputAttributes.reserve(attrCount + 1);
    quint32 offset = 0;
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
        Q_UNREACHABLE_RETURN(QRhiCommandBuffer::IndexUInt16);
    }
}

QRhiGraphicsPipeline::Topology qsg_topology(int geomDrawMode, QRhi *rhi)
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
    case QSGGeometry::DrawTriangleFan:
    {
        static bool triangleFanSupported = false;
        static bool triangleFanSupportChecked = false;
        if (!triangleFanSupportChecked) {
            triangleFanSupportChecked = true;
            triangleFanSupported = rhi->isFeatureSupported(QRhi::TriangleFanTopology);
        }
        if (triangleFanSupported) {
            topology = QRhiGraphicsPipeline::TriangleFan;
            break;
        }
        Q_FALLTHROUGH();
    }
    default:
        qWarning("Primitive topology 0x%x not supported", geomDrawMode);
        break;
    }
    return topology;
}

void qsg_setMultiViewFlagsOnMaterial(QSGMaterial *material, int multiViewCount)
{
    material->setFlag(QSGMaterial::MultiView2, multiViewCount == 2);
    material->setFlag(QSGMaterial::MultiView3, multiViewCount == 3);
    material->setFlag(QSGMaterial::MultiView4, multiViewCount == 4);
}

ShaderManager::Shader *ShaderManager::prepareMaterial(QSGMaterial *material,
                                                      const QSGGeometry *geometry,
                                                      QSGRendererInterface::RenderMode renderMode,
                                                      int multiViewCount)
{
    qsg_setMultiViewFlagsOnMaterial(material, multiViewCount);

    QSGMaterialType *type = material->type();
    ShaderKey key = { type, renderMode, multiViewCount };
    Shader *shader = rewrittenShaders.value(key, nullptr);
    if (shader)
        return shader;

    shader = new Shader;
    QSGMaterialShader *s = static_cast<QSGMaterialShader *>(material->createShader(renderMode));
    context->initializeRhiShader(s, QShader::BatchableVertexShader);
    shader->materialShader = s;
    shader->inputLayout = calculateVertexInputLayout(s, geometry, true);
    QSGMaterialShaderPrivate *sD = QSGMaterialShaderPrivate::get(s);
    shader->stages = {
        { QRhiShaderStage::Vertex, sD->shader(QShader::VertexStage), QShader::BatchableVertexShader },
        { QRhiShaderStage::Fragment, sD->shader(QShader::FragmentStage) }
    };

    shader->lastOpacity = 0;

    rewrittenShaders[key] = shader;
    return shader;
}

ShaderManager::Shader *ShaderManager::prepareMaterialNoRewrite(QSGMaterial *material,
                                                               const QSGGeometry *geometry,
                                                               QSGRendererInterface::RenderMode renderMode,
                                                               int multiViewCount)
{
    qsg_setMultiViewFlagsOnMaterial(material, multiViewCount);

    QSGMaterialType *type = material->type();
    ShaderKey key = { type, renderMode, multiViewCount };
    Shader *shader = stockShaders.value(key, nullptr);
    if (shader)
        return shader;

    shader = new Shader;
    QSGMaterialShader *s = static_cast<QSGMaterialShader *>(material->createShader(renderMode));
    context->initializeRhiShader(s, QShader::StandardShader);
    shader->materialShader = s;
    shader->inputLayout = calculateVertexInputLayout(s, geometry, false);
    QSGMaterialShaderPrivate *sD = QSGMaterialShaderPrivate::get(s);
    shader->stages = {
        { QRhiShaderStage::Vertex, sD->shader(QShader::VertexStage) },
        { QRhiShaderStage::Fragment, sD->shader(QShader::FragmentStage) }
    };

    shader->lastOpacity = 0;

    stockShaders[key] = shader;

    return shader;
}

void ShaderManager::invalidated()
{
    qDeleteAll(stockShaders);
    stockShaders.clear();
    qDeleteAll(rewrittenShaders);
    rewrittenShaders.clear();

    qDeleteAll(pipelineCache);
    pipelineCache.clear();

    qDeleteAll(srbPool);
    srbPool.clear();
}

void ShaderManager::clearCachedRendererData()
{
    for (ShaderManager::Shader *sms : std::as_const(stockShaders)) {
        QSGMaterialShader *s = sms->materialShader;
        if (s) {
            QSGMaterialShaderPrivate *sd = QSGMaterialShaderPrivate::get(s);
            sd->clearCachedRendererData();
        }
    }
    for (ShaderManager::Shader *sms : std::as_const(rewrittenShaders)) {
        QSGMaterialShader *s = sms->materialShader;
        if (s) {
            QSGMaterialShaderPrivate *sd = QSGMaterialShaderPrivate::get(s);
            sd->clearCachedRendererData();
        }
    }
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
    Q_UNUSED(n);
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
        e->translateOnlyToRoot = isTranslate(*gn->matrix());

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
            e->translateOnlyToRoot = isTranslate(*gn->matrix());
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
    if (isScale(matrix)) {
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
    return (nm->type() == m->type() && nm->viewCount() == m->viewCount() && nm->compare(m) == 0)
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
        if (!is2DSafe(*e->node->matrix()))
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

Renderer::Renderer(QSGDefaultRenderContext *ctx, QSGRendererInterface::RenderMode renderMode)
    : QSGRenderer(ctx)
    , m_context(ctx)
    , m_renderMode(renderMode)
    , m_opaqueRenderList(64)
    , m_alphaRenderList(64)
    , m_nextRenderOrder(0)
    , m_partialRebuild(false)
    , m_partialRebuildRoot(nullptr)
    , m_forceNoDepthBuffer(false)
    , m_opaqueBatches(16)
    , m_alphaBatches(16)
    , m_batchPool(16)
    , m_elementsToDelete(64)
    , m_tmpAlphaElements(16)
    , m_tmpOpaqueElements(16)
    , m_vboPool(16)
    , m_iboPool(16)
    , m_vboPoolCost(0)
    , m_iboPoolCost(0)
    , m_rebuild(FullRebuild)
    , m_zRange(0)
#if defined(QSGBATCHRENDERER_INVALIDATE_WEDGED_NODES)
    , m_renderOrderRebuildLower(-1)
    , m_renderOrderRebuildUpper(-1)
#endif
    , m_currentMaterial(nullptr)
    , m_currentShader(nullptr)
    , m_vertexUploadPool(256)
    , m_indexUploadPool(64)
{
    m_rhi = m_context->rhi();
    Q_ASSERT(m_rhi); // no more direct OpenGL code path in Qt 6

    m_ubufAlignment = m_rhi->ubufAlignment();

    m_uint32IndexForRhi = !m_rhi->isFeatureSupported(QRhi::NonFourAlignedEffectiveIndexBufferOffset);
    if (qEnvironmentVariableIntValue("QSG_RHI_UINT32_INDEX"))
        m_uint32IndexForRhi = true;

    m_visualizer = new RhiVisualizer(this);

    setNodeUpdater(new Updater(this));

    // The shader manager is shared between renderers (think for example Item
    // layers that create a new Renderer each) with the same rendercontext (and
    // so same QRhi).
    m_shaderManager = ctx->findChild<ShaderManager *>(QString(), Qt::FindDirectChildrenOnly);
    if (!m_shaderManager) {
        m_shaderManager = new ShaderManager(ctx);
        m_shaderManager->setObjectName(QStringLiteral("__qt_ShaderManager"));
        m_shaderManager->setParent(ctx);
        QObject::connect(ctx, SIGNAL(invalidated()), m_shaderManager, SLOT(invalidated()), Qt::DirectConnection);
    }

    m_batchNodeThreshold = qt_sg_envInt("QSG_RENDERER_BATCH_NODE_THRESHOLD", 64);
    m_batchVertexThreshold = qt_sg_envInt("QSG_RENDERER_BATCH_VERTEX_THRESHOLD", 1024);
    m_srbPoolThreshold = qt_sg_envInt("QSG_RENDERER_SRB_POOL_THRESHOLD", 1024);
    m_bufferPoolSizeLimit = qt_sg_envInt("QSG_RENDERER_BUFFER_POOL_LIMIT", DEFAULT_BUFFER_POOL_SIZE_LIMIT);

    if (Q_UNLIKELY(debug_build() || debug_render() || debug_pools())) {
        qDebug("Batch thresholds: nodes: %d vertices: %d srb pool: %d buffer pool: %d",
               m_batchNodeThreshold, m_batchVertexThreshold, m_srbPoolThreshold, m_bufferPoolSizeLimit);
    }
}

static void qsg_wipeBuffer(Buffer *buffer)
{
    delete buffer->buf;

    // The free here is ok because we're in one of two situations.
    // 1. We're using the upload pool in which case unmap will have set the
    //    data pointer to 0 and calling free on 0 is ok.
    // 2. We're using dedicated buffers because of visualization or IBO workaround
    //    and the data something we malloced and must be freed.
    free(buffer->data);
}

static void qsg_wipeBatch(Batch *batch)
{
    qsg_wipeBuffer(&batch->vbo);
    qsg_wipeBuffer(&batch->ibo);
    delete batch->ubuf;
    batch->stencilClipState.reset();
    delete batch;
}

Renderer::~Renderer()
{
    if (m_rhi) {
        // Clean up batches and buffers
        for (int i = 0; i < m_opaqueBatches.size(); ++i)
            qsg_wipeBatch(m_opaqueBatches.at(i));
        for (int i = 0; i < m_alphaBatches.size(); ++i)
            qsg_wipeBatch(m_alphaBatches.at(i));
        for (int i = 0; i < m_batchPool.size(); ++i)
            qsg_wipeBatch(m_batchPool.at(i));
        for (int i = 0; i < m_vboPool.size(); ++i)
            delete m_vboPool.at(i);
        for (int i = 0; i < m_iboPool.size(); ++i)
            delete m_iboPool.at(i);
    }

    for (Node *n : std::as_const(m_nodes)) {
        if (n->type() == QSGNode::GeometryNodeType) {
            Element *e = n->element();
            if (!e->removed)
                m_elementsToDelete.add(e);
        } else if (n->type() == QSGNode::ClipNodeType) {
            delete n->clipInfo();
        } else if (n->type() == QSGNode::RenderNodeType) {
            RenderNodeElement *e = n->renderNodeElement();
            if (!e->removed)
                m_elementsToDelete.add(e);
        }

        m_nodeAllocator.release(n);
    }

    // Remaining elements...
    for (int i=0; i<m_elementsToDelete.size(); ++i)
        releaseElement(m_elementsToDelete.at(i), true);

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

    m_rhi->releaseCachedResources();

    m_vertexUploadPool.shrink(0);
    m_vertexUploadPool.reset();
    m_indexUploadPool.shrink(0);
    m_indexUploadPool.reset();

    for (int i = 0; i < m_vboPool.size(); ++i)
        delete m_vboPool.at(i);
    m_vboPool.reset();
    m_vboPoolCost = 0;

    for (int i = 0; i < m_iboPool.size(); ++i)
        delete m_iboPool.at(i);
    m_iboPool.reset();
    m_iboPoolCost = 0;
}

void Renderer::invalidateAndRecycleBatch(Batch *b)
{
    if (b->vbo.buf != nullptr && m_vboPoolCost + b->vbo.buf->size() <= quint32(m_bufferPoolSizeLimit)) {
        m_vboPool.add(b->vbo.buf);
        m_vboPoolCost += b->vbo.buf->size();
    } else {
        delete b->vbo.buf;
    }
    if (b->ibo.buf != nullptr && m_iboPoolCost + b->ibo.buf->size() <= quint32(m_bufferPoolSizeLimit)) {
        m_iboPool.add(b->ibo.buf);
        m_iboPoolCost += b->ibo.buf->size();
    } else {
        delete b->ibo.buf;
    }
    b->vbo.buf = nullptr;
    b->ibo.buf = nullptr;
    b->invalidate();
    for (int i=0; i<m_batchPool.size(); ++i)
        if (b == m_batchPool.at(i))
            return;
    m_batchPool.add(b);
}

void Renderer::map(Buffer *buffer, quint32 byteSize, bool isIndexBuf)
{
    if (m_visualizer->mode() == Visualizer::VisualizeNothing) {
        // Common case, use a shared memory pool for uploading vertex data to avoid
        // excessive reevaluation
        QDataBuffer<char> &pool = isIndexBuf ? m_indexUploadPool : m_vertexUploadPool;
        if (byteSize > quint32(pool.size()))
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
    // Batches are pooled and reused which means the QRhiBuffer will be
    // still valid in a recycled Batch. We only hit the newBuffer() path
    // when there are no buffers to recycle.
    QDataBuffer<QRhiBuffer *> *bufferPool = isIndexBuf ? &m_iboPool : &m_vboPool;
    if (!buffer->buf && bufferPool->isEmpty()) {
        buffer->buf = m_rhi->newBuffer(QRhiBuffer::Immutable,
                                       isIndexBuf ? QRhiBuffer::IndexBuffer : QRhiBuffer::VertexBuffer,
                                       buffer->size);
        if (!buffer->buf->create()) {
            qWarning("Failed to build vertex/index buffer of size %u", buffer->size);
            delete buffer->buf;
            buffer->buf = nullptr;
        }
    } else {
        if (!buffer->buf) {
            const quint32 expectedSize = buffer->size;
            qsizetype foundBufferIndex = 0;
            for (qsizetype i = 0; i < bufferPool->size(); ++i) {
                QRhiBuffer *testBuffer = bufferPool->at(i);
                if (!buffer->buf
                    || (testBuffer->size() >= expectedSize && testBuffer->size() < buffer->buf->size())
                    || (testBuffer->size() < expectedSize && testBuffer->size() > buffer->buf->size())) {
                    foundBufferIndex = i;
                    buffer->buf = testBuffer;
                    if (buffer->buf->size() == expectedSize)
                        break;
                }
            }

            const qsizetype lastBufferIndex = bufferPool->size() - 1;
            if (foundBufferIndex < lastBufferIndex) {
                qSwap(bufferPool->data()[foundBufferIndex],
                      bufferPool->data()[lastBufferIndex]);
            }
            if (isIndexBuf)
                m_iboPoolCost -= bufferPool->data()[lastBufferIndex]->size();
            else
                m_vboPoolCost -= bufferPool->data()[lastBufferIndex]->size();
            bufferPool->pop_back();
        }

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
            if (!buffer->buf->create()) {
                qWarning("Failed to (re)build vertex/index buffer of size %u", buffer->size);
                delete buffer->buf;
                buffer->buf = nullptr;
            }
        }
    }
    if (buffer->buf) {
        if (buffer->buf->type() != QRhiBuffer::Dynamic) {
            m_resourceUpdates->uploadStaticBuffer(buffer->buf, 0, buffer->size, buffer->data);
            buffer->nonDynamicChangeCount += 1;
        } else {
            if (m_rhi->resourceLimit(QRhi::FramesInFlight) == 1)
                buffer->buf->fullDynamicBufferUpdateForCurrentFrame(buffer->data, buffer->size);
            else
                m_resourceUpdates->updateDynamicBuffer(buffer->buf, 0, buffer->size, buffer->data);
        }
    }
    if (m_visualizer->mode() == Visualizer::VisualizeNothing)
        buffer->data = nullptr;
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
            m_forceNoDepthBuffer = true;
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
                m_forceNoDepthBuffer = false;
                // Must have a full rebuild given useDepthBuffer() now returns
                // a different value than before, meaning there can once again
                // be an opaque pass.
                m_rebuild |= FullRebuild;
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
        if (opaque && useDepthBuffer())
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

#if defined(QSGBATCHRENDERER_INVALIDATE_WEDGED_NODES)
    if (m_renderOrderRebuildLower < 0 || batch->first->order < m_renderOrderRebuildLower)
        m_renderOrderRebuildLower = batch->first->order;
    if (m_renderOrderRebuildUpper < 0 || batch->lastOrderInBatch > m_renderOrderRebuildUpper)
        m_renderOrderRebuildUpper = batch->lastOrderInBatch;

    int first = m_renderOrderRebuildLower;
    int last = m_renderOrderRebuildUpper;
#else
    int first = batch->first->order;
    int last = batch->lastOrderInBatch;
#endif

    batch->invalidate();

    for (int i=0; i<m_alphaBatches.size(); ++i) {
        Batch *b = m_alphaBatches.at(i);
        if (b->first) {
            int bf = b->first->order;
            int bl = b->lastOrderInBatch;
            if (bl > first && bf < last)
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

            const QSGGeometry *gniGeometry = gni->geometry();
            const QSGMaterial *gniMaterial = gni->activeMaterial();
            const QSGGeometry *gnjGeometry = gnj->geometry();
            const QSGMaterial *gnjMaterial = gnj->activeMaterial();
            if (gni->clipList() == gnj->clipList()
                    && gniGeometry->drawingMode() == gnjGeometry->drawingMode()
                    && (gniGeometry->lineWidth() == gnjGeometry->lineWidth()
                        || (gniGeometry->drawingMode() != QSGGeometry::DrawLines
                            && gniGeometry->drawingMode() != QSGGeometry::DrawLineStrip))
                    && gniGeometry->attributes() == gnjGeometry->attributes()
                    && gniGeometry->indexType() == gnjGeometry->indexType()
                    && gni->inheritedOpacity() == gnj->inheritedOpacity()
                    && gniMaterial->type() == gnjMaterial->type()
                    && gniMaterial->viewCount() == gnjMaterial->viewCount()
                    && gniMaterial->compare(gnjMaterial) == 0)
            {
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
#if defined(QSGBATCHRENDERER_INVALIDATE_WEDGED_NODES)
        if (!e || e->batch)
#else
        if (!e)
#endif
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
            if (ej->batch) {
#if !defined(QSGBATCHRENDERER_INVALIDATE_WEDGED_NODES)
                overlapBounds |= ej->bounds;
#endif
                continue;
            }

            QSGGeometryNode *gnj = ej->node;
            if (gnj->geometry()->vertexCount() == 0)
                continue;

            const QSGGeometry *gniGeometry = gni->geometry();
            const QSGMaterial *gniMaterial = gni->activeMaterial();
            const QSGGeometry *gnjGeometry = gnj->geometry();
            const QSGMaterial *gnjMaterial = gnj->activeMaterial();
            if (gni->clipList() == gnj->clipList()
                    && gniGeometry->drawingMode() == gnjGeometry->drawingMode()
                    && (gniGeometry->drawingMode() != QSGGeometry::DrawLines
                        || (gniGeometry->lineWidth() == gnjGeometry->lineWidth()
                            // Must not do overlap checks when the line width is not 1,
                            // we have no knowledge how such lines are rasterized.
                            && gniGeometry->lineWidth() == 1.0f))
                    && gniGeometry->attributes() == gnjGeometry->attributes()
                    && gniGeometry->indexType() == gnjGeometry->indexType()
                    && gni->inheritedOpacity() == gnj->inheritedOpacity()
                    && gniMaterial->type() == gnjMaterial->type()
                    && gniMaterial->viewCount() == gnjMaterial->viewCount()
                    && gniMaterial->compare(gnjMaterial) == 0)
            {
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

static inline float calculateElementZOrder(const Element *e, qreal zRange)
{
    // Clamp the zOrder to within the min and max depth of the viewport.
    return std::clamp(1.0f - float(e->order * zRange), VIEWPORT_MIN_DEPTH, VIEWPORT_MAX_DEPTH);
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
    const float *localxdata = localx.constData();

    const int vCount = g->vertexCount();
    const int vSize = g->sizeOfVertex();
    memcpy(*vertexData, g->vertexData(), vSize * vCount);

    // apply vertex transform..
    char *vdata = *vertexData + vaOffset;
    if (localx.flags() == QMatrix4x4::Translation) {
        for (int i=0; i<vCount; ++i) {
            Pt *p = (Pt *) vdata;
            p->x += localxdata[12];
            p->y += localxdata[13];
            vdata += vSize;
        }
    } else if (localx.flags() > QMatrix4x4::Translation) {
        for (int i=0; i<vCount; ++i) {
            ((Pt *) vdata)->map(localx);
            vdata += vSize;
        }
    }

    if (useDepthBuffer()) {
        float *vzorder = (float *) *zData;
        float zorder = calculateElementZOrder(e, m_zRange);
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
            && (flags & (QSGMaterial::NoBatching | QSGMaterial_FullMatrix)) == 0
            && ((flags & QSGMaterial::RequiresFullMatrixExceptTranslate) == 0 || b->isTranslateOnlyToRoot())
            && b->isSafeToBatch();

    b->merged = canMerge;

    // Figure out how much memory we need...
    b->vertexCount = 0;
    b->indexCount = 0;
    int unmergedIndexSize = 0;
    Element *e = b->first;

    // Merged batches always do indexed draw calls. Non-indexed geometry gets
    // indices generated automatically, when merged.
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
        if (useDepthBuffer())
            bufferSize += b->vertexCount * sizeof(float);
    } else {
        ibufferSize = unmergedIndexSize;
    }

    map(&b->ibo, ibufferSize, true);
    map(&b->vbo, bufferSize);

    if (Q_UNLIKELY(debug_upload())) qDebug() << " - batch" << b << " first:" << b->first << " root:"
                                             << b->root << " merged:" << b->merged << " positionAttribute" << b->positionAttribute
                                             << " vbo:" << b->vbo.buf << ":" << b->vbo.size;

    if (b->merged) {
        char *vertexData = b->vbo.data;
        char *zData = vertexData + b->vertexCount * g->sizeOfVertex();
        char *indexData = b->ibo.data;

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
        int drawSetIndices = 0;
        const char *indexBase = b->ibo.data;
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
        char *iboData = b->ibo.data;
        Element *e = b->first;
        while (e) {
            QSGGeometry *g = e->node->geometry();
            int vbs = g->vertexCount() * g->sizeOfVertex();
            memcpy(vboData, g->vertexData(), vbs);
            vboData = vboData + vbs;
            const int indexCount = g->indexCount();
            if (indexCount) {
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
            if (b->merged && useDepthBuffer()) {
                float zorder = ((float*)(b->vbo.data + b->vertexCount * g->sizeOfVertex()))[i];
                dump << " Z:(" << zorder << ")";
            }
            vd += g->sizeOfVertex();
        }

        if (!b->drawSets.isEmpty()) {
            if (m_uint32IndexForRhi) {
                const quint32 *id = (const quint32 *) b->ibo.data;
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
                const quint16 *id = (const quint16 *) b->ibo.data;
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
    unmap(&b->ibo, true);

    if (Q_UNLIKELY(debug_upload() || debug_pools()))
        qDebug() << "  --- vertex/index buffers unmapped, batch upload completed... vbo pool size" << m_vboPoolCost << "ibo pool size" << m_iboPoolCost;

    b->needsUpload = false;

    if (Q_UNLIKELY(debug_render()))
        b->uploadedThisFrame = true;
}

void Renderer::applyClipStateToGraphicsState()
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
    ps->setSampleCount(renderTarget().rt->sampleCount());
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

    ps->setMultiViewCount(renderTarget().multiViewCount);

    ps->setShaderStages({ QRhiShaderStage(QRhiShaderStage::Vertex, m_stencilClipCommon.vs),
                          QRhiShaderStage(QRhiShaderStage::Fragment, m_stencilClipCommon.fs) });
    ps->setVertexInputLayout(m_stencilClipCommon.inputLayout);
    ps->setShaderResourceBindings(batch->stencilClipState.srb); // use something, it just needs to be layout-compatible
    ps->setRenderPassDescriptor(renderTarget().rpDesc);

    if (!ps->create()) {
        qWarning("Failed to build stencil clip pipeline");
        delete ps;
        return nullptr;
    }

    return ps;
}

void Renderer::updateClipState(const QSGClipNode *clipList, Batch *batch)
{
    // Note: No use of the clip-related speparate m_current* vars is allowed
    // here. All stored in batch->clipState instead. To collect state during
    // the prepare steps, m_currentClipState is used. It should not be used in
    // the render steps afterwards.

    // The stenciling logic is slightly different from Qt 5's direct OpenGL version
    // as we cannot just randomly clear the stencil buffer. We now put all clip
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
    quint32 totalVSize = 0;
    quint32 totalISize = 0;
    quint32 totalUSize = 0;
    const quint32 StencilClipUbufSize = 64;

    while (clip) {
        QMatrix4x4 m = m_current_projection_matrix_native_ndc[0]; // never hit for 3D and so multiview
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

            qint32 ix1 = qRound((fx1 + 1) * deviceRect.width() * qreal(0.5));
            qint32 iy1 = qRound((fy1 + 1) * deviceRect.height() * qreal(0.5));
            qint32 ix2 = qRound((fx2 + 1) * deviceRect.width() * qreal(0.5));
            qint32 iy2 = qRound((fy2 + 1) * deviceRect.height() * qreal(0.5));

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
            totalVSize = aligned(totalVSize, 4u) + vertexByteSize;
            if (g->indexCount()) {
                const int indexByteSize = g->sizeOfIndex() * g->indexCount();
                // so no need to worry about NonFourAlignedEffectiveIndexBufferOffset
                totalISize = aligned(totalISize, 4u) + indexByteSize;
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
            if (!batch->stencilClipState.vbuf->create()) {
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
                if (!batch->stencilClipState.ibuf->create()) {
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
            if (!batch->stencilClipState.ubuf->create()) {
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
            if (!batch->stencilClipState.srb->create()) {
                qWarning("Failed to build stencil clip srb");
                delete batch->stencilClipState.srb;
                batch->stencilClipState.srb = nullptr;
                return;
            }
        }

        quint32 vOffset = 0;
        quint32 iOffset = 0;
        quint32 uOffset = 0;
        for (const QSGClipNode *clip : stencilClipNodes) {
            const QSGGeometry *g = clip->geometry();
            const QSGGeometry::Attribute *a = g->attributes();
            StencilClipState::StencilDrawCall drawCall;
            const bool firstStencilClipInBatch = batch->stencilClipState.drawCalls.isEmpty();

            if (firstStencilClipInBatch) {
                m_stencilClipCommon.inputLayout.setBindings({ QRhiVertexInputBinding(g->sizeOfVertex()) });
                m_stencilClipCommon.inputLayout.setAttributes({ QRhiVertexInputAttribute(0, 0, qsg_vertexInputFormat(*a), 0) });
                m_stencilClipCommon.topology = qsg_topology(g->drawingMode(), m_rhi);
            }
#ifndef QT_NO_DEBUG
            else {
                if (qsg_topology(g->drawingMode(), m_rhi) != m_stencilClipCommon.topology)
                    qWarning("updateClipState: Clip list entries have different primitive topologies, this is not currently supported.");
                if (qsg_vertexInputFormat(*a) != m_stencilClipCommon.inputLayout.cbeginAttributes()->format())
                    qWarning("updateClipState: Clip list entries have different vertex input layouts, this is must not happen.");
            }
#endif

            drawCall.vbufOffset = aligned(vOffset, 4u);
            const int vertexByteSize = g->sizeOfVertex() * g->vertexCount();
            vOffset = drawCall.vbufOffset + vertexByteSize;

            int indexByteSize = 0;
            if (g->indexCount()) {
                drawCall.ibufOffset = aligned(iOffset, 4u);
                indexByteSize = g->sizeOfIndex() * g->indexCount();
                iOffset = drawCall.ibufOffset + indexByteSize;
            }

            drawCall.ubufOffset = aligned(uOffset, m_ubufAlignment);
            uOffset = drawCall.ubufOffset + StencilClipUbufSize;

            QMatrix4x4 matrixYUpNDC = m_current_projection_matrix[0];
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
            m_stencilClipCommon.vs = QSGMaterialShaderPrivate::loadShader(QLatin1String(":/qt-project.org/scenegraph/shaders_ng/stencilclip.vert.qsb"));

        if (!m_stencilClipCommon.fs.isValid())
            m_stencilClipCommon.fs = QSGMaterialShaderPrivate::loadShader(QLatin1String(":/qt-project.org/scenegraph/shaders_ng/stencilclip.frag.qsb"));

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

void Renderer::enqueueStencilDraw(const Batch *batch)
{
    // cliptype stencil + updateStencilBuffer==false means the batch uses
    // stenciling but relies on the stencil data generated by a previous batch
    // (due to the having the same clip node). Do not enqueue draw calls for
    // stencil in this case as the stencil buffer is already up-to-date.
    if (!batch->stencilClipState.updateStencilBuffer)
        return;

    QRhiCommandBuffer *cb = renderTarget().cb;
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

void Renderer::setActiveRhiShader(QSGMaterialShader *program, ShaderManager::Shader *shader)
{
    Q_ASSERT(m_rhi);
    m_currentProgram = program;
    m_currentShader = shader;
    m_currentMaterial = nullptr;
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

bool Renderer::ensurePipelineState(Element *e, const ShaderManager::Shader *sms, bool depthPostPass)
{
    // Note the key's == and qHash implementations: the renderpass descriptor
    // and srb are tested for compatibility, not pointer equality.
    //
    // We do not store the srb pointer itself because the ownership stays with
    // the Element and that can go away more often that we would like it
    // to. (think scrolling a list view, constantly dropping and creating new
    // nodes) Rather, use an opaque blob of a few uints and store and compare
    // that. This works because once the pipeline is built, we will always call
    // setShaderResources with an explicitly specified srb which is fine even if
    // e->srb we used here to bake the pipeline is already gone by that point.
    //
    // A typical QSGMaterial's serialized srb layout is 8 uints. (uniform buffer
    // + texture, 4 fields each) Regardless, using an implicitly shared
    // container is essential here. (won't detach so no more allocs and copies
    // are done, unless the Element decides to rebake the srb with a different
    // layout - but then the detach is exactly what we need)
    //
    // Same story for the renderpass descriptor: the object can go away but
    // that's fine because that has no effect on an already built pipeline, and
    // for comparison we only rely on the serialized blob in order decide if the
    // render target is compatible with the pipeline.

    const GraphicsPipelineStateKey k = GraphicsPipelineStateKey::create(m_gstate, sms, renderTarget().rpDesc, e->srb);

    // Note: dynamic state (viewport rect, scissor rect, stencil ref, blend
    // constant) is never a part of GraphicsState/QRhiGraphicsPipeline.

    // See if there is an existing, matching pipeline state object.
    auto it = m_shaderManager->pipelineCache.constFind(k);
    if (it != m_shaderManager->pipelineCache.constEnd()) {
        if (depthPostPass)
            e->depthPostPassPs = *it;
        else
            e->ps = *it;
        return true;
    }

    // Build a new one. This is potentially expensive.
    QRhiGraphicsPipeline *ps = m_rhi->newGraphicsPipeline();
    ps->setShaderStages(sms->stages.cbegin(), sms->stages.cend());
    ps->setVertexInputLayout(sms->inputLayout);
    ps->setShaderResourceBindings(e->srb);
    ps->setRenderPassDescriptor(renderTarget().rpDesc);

    QRhiGraphicsPipeline::Flags flags;
    if (needsBlendConstant(m_gstate.srcColor) || needsBlendConstant(m_gstate.dstColor)
            || needsBlendConstant(m_gstate.srcAlpha) || needsBlendConstant(m_gstate.dstAlpha))
    {
        flags |= QRhiGraphicsPipeline::UsesBlendConstants;
    }
    if (m_gstate.usesScissor)
        flags |= QRhiGraphicsPipeline::UsesScissor;
    if (m_gstate.stencilTest)
        flags |= QRhiGraphicsPipeline::UsesStencilRef;

    ps->setFlags(flags);
    ps->setTopology(qsg_topology(m_gstate.drawMode, m_rhi));
    ps->setCullMode(m_gstate.cullMode);
    ps->setFrontFace(invertFrontFace() ? QRhiGraphicsPipeline::CW : QRhiGraphicsPipeline::CCW);
    ps->setPolygonMode(m_gstate.polygonMode);
    ps->setMultiViewCount(m_gstate.multiViewCount);

    QRhiGraphicsPipeline::TargetBlend blend;
    blend.colorWrite = m_gstate.colorWrite;
    blend.enable = m_gstate.blending;
    blend.srcColor = m_gstate.srcColor;
    blend.dstColor = m_gstate.dstColor;
    blend.srcAlpha = m_gstate.srcAlpha;
    blend.dstAlpha = m_gstate.dstAlpha;
    blend.opColor = m_gstate.opColor;
    blend.opAlpha = m_gstate.opAlpha;
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

    if (!ps->create()) {
        qWarning("Failed to build graphics pipeline state");
        delete ps;
        return false;
    }

    m_shaderManager->pipelineCache.insert(k, ps);
    if (depthPostPass)
        e->depthPostPassPs = ps;
    else
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
        if (m_dummyTexture->create()) {
            if (m_resourceUpdates) {
                QImage img(m_dummyTexture->pixelSize(), QImage::Format_RGBA8888_Premultiplied);
                img.fill(0);
                m_resourceUpdates->uploadTexture(m_dummyTexture, img);
            }
        }
    }
    return m_dummyTexture;
}

static void rendererToMaterialGraphicsState(QSGMaterialShader::GraphicsPipelineState *dst,
                                            GraphicsState *src)
{
    dst->blendEnable = src->blending;

    // the enum values should match, sanity check it
    Q_ASSERT(int(QSGMaterialShader::GraphicsPipelineState::OneMinusSrc1Alpha) == int(QRhiGraphicsPipeline::OneMinusSrc1Alpha));
    Q_ASSERT(int(QSGMaterialShader::GraphicsPipelineState::BlendOp::Max) == int(QRhiGraphicsPipeline::Max));
    Q_ASSERT(int(QSGMaterialShader::GraphicsPipelineState::A) == int(QRhiGraphicsPipeline::A));
    Q_ASSERT(int(QSGMaterialShader::GraphicsPipelineState::CullBack) == int(QRhiGraphicsPipeline::Back));
    Q_ASSERT(int(QSGMaterialShader::GraphicsPipelineState::Line) == int(QRhiGraphicsPipeline::Line));
    dst->srcColor = QSGMaterialShader::GraphicsPipelineState::BlendFactor(src->srcColor);
    dst->dstColor = QSGMaterialShader::GraphicsPipelineState::BlendFactor(src->dstColor);

    // For compatibility with any existing code, separateBlendFactors defaults
    // to _false_ which means that materials that do not touch srcAlpha and
    // dstAlpha will continue to use srcColor and dstColor as the alpha
    // blending factors. New code that needs different values for color/alpha,
    // can explicitly set separateBlendFactors to true and then set srcAlpha
    // and dstAlpha as well.
    dst->separateBlendFactors = false;

    dst->srcAlpha = QSGMaterialShader::GraphicsPipelineState::BlendFactor(src->srcAlpha);
    dst->dstAlpha = QSGMaterialShader::GraphicsPipelineState::BlendFactor(src->dstAlpha);

    dst->opColor = QSGMaterialShader::GraphicsPipelineState::BlendOp(src->opColor);
    dst->opAlpha = QSGMaterialShader::GraphicsPipelineState::BlendOp(src->opAlpha);

    dst->colorWrite = QSGMaterialShader::GraphicsPipelineState::ColorMask(int(src->colorWrite));

    dst->cullMode = QSGMaterialShader::GraphicsPipelineState::CullMode(src->cullMode);
    dst->polygonMode = QSGMaterialShader::GraphicsPipelineState::PolygonMode(src->polygonMode);
}

static void materialToRendererGraphicsState(GraphicsState *dst,
                                            QSGMaterialShader::GraphicsPipelineState *src)
{
    dst->blending = src->blendEnable;
    dst->srcColor = QRhiGraphicsPipeline::BlendFactor(src->srcColor);
    dst->dstColor = QRhiGraphicsPipeline::BlendFactor(src->dstColor);
    if (src->separateBlendFactors) {
        dst->srcAlpha = QRhiGraphicsPipeline::BlendFactor(src->srcAlpha);
        dst->dstAlpha = QRhiGraphicsPipeline::BlendFactor(src->dstAlpha);
    } else {
        dst->srcAlpha = dst->srcColor;
        dst->dstAlpha = dst->dstColor;
    }
    dst->opColor = QRhiGraphicsPipeline::BlendOp(src->opColor);
    dst->opAlpha = QRhiGraphicsPipeline::BlendOp(src->opAlpha);
    dst->colorWrite = QRhiGraphicsPipeline::ColorMask(int(src->colorWrite));
    dst->cullMode = QRhiGraphicsPipeline::CullMode(src->cullMode);
    dst->polygonMode = QRhiGraphicsPipeline::PolygonMode(src->polygonMode);
}

void Renderer::updateMaterialDynamicData(ShaderManager::Shader *sms,
                                         QSGMaterialShader::RenderState &renderState,
                                         QSGMaterial *material,
                                         const Batch *batch,
                                         Element *e,
                                         int ubufOffset,
                                         int ubufRegionSize,
                                         char *directUpdatePtr)
{
    m_current_resource_update_batch = m_resourceUpdates;

    QSGMaterialShader *shader = sms->materialShader;
    QSGMaterialShaderPrivate *pd = QSGMaterialShaderPrivate::get(shader);
    QVarLengthArray<QRhiShaderResourceBinding, 8> bindings;

    if (pd->ubufBinding >= 0) {
        m_current_uniform_data = &pd->masterUniformData;
        const bool changed = shader->updateUniformData(renderState, material, m_currentMaterial);
        m_current_uniform_data = nullptr;

        if (changed || !batch->ubufDataValid) {
            if (directUpdatePtr)
                memcpy(directUpdatePtr + ubufOffset, pd->masterUniformData.constData(), ubufRegionSize);
            else
                m_resourceUpdates->updateDynamicBuffer(batch->ubuf, ubufOffset, ubufRegionSize, pd->masterUniformData.constData());
        }

        bindings.append(QRhiShaderResourceBinding::uniformBuffer(pd->ubufBinding,
                                                                 pd->ubufStages,
                                                                 batch->ubuf,
                                                                 ubufOffset,
                                                                 ubufRegionSize));
    }

    for (int binding = 0; binding < QSGMaterialShaderPrivate::MAX_SHADER_RESOURCE_BINDINGS; ++binding) {
        const QRhiShaderResourceBinding::StageFlags stages = pd->combinedImageSamplerBindings[binding];
        if (!stages)
            continue;

        const QVarLengthArray<QSGTexture *, 4> &prevTex(pd->textureBindingTable[binding]);
        QVarLengthArray<QSGTexture *, 4> nextTex = prevTex;

        const int count = pd->combinedImageSamplerCount[binding];
        nextTex.resize(count);

        shader->updateSampledImage(renderState, binding, nextTex.data(), material,
                                   m_currentMaterial);

        if (nextTex.contains(nullptr)) {
            qWarning("No QSGTexture provided from updateSampledImage(). This is wrong.");
            continue;
        }

        bool hasDirtySamplerOptions = false;
        bool isAnisotropic = false;
        for (QSGTexture *t : nextTex) {
            QSGTexturePrivate *td = QSGTexturePrivate::get(t);
            hasDirtySamplerOptions |= td->hasDirtySamplerOptions();
            isAnisotropic |= t->anisotropyLevel() != QSGTexture::AnisotropyNone;
            td->resetDirtySamplerOptions();
        }

        // prevTex may be invalid at this point, avoid dereferencing it
        if (nextTex != prevTex || hasDirtySamplerOptions) {

            // The QSGTexture, and so the sampler parameters, may have changed.
            // The rhiTexture is not relevant here.
            pd->textureBindingTable[binding] = nextTex; // does not own
            pd->samplerBindingTable[binding].clear();

            if (isAnisotropic) // ###
                qWarning("QSGTexture anisotropy levels are not currently supported");

            QVarLengthArray<QRhiSampler *, 4> samplers;

            for (QSGTexture *t : nextTex) {
                const QSGSamplerDescription samplerDesc = QSGSamplerDescription::fromTexture(t);

                QRhiSampler *sampler = m_samplers[samplerDesc];

                if (!sampler) {
                    sampler = newSampler(m_rhi, samplerDesc);
                    if (!sampler->create()) {
                        qWarning("Failed to build sampler");
                        delete sampler;
                        continue;
                    }
                    m_samplers[samplerDesc] = sampler;
                }
                samplers.append(sampler);
            }

            pd->samplerBindingTable[binding] = samplers; // does not own
        }

        if (pd->textureBindingTable[binding].size() == pd->samplerBindingTable[binding].size()) {

            QVarLengthArray<QRhiShaderResourceBinding::TextureAndSampler, 4> textureSamplers;

            for (int i = 0; i < pd->textureBindingTable[binding].size(); ++i) {

                QRhiTexture *texture = pd->textureBindingTable[binding].at(i)->rhiTexture();

                // texture may be null if the update above failed for any reason,
                // or if the QSGTexture chose to return null intentionally. This is
                // valid and we still need to provide something to the shader.
                if (!texture)
                    texture = dummyTexture();

                QRhiSampler *sampler = pd->samplerBindingTable[binding].at(i);

                textureSamplers.append(
                        QRhiShaderResourceBinding::TextureAndSampler { texture, sampler });
            }

            if (!textureSamplers.isEmpty())
                bindings.append(QRhiShaderResourceBinding::sampledTextures(
                        binding, stages, count, textureSamplers.constData()));
        }
    }

#ifndef QT_NO_DEBUG
    if (bindings.isEmpty())
        qWarning("No shader resources for material %p, this is odd.", material);
#endif

    enum class SrbAction {
        Unknown,
        DoNothing,
        UpdateResources,
        Rebake
    } srbAction = SrbAction::Unknown;

    // First, if the Element has no srb created at all, then try to find an existing,
    // currently unused srb that is layout-compatible with our binding list.
    if (!e->srb) {
        // reuse a QVector as our work area, thus possibly reusing the underlying allocation too
        QVector<quint32> &layoutDesc(m_shaderManager->srbLayoutDescSerializeWorkspace);
        layoutDesc.clear();
        QRhiShaderResourceBinding::serializeLayoutDescription(bindings.cbegin(), bindings.cend(), std::back_inserter(layoutDesc));
        e->srb = m_shaderManager->srbPool.take(layoutDesc);
        if (e->srb) {
            // Here we know layout compatibility is satisfied, but do not spend time on full
            // comparison. The chance of getting an srb that refers to the same resources
            // (buffer, textures) is low in practice. So reuse, but write new resources.
            srbAction = SrbAction::UpdateResources;
        }
    }

    // If the Element had an existing srb, investigate:
    //   - It may be used as-is (when nothing changed in the scene regarding this node compared to the previous frame).
    //   - Otherwise it may be able to go with a lightweight update (replace resources, binding list layout is the same).
    //   - If all else fails rebake the full thing, meaning we reuse the memory allocation but will recreate everything underneath.
    if (srbAction == SrbAction::Unknown && e->srb) {
        if (std::equal(e->srb->cbeginBindings(), e->srb->cendBindings(), bindings.cbegin(), bindings.cend())) {
            srbAction = SrbAction::DoNothing;
        } else if (std::equal(e->srb->cbeginBindings(), e->srb->cendBindings(), bindings.cbegin(), bindings.cend(),
                              [](const auto &a, const auto &b) { return a.isLayoutCompatible(b); }))
        {
            srbAction = SrbAction::UpdateResources;
        } else {
            srbAction = SrbAction::Rebake;
        }
    }

    // If the Element had no srb associated at all and could not find a layout-compatible
    // one from the pool, then create a whole new object.
    if (!e->srb) {
        e->srb = m_rhi->newShaderResourceBindings();
        srbAction = SrbAction::Rebake;
    }

    Q_ASSERT(srbAction != SrbAction::Unknown && e->srb);

    switch (srbAction) {
    case SrbAction::DoNothing:
        break;
    case SrbAction::UpdateResources:
    {
        e->srb->setBindings(bindings.cbegin(), bindings.cend());
        QRhiShaderResourceBindings::UpdateFlags flags;
        // Due to the way the binding list is built up above, if we have a uniform buffer
        // at binding point 0 (or none at all) then the sampledTexture bindings are added
        // with increasing binding points afterwards, so the list is already sorted based
        // on the binding points, thus we can save some time by telling the QRhi backend
        // not to sort again.
        if (pd->ubufBinding <= 0 || bindings.size() <= 1)
            flags |= QRhiShaderResourceBindings::BindingsAreSorted;

        e->srb->updateResources(flags);
    }
        break;
    case SrbAction::Rebake:
        e->srb->setBindings(bindings.cbegin(), bindings.cend());
        if (!e->srb->create())
            qWarning("Failed to build srb");
        break;
    default:
        Q_ASSERT_X(false, "updateMaterialDynamicData", "No srb action set, this cannot happen");
    }
}

void Renderer::updateMaterialStaticData(ShaderManager::Shader *sms,
                                        QSGMaterialShader::RenderState &renderState,
                                        QSGMaterial *material,
                                        Batch *batch,
                                        bool *gstateChanged)
{
    QSGMaterialShader *shader = sms->materialShader;
    *gstateChanged = false;
    if (shader->flags().testFlag(QSGMaterialShader::UpdatesGraphicsPipelineState)) {
        // generate the public mini-state from m_gstate, invoke the material,
        // write the changes, if any, back to m_gstate, together with a way to
        // roll those back.
        QSGMaterialShader::GraphicsPipelineState shaderPs;
        rendererToMaterialGraphicsState(&shaderPs, &m_gstate);
        const bool changed = shader->updateGraphicsPipelineState(renderState, &shaderPs, material, m_currentMaterial);
        if (changed) {
            m_gstateStack.push(m_gstate);
            materialToRendererGraphicsState(&m_gstate, &shaderPs);
            if (needsBlendConstant(m_gstate.srcColor) || needsBlendConstant(m_gstate.dstColor)
                    || needsBlendConstant(m_gstate.srcAlpha) || needsBlendConstant(m_gstate.dstAlpha))
            {
                batch->blendConstant = shaderPs.blendConstant;
            }
            *gstateChanged = true;
        }
    }
}

bool Renderer::prepareRenderMergedBatch(Batch *batch, PreparedRenderBatch *renderBatch)
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

    const int viewCount = projectionMatrixCount();
    m_current_projection_matrix.resize(viewCount);
    for (int viewIndex = 0; viewIndex < viewCount; ++viewIndex)
        m_current_projection_matrix[viewIndex] = projectionMatrix(viewIndex);

    m_current_projection_matrix_native_ndc.resize(projectionMatrixWithNativeNDCCount());
    for (int viewIndex = 0; viewIndex < projectionMatrixWithNativeNDCCount(); ++viewIndex)
        m_current_projection_matrix_native_ndc[viewIndex] = projectionMatrixWithNativeNDC(viewIndex);

    QSGMaterial *material = gn->activeMaterial();
    if (m_renderMode != QSGRendererInterface::RenderMode3D)
        updateClipState(gn->clipList(), batch);

    const QSGGeometry *g = gn->geometry();
    const int multiViewCount = renderTarget().multiViewCount;
    ShaderManager::Shader *sms = useDepthBuffer() ? m_shaderManager->prepareMaterial(material, g, m_renderMode, multiViewCount)
                                                  : m_shaderManager->prepareMaterialNoRewrite(material, g, m_renderMode, multiViewCount);
    if (!sms)
        return false;

    Q_ASSERT(sms->materialShader);
    if (m_currentShader != sms)
        setActiveRhiShader(sms->materialShader, sms);

    m_current_opacity = gn->inheritedOpacity();
    if (!qFuzzyCompare(sms->lastOpacity, float(m_current_opacity))) {
        dirty |= QSGMaterialShader::RenderState::DirtyOpacity;
        sms->lastOpacity = m_current_opacity;
    }

    QSGMaterialShaderPrivate *pd = QSGMaterialShaderPrivate::get(sms->materialShader);
    const quint32 ubufSize = quint32(pd->masterUniformData.size());
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
            if (!batch->ubuf->create()) {
                qWarning("Failed to build uniform buffer of size %u bytes", ubufSize);
                delete batch->ubuf;
                batch->ubuf = nullptr;
                return false;
            }
        }
    }

    QSGMaterialShader::RenderState renderState = state(QSGMaterialShader::RenderState::DirtyStates(int(dirty)));

    bool pendingGStatePop = false;
    updateMaterialStaticData(sms, renderState, material, batch, &pendingGStatePop);

    char *directUpdatePtr = nullptr;
    if (batch->ubuf->nativeBuffer().slotCount == 0)
        directUpdatePtr = batch->ubuf->beginFullDynamicBufferUpdateForCurrentFrame();

    updateMaterialDynamicData(sms, renderState, material, batch, e, 0, ubufSize, directUpdatePtr);

    if (directUpdatePtr)
        batch->ubuf->endFullDynamicBufferUpdateForCurrentFrame();

    m_gstate.drawMode = QSGGeometry::DrawingMode(g->drawingMode());
    m_gstate.lineWidth = g->lineWidth();

    const bool hasPipeline = ensurePipelineState(e, sms);

    if (pendingGStatePop)
        m_gstate = m_gstateStack.pop();

    if (!hasPipeline)
        return false;

    if (m_renderMode == QSGRendererInterface::RenderMode3D) {
        m_gstateStack.push(m_gstate);
        setStateForDepthPostPass();
        ensurePipelineState(e, sms, true);
        m_gstate = m_gstateStack.pop();
    }

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

void Renderer::renderMergedBatch(PreparedRenderBatch *renderBatch, bool depthPostPass)
{
    const Batch *batch = renderBatch->batch;
    if (!batch->vbo.buf || !batch->ibo.buf)
        return;

    Element *e = batch->first;
    QSGGeometryNode *gn = e->node;
    QSGGeometry *g = gn->geometry();
    checkLineWidth(g);

    if (batch->clipState.type & ClipState::StencilClip)
        enqueueStencilDraw(batch);

    QRhiCommandBuffer *cb = renderTarget().cb;
    setGraphicsPipeline(cb, batch, e, depthPostPass);

    for (int i = 0, ie = batch->drawSets.size(); i != ie; ++i) {
        const DrawSet &draw = batch->drawSets.at(i);
        const QRhiCommandBuffer::VertexInput vbufBindings[] = {
            { batch->vbo.buf, quint32(draw.vertices) },
            { batch->vbo.buf, quint32(draw.zorders) }
        };
        cb->setVertexInput(VERTEX_BUFFER_BINDING, useDepthBuffer() ? 2 : 1, vbufBindings,
                           batch->ibo.buf, draw.indices,
                           m_uint32IndexForRhi ? QRhiCommandBuffer::IndexUInt32 : QRhiCommandBuffer::IndexUInt16);
        cb->drawIndexed(draw.indexCount);
    }
}

bool Renderer::prepareRenderUnmergedBatch(Batch *batch, PreparedRenderBatch *renderBatch)
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

    const int viewCount = projectionMatrixCount();
    m_current_projection_matrix.resize(viewCount);
    for (int viewIndex = 0; viewIndex < viewCount; ++viewIndex)
        m_current_projection_matrix[viewIndex] = projectionMatrix(viewIndex);

    m_current_projection_matrix_native_ndc.resize(projectionMatrixWithNativeNDCCount());
    for (int viewIndex = 0; viewIndex < projectionMatrixWithNativeNDCCount(); ++viewIndex)
        m_current_projection_matrix_native_ndc[viewIndex] = projectionMatrixWithNativeNDC(viewIndex);

    QSGGeometryNode *gn = e->node;
    if (m_renderMode != QSGRendererInterface::RenderMode3D)
        updateClipState(gn->clipList(), batch);

    // We always have dirty matrix as all batches are at a unique z range.
    QSGMaterialShader::RenderState::DirtyStates dirty = QSGMaterialShader::RenderState::DirtyMatrix;

    // The vertex attributes are assumed to be the same for all elements in the
    // unmerged batch since the material (and so the shaders) is the same.
    QSGGeometry *g = gn->geometry();
    QSGMaterial *material = gn->activeMaterial();
    ShaderManager::Shader *sms = m_shaderManager->prepareMaterialNoRewrite(material, g, m_renderMode, renderTarget().multiViewCount);
    if (!sms)
        return false;

    Q_ASSERT(sms->materialShader);
    if (m_currentShader != sms)
        setActiveRhiShader(sms->materialShader, sms);

    m_current_opacity = gn->inheritedOpacity();
    if (sms->lastOpacity != m_current_opacity) {
        dirty |= QSGMaterialShader::RenderState::DirtyOpacity;
        sms->lastOpacity = m_current_opacity;
    }

    QMatrix4x4 rootMatrix = batch->root ? qsg_matrixForRoot(batch->root) : QMatrix4x4();

    QSGMaterialShaderPrivate *pd = QSGMaterialShaderPrivate::get(sms->materialShader);
    const quint32 ubufSize = quint32(pd->masterUniformData.size());
    if (pd->ubufBinding >= 0) {
        quint32 totalUBufSize = 0;
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
            if (!batch->ubuf->create()) {
                qWarning("Failed to build uniform buffer of size %u bytes", totalUBufSize);
                delete batch->ubuf;
                batch->ubuf = nullptr;
                return false;
            }
        }
    }

    QSGMaterialShader::RenderState renderState = state(QSGMaterialShader::RenderState::DirtyStates(int(dirty)));
    bool pendingGStatePop = false;
    updateMaterialStaticData(sms, renderState,
                             material, batch, &pendingGStatePop);

    int ubufOffset = 0;
    QRhiGraphicsPipeline *ps = nullptr;
    QRhiGraphicsPipeline *depthPostPassPs = nullptr;
    e = batch->first;

    char *directUpdatePtr = nullptr;
    if (batch->ubuf->nativeBuffer().slotCount == 0)
        directUpdatePtr = batch->ubuf->beginFullDynamicBufferUpdateForCurrentFrame();

    while (e) {
        gn = e->node;

        m_current_model_view_matrix = rootMatrix * *gn->matrix();
        m_current_determinant = m_current_model_view_matrix.determinant();

        const int viewCount = projectionMatrixCount();
        m_current_projection_matrix.resize(viewCount);
        for (int viewIndex = 0; viewIndex < viewCount; ++viewIndex)
            m_current_projection_matrix[viewIndex] = projectionMatrix(viewIndex);

        m_current_projection_matrix_native_ndc.resize(projectionMatrixWithNativeNDCCount());
        for (int viewIndex = 0; viewIndex < projectionMatrixWithNativeNDCCount(); ++viewIndex)
            m_current_projection_matrix_native_ndc[viewIndex] = projectionMatrixWithNativeNDC(viewIndex);

        if (useDepthBuffer()) {
            // this cannot be multiview
            m_current_projection_matrix[0](2, 2) = m_zRange;
            m_current_projection_matrix[0](2, 3) = calculateElementZOrder(e, m_zRange);
        }

        QSGMaterialShader::RenderState renderState = state(QSGMaterialShader::RenderState::DirtyStates(int(dirty)));
        updateMaterialDynamicData(sms, renderState, material, batch, e, ubufOffset, ubufSize, directUpdatePtr);

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
            if (m_renderMode == QSGRendererInterface::RenderMode3D) {
                m_gstateStack.push(m_gstate);
                setStateForDepthPostPass();
                ensurePipelineState(e, sms, true);
                m_gstate = m_gstateStack.pop();
                depthPostPassPs = e->depthPostPassPs;
            }
        } else {
            e->ps = ps;
            if (m_renderMode == QSGRendererInterface::RenderMode3D)
                e->depthPostPassPs = depthPostPassPs;
        }

        // We don't need to bother with asking each node for its material as they
        // are all identical (compare==0) since they are in the same batch.
        m_currentMaterial = material;

        // We only need to push this on the very first iteration...
        dirty &= ~QSGMaterialShader::RenderState::DirtyOpacity;

        e = e->nextInBatch;
    }

    if (directUpdatePtr)
        batch->ubuf->endFullDynamicBufferUpdateForCurrentFrame();

    if (pendingGStatePop)
        m_gstate = m_gstateStack.pop();

    batch->ubufDataValid = true;

    renderBatch->batch = batch;
    renderBatch->sms = sms;

    return true;
}

void Renderer::renderUnmergedBatch(PreparedRenderBatch *renderBatch, bool depthPostPass)
{
    const Batch *batch = renderBatch->batch;
    if (!batch->vbo.buf)
        return;

    Element *e = batch->first;

    if (batch->clipState.type & ClipState::StencilClip)
        enqueueStencilDraw(batch);

    quint32 vOffset = 0;
    quint32 iOffset = 0;
    QRhiCommandBuffer *cb = renderTarget().cb;

    while (e) {
        QSGGeometry *g = e->node->geometry();
        checkLineWidth(g);
        const int effectiveIndexSize = m_uint32IndexForRhi ? sizeof(quint32) : g->sizeOfIndex();

        setGraphicsPipeline(cb, batch, e, depthPostPass);

        const QRhiCommandBuffer::VertexInput vbufBinding(batch->vbo.buf, vOffset);
        if (g->indexCount()) {
            if (batch->ibo.buf) {
                cb->setVertexInput(VERTEX_BUFFER_BINDING, 1, &vbufBinding,
                                   batch->ibo.buf, iOffset,
                                   effectiveIndexSize == sizeof(quint32) ? QRhiCommandBuffer::IndexUInt32
                                                                         : QRhiCommandBuffer::IndexUInt16);
                cb->drawIndexed(g->indexCount());
            }
        } else {
            cb->setVertexInput(VERTEX_BUFFER_BINDING, 1, &vbufBinding);
            cb->draw(g->vertexCount());
        }

        vOffset += g->sizeOfVertex() * g->vertexCount();
        iOffset += g->indexCount() * effectiveIndexSize;

        e = e->nextInBatch;
    }
}

void Renderer::setGraphicsPipeline(QRhiCommandBuffer *cb, const Batch *batch, Element *e, bool depthPostPass)
{
    cb->setGraphicsPipeline(depthPostPass ? e->depthPostPassPs : e->ps);

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
    if (!depthPostPass && e->ps->flags().testFlag(QRhiGraphicsPipeline::UsesBlendConstants))
        cb->setBlendConstants(batch->blendConstant);

    cb->setShaderResources(e->srb);
}

void Renderer::releaseElement(Element *e, bool inDestructor)
{
    if (e->isRenderNode) {
        delete static_cast<RenderNodeElement *>(e);
    } else {
        if (e->srb) {
            if (!inDestructor) {
                if (m_shaderManager->srbPool.size() < m_srbPoolThreshold)
                    m_shaderManager->srbPool.insert(e->srb->serializedLayoutDescription(), e->srb);
                else
                    delete e->srb;
            } else {
                delete e->srb;
            }
            e->srb = nullptr;
        }
        m_elementAllocator.release(e);
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

    for (int i=0; i<m_elementsToDelete.size(); ++i)
        releaseElement(m_elementsToDelete.at(i));

    m_elementsToDelete.reset();
}

void Renderer::render()
{
    // Gracefully handle the lack of a render target - some autotests may rely
    // on this in odd cases.
    if (!renderTarget().rt)
        return;

    prepareRenderPass(&m_mainRenderPassContext);
    beginRenderPass(&m_mainRenderPassContext);
    recordRenderPass(&m_mainRenderPassContext);
    endRenderPass(&m_mainRenderPassContext);
}

// An alternative to render() is to call prepareInline() and renderInline() at
// the appropriate times (i.e. outside of a QRhi::beginPass() and then inside,
// respectively) These allow rendering within a render pass that is started by
// another component. In contrast, render() records a full render pass on its
// own.

void Renderer::prepareInline()
{
    prepareRenderPass(&m_mainRenderPassContext);
}

void Renderer::renderInline()
{
    recordRenderPass(&m_mainRenderPassContext);
}

void Renderer::prepareRenderPass(RenderPassContext *ctx)
{
    if (ctx->valid)
        qWarning("prepareRenderPass() called with an already prepared render pass context");

    ctx->valid = true;

    if (Q_UNLIKELY(debug_dump())) {
        qDebug("\n");
        QSGNodeDumper::dump(rootNode());
    }

    ctx->timeRenderLists = 0;
    ctx->timePrepareOpaque = 0;
    ctx->timePrepareAlpha = 0;
    ctx->timeSorting = 0;
    ctx->timeUploadOpaque = 0;
    ctx->timeUploadAlpha = 0;

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
        ctx->timer.start();
    }

    m_resourceUpdates = m_rhi->nextResourceUpdateBatch();

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
    if (Q_UNLIKELY(debug_render())) ctx->timeRenderLists = ctx->timer.restart();

    for (int i=0; i<m_opaqueBatches.size(); ++i)
        m_opaqueBatches.at(i)->cleanupRemovedElements();
    for (int i=0; i<m_alphaBatches.size(); ++i)
        m_alphaBatches.at(i)->cleanupRemovedElements();
    deleteRemovedElements();

    cleanupBatches(&m_opaqueBatches);
    cleanupBatches(&m_alphaBatches);

    if (m_rebuild & BuildBatches) {
        prepareOpaqueBatches();
        if (Q_UNLIKELY(debug_render())) ctx->timePrepareOpaque = ctx->timer.restart();
        prepareAlphaBatches();
        if (Q_UNLIKELY(debug_render())) ctx->timePrepareAlpha = ctx->timer.restart();

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
        if (Q_UNLIKELY(debug_render())) ctx->timePrepareOpaque = ctx->timePrepareAlpha = ctx->timer.restart();
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

    if (Q_UNLIKELY(debug_render())) ctx->timeSorting = ctx->timer.restart();

    // Set size to 0, nothing is deallocated, they will "grow" again
    // as part of uploadBatch.
    m_vertexUploadPool.reset();
    m_indexUploadPool.reset();

    if (Q_UNLIKELY(debug_upload())) qDebug("Uploading Opaque Batches:");
    for (int i=0; i<m_opaqueBatches.size(); ++i) {
        Batch *b = m_opaqueBatches.at(i);
        uploadBatch(b);
    }
    if (Q_UNLIKELY(debug_render())) ctx->timeUploadOpaque = ctx->timer.restart();

    if (Q_UNLIKELY(debug_upload())) qDebug("Uploading Alpha Batches:");
    for (int i=0; i<m_alphaBatches.size(); ++i) {
        Batch *b = m_alphaBatches.at(i);
        uploadBatch(b);
    }
    if (Q_UNLIKELY(debug_render())) ctx->timeUploadAlpha = ctx->timer.restart();

    if (Q_UNLIKELY(debug_render())) {
        qDebug().nospace() << "Rendering:" << Qt::endl
                           << " -> Opaque: " << qsg_countNodesInBatches(m_opaqueBatches) << " nodes in " << m_opaqueBatches.size() << " batches..." << Qt::endl
                           << " -> Alpha: " << qsg_countNodesInBatches(m_alphaBatches) << " nodes in " << m_alphaBatches.size() << " batches...";
    }

    m_current_opacity = 1;
    m_currentMaterial = nullptr;
    m_currentShader = nullptr;
    m_currentProgram = nullptr;
    m_currentClipState.reset();

    const QRect viewport = viewportRect();

    bool renderOpaque = !debug_noopaque();
    bool renderAlpha = !debug_noalpha();

    m_pstate.viewport =
            QRhiViewport(viewport.x(), deviceRect().bottom() - viewport.bottom(), viewport.width(),
                         viewport.height(), VIEWPORT_MIN_DEPTH, VIEWPORT_MAX_DEPTH);
    m_pstate.clearColor = clearColor();
    m_pstate.dsClear = QRhiDepthStencilClearValue(1.0f, 0);
    m_pstate.viewportSet = false;
    m_pstate.scissorSet = false;

    m_gstate.depthTest = useDepthBuffer();
    m_gstate.depthWrite = useDepthBuffer();
    m_gstate.depthFunc = QRhiGraphicsPipeline::Less;
    m_gstate.blending = false;

    m_gstate.cullMode = QRhiGraphicsPipeline::None;
    m_gstate.polygonMode = QRhiGraphicsPipeline::Fill;
    m_gstate.colorWrite = QRhiGraphicsPipeline::R
            | QRhiGraphicsPipeline::G
            | QRhiGraphicsPipeline::B
            | QRhiGraphicsPipeline::A;
    m_gstate.usesScissor = false;
    m_gstate.stencilTest = false;

    m_gstate.sampleCount = renderTarget().rt->sampleCount();
    m_gstate.multiViewCount = renderTarget().multiViewCount;

    ctx->opaqueRenderBatches.clear();
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
                ctx->opaqueRenderBatches.append(renderBatch);
        }
    }

    m_gstate.blending = true;
    // factors never change, always set for premultiplied alpha based blending

    // depth test stays enabled (if useDepthBuffer(), that is) but no need
    // to write out depth from the transparent (back-to-front) pass
    m_gstate.depthWrite = false;

    // special case: the 3D plane mode tests against the depth buffer, but does
    // not write (and all batches are alpha because this render mode evaluates
    // to useDepthBuffer()==false)
    if (m_renderMode == QSGRendererInterface::RenderMode3D) {
        Q_ASSERT(m_opaqueBatches.isEmpty());
        m_gstate.depthTest = true;
    }

    ctx->alphaRenderBatches.clear();
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
                ctx->alphaRenderBatches.append(renderBatch);
        }
    }

    m_rebuild = 0;

#if defined(QSGBATCHRENDERER_INVALIDATE_WEDGED_NODES)
    m_renderOrderRebuildLower = -1;
    m_renderOrderRebuildUpper = -1;
#endif

    if (m_visualizer->mode() != Visualizer::VisualizeNothing)
        m_visualizer->prepareVisualize();

    renderTarget().cb->resourceUpdate(m_resourceUpdates);
    m_resourceUpdates = nullptr;
}

void Renderer::beginRenderPass(RenderPassContext *)
{
    const QSGRenderTarget &rt(renderTarget());
    rt.cb->beginPass(rt.rt, m_pstate.clearColor, m_pstate.dsClear, nullptr,
                     // we cannot tell if the application will have
                     // native rendering thrown in to this pass
                     // (QQuickWindow::beginExternalCommands()), so
                     // we have no choice but to set the flag always
                     // (thus triggering using secondary command
                     // buffers with Vulkan)
                     QRhiCommandBuffer::ExternalContent
                     // We do not use GPU compute at all at the moment, this means we can
                     // get a small performance gain with OpenGL by declaring this.
                     | QRhiCommandBuffer::DoNotTrackResourcesForCompute);

    if (m_renderPassRecordingCallbacks.start)
        m_renderPassRecordingCallbacks.start(m_renderPassRecordingCallbacks.userData);
}

void Renderer::recordRenderPass(RenderPassContext *ctx)
{
    // prepareRenderPass and recordRenderPass must always be called together.
    // They are separate because beginRenderPass and endRenderPass are optional.
    //
    // The valid call sequence are therefore:
    //    prepare, begin, record, end
    // or
    //    prepare, record

    if (!ctx->valid)
        qWarning("recordRenderPass() called without a prepared render pass context");

    ctx->valid = false;

    QRhiCommandBuffer *cb = renderTarget().cb;
    cb->debugMarkBegin(QByteArrayLiteral("Qt Quick scene render"));

    for (int i = 0, ie = ctx->opaqueRenderBatches.size(); i != ie; ++i) {
        if (i == 0)
            cb->debugMarkMsg(QByteArrayLiteral("Qt Quick opaque batches"));
        PreparedRenderBatch *renderBatch = &ctx->opaqueRenderBatches[i];
        if (renderBatch->batch->merged)
            renderMergedBatch(renderBatch);
        else
            renderUnmergedBatch(renderBatch);
    }

    for (int i = 0, ie = ctx->alphaRenderBatches.size(); i != ie; ++i) {
        if (i == 0) {
            if (m_renderMode == QSGRendererInterface::RenderMode3D)
                cb->debugMarkMsg(QByteArrayLiteral("Qt Quick 2D-in-3D batches"));
            else
                cb->debugMarkMsg(QByteArrayLiteral("Qt Quick alpha batches"));
        }
        PreparedRenderBatch *renderBatch = &ctx->alphaRenderBatches[i];
        if (renderBatch->batch->merged)
            renderMergedBatch(renderBatch);
        else if (renderBatch->batch->isRenderNode)
            renderRhiRenderNode(renderBatch->batch);
        else
            renderUnmergedBatch(renderBatch);
    }

    if (m_renderMode == QSGRendererInterface::RenderMode3D) {
        // Depth post-pass to fill up the depth buffer in a way that it
        // corresponds to what got rendered to the color buffer in the previous
        // (alpha) pass. The previous pass cannot enable depth write due to Z
        // fighting. Rather, do it separately in a dedicated color-write-off,
        // depth-write-on pass. This enables the 3D content drawn afterwards to
        // depth test against the 2D items' rendering.
        for (int i = 0, ie = ctx->alphaRenderBatches.size(); i != ie; ++i) {
            if (i == 0)
                cb->debugMarkMsg(QByteArrayLiteral("Qt Quick 2D-in-3D depth post-pass"));
            PreparedRenderBatch *renderBatch = &ctx->alphaRenderBatches[i];
            if (renderBatch->batch->merged)
                renderMergedBatch(renderBatch, true);
            else if (!renderBatch->batch->isRenderNode) // rendernodes are skipped here for now
                renderUnmergedBatch(renderBatch, true);
        }
    }

    if (m_currentShader)
        setActiveRhiShader(nullptr, nullptr);

    cb->debugMarkEnd();

    if (Q_UNLIKELY(debug_render())) {
        qDebug(" -> times: build: %d, prepare(opaque/alpha): %d/%d, sorting: %d, upload(opaque/alpha): %d/%d, record rendering: %d",
               (int) ctx->timeRenderLists,
               (int) ctx->timePrepareOpaque, (int) ctx->timePrepareAlpha,
               (int) ctx->timeSorting,
               (int) ctx->timeUploadOpaque, (int) ctx->timeUploadAlpha,
               (int) ctx->timer.elapsed());
    }
}

void Renderer::endRenderPass(RenderPassContext *)
{
    if (m_renderPassRecordingCallbacks.end)
        m_renderPassRecordingCallbacks.end(m_renderPassRecordingCallbacks.userData);

    if (m_visualizer->mode() != Visualizer::VisualizeNothing)
        m_visualizer->visualize();

    renderTarget().cb->endPass();
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

bool Renderer::prepareRhiRenderNode(Batch *batch, PreparedRenderBatch *renderBatch)
{
    if (Q_UNLIKELY(debug_render()))
        qDebug() << " -" << batch << "rendernode";

    const int viewCount = projectionMatrixCount();
    m_current_projection_matrix.resize(viewCount);
    for (int viewIndex = 0; viewIndex < viewCount; ++viewIndex)
        m_current_projection_matrix[viewIndex] = projectionMatrix(viewIndex);
    m_current_projection_matrix_native_ndc.resize(projectionMatrixWithNativeNDCCount());
    for (int viewIndex = 0; viewIndex < projectionMatrixWithNativeNDCCount(); ++viewIndex)
        m_current_projection_matrix_native_ndc[viewIndex] = projectionMatrixWithNativeNDC(viewIndex);

    Q_ASSERT(batch->first->isRenderNode);
    RenderNodeElement *e = static_cast<RenderNodeElement *>(batch->first);

    setActiveRhiShader(nullptr, nullptr);

    QSGRenderNodePrivate *rd = QSGRenderNodePrivate::get(e->renderNode);
    rd->m_clip_list = nullptr;
    if (m_renderMode != QSGRendererInterface::RenderMode3D) {
        QSGNode *clip = e->renderNode->parent();
        while (clip != rootNode()) {
            if (clip->type() == QSGNode::ClipNodeType) {
                rd->m_clip_list = static_cast<QSGClipNode *>(clip);
                break;
            }
            clip = clip->parent();
        }
        updateClipState(rd->m_clip_list, batch);
    }

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
    rd->m_localMatrix = matrix;
    rd->m_matrix = &rd->m_localMatrix;

    QSGNode *opacity = e->renderNode->parent();
    rd->m_opacity = 1.0;
    while (opacity != rootNode()) {
        if (opacity->type() == QSGNode::OpacityNodeType) {
            rd->m_opacity = static_cast<QSGOpacityNode *>(opacity)->combinedOpacity();
            break;
        }
        opacity = opacity->parent();
    }

    rd->m_rt = renderTarget();

    rd->m_projectionMatrix.resize(viewCount);
    for (int viewIndex = 0; viewIndex < viewCount; ++viewIndex)
        rd->m_projectionMatrix[viewIndex] = projectionMatrix(viewIndex);

    if (useDepthBuffer()) {
        // this cannot be multiview
        rd->m_projectionMatrix[0](2, 2) = m_zRange;
        rd->m_projectionMatrix[0](2, 3) = calculateElementZOrder(e, m_zRange);
    }

    e->renderNode->prepare();

    renderBatch->batch = batch;
    renderBatch->sms = nullptr;

    return true;
}

void Renderer::renderRhiRenderNode(const Batch *batch)
{
    if (batch->clipState.type & ClipState::StencilClip)
        enqueueStencilDraw(batch);

    RenderNodeElement *e = static_cast<RenderNodeElement *>(batch->first);
    QSGRenderNodePrivate *rd = QSGRenderNodePrivate::get(e->renderNode);

    RenderNodeState state;
    // Expose only the first matrix through the state object, the rest are
    // queriable through the QSGRenderNode getters anyway.
    state.m_projectionMatrix = &rd->m_projectionMatrix[0];
    const std::array<int, 4> scissor = batch->clipState.scissor.scissor();
    state.m_scissorRect = QRect(scissor[0], scissor[1], scissor[2], scissor[3]);
    state.m_stencilValue = batch->clipState.stencilRef;
    state.m_scissorEnabled = batch->clipState.type & ClipState::ScissorClip;
    state.m_stencilEnabled = batch->clipState.type & ClipState::StencilClip;

    const QSGRenderNode::StateFlags changes = e->renderNode->changedStates();

    QRhiCommandBuffer *cb = renderTarget().cb;
    const bool needsExternal = !e->renderNode->flags().testFlag(QSGRenderNode::NoExternalRendering);
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

void Renderer::setVisualizationMode(const QByteArray &mode)
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

bool Renderer::hasVisualizationModeWithContinuousUpdate() const
{
    return m_visualizer->mode() == Visualizer::VisualizeOverdraw;
}

bool operator==(const GraphicsState &a, const GraphicsState &b) noexcept
{
    return a.depthTest == b.depthTest
            && a.depthWrite == b.depthWrite
            && a.depthFunc == b.depthFunc
            && a.blending == b.blending
            && a.srcColor == b.srcColor
            && a.dstColor == b.dstColor
            && a.srcAlpha == b.srcAlpha
            && a.dstAlpha == b.dstAlpha
            && a.opColor == b.opColor
            && a.opAlpha == b.opAlpha
            && a.colorWrite == b.colorWrite
            && a.cullMode == b.cullMode
            && a.usesScissor == b.usesScissor
            && a.stencilTest == b.stencilTest
            && a.sampleCount == b.sampleCount
            && a.drawMode == b.drawMode
            && a.lineWidth == b.lineWidth
            && a.polygonMode == b.polygonMode
            && a.multiViewCount == b.multiViewCount;
}

bool operator!=(const GraphicsState &a, const GraphicsState &b) noexcept
{
    return !(a == b);
}

size_t qHash(const GraphicsState &s, size_t seed) noexcept
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
            + s.sampleCount
            + s.multiViewCount;
}

bool operator==(const GraphicsPipelineStateKey &a, const GraphicsPipelineStateKey &b) noexcept
{
    return a.state == b.state
            && a.sms->materialShader == b.sms->materialShader
            && a.renderTargetDescription == b.renderTargetDescription
            && a.srbLayoutDescription == b.srbLayoutDescription;
}

bool operator!=(const GraphicsPipelineStateKey &a, const GraphicsPipelineStateKey &b) noexcept
{
    return !(a == b);
}

size_t qHash(const GraphicsPipelineStateKey &k, size_t seed) noexcept
{
    return qHash(k.state, seed)
        ^ qHash(k.sms->materialShader)
        ^ k.extra.renderTargetDescriptionHash
        ^ k.extra.srbLayoutDescriptionHash;
}

bool operator==(const ShaderKey &a, const ShaderKey &b) noexcept
{
    return a.type == b.type
           && a.renderMode == b.renderMode
           && a.multiViewCount == b.multiViewCount;
}

bool operator!=(const ShaderKey &a, const ShaderKey &b) noexcept
{
    return !(a == b);
}

size_t qHash(const ShaderKey &k, size_t seed) noexcept
{
    return qHash(k.type, seed) ^ int(k.renderMode) ^ k.multiViewCount;
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
