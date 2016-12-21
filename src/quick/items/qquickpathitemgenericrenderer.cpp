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

#include "qquickpathitemgenericrenderer_p.h"
#include <QtGui/private/qtriangulator_p.h>
#include <QSGVertexColorMaterial>

QT_BEGIN_NAMESPACE

static const qreal SCALE = 100;

struct ColoredVertex // must match QSGGeometry::ColoredPoint2D
{
    float x, y;
    QQuickPathItemGenericRenderer::Color4ub color;
    void set(float nx, float ny, QQuickPathItemGenericRenderer::Color4ub ncolor)
    {
        x = nx; y = ny; color = ncolor;
    }
};

static inline QQuickPathItemGenericRenderer::Color4ub colorToColor4ub(const QColor &c)
{
    QQuickPathItemGenericRenderer::Color4ub color = {
        uchar(qRound(c.redF() * c.alphaF() * 255)),
        uchar(qRound(c.greenF() * c.alphaF() * 255)),
        uchar(qRound(c.blueF() * c.alphaF() * 255)),
        uchar(qRound(c.alphaF() * 255))
    };
    return color;
}

QQuickPathItemGenericRootRenderNode::QQuickPathItemGenericRootRenderNode(QQuickWindow *window,
                                                                         bool hasFill,
                                                                         bool hasStroke)
    : m_fillNode(nullptr),
      m_strokeNode(nullptr)
{
    if (hasFill) {
        m_fillNode = new QQuickPathItemGenericRenderNode(window, this);
        appendChildNode(m_fillNode);
    }
    if (hasStroke) {
        m_strokeNode = new QQuickPathItemGenericRenderNode(window, this);
        appendChildNode(m_strokeNode);
    }
}

QQuickPathItemGenericRootRenderNode::~QQuickPathItemGenericRootRenderNode()
{
}

QQuickPathItemGenericRenderNode::QQuickPathItemGenericRenderNode(QQuickWindow *window,
                                                                 QQuickPathItemGenericRootRenderNode *rootNode)
    : m_geometry(QSGGeometry::defaultAttributes_ColoredPoint2D(), 0, 0),
      m_window(window),
      m_rootNode(rootNode),
      m_material(nullptr)
{
    setGeometry(&m_geometry);
    activateMaterial(MatSolidColor);
}

QQuickPathItemGenericRenderNode::~QQuickPathItemGenericRenderNode()
{
}

void QQuickPathItemGenericRenderNode::activateMaterial(Material m)
{
    switch (m) {
    case MatSolidColor:
        // Use vertexcolor material. Items with different colors remain batchable
        // this way, at the expense of having to provide per-vertex color values.
        if (!m_solidColorMaterial)
            m_solidColorMaterial.reset(QQuickPathItemGenericMaterialFactory::createVertexColor(m_window));
        m_material = m_solidColorMaterial.data();
        break;
    case MatLinearGradient:
        if (!m_linearGradientMaterial)
            m_linearGradientMaterial.reset(QQuickPathItemGenericMaterialFactory::createLinearGradient(m_window, this));
        m_material = m_linearGradientMaterial.data();
        break;
    default:
        qWarning("Unknown material %d", m);
        return;
    }

    if (material() != m_material)
        setMaterial(m_material);
}

// sync, and so triangulation too, happens on the gui thread
void QQuickPathItemGenericRenderer::beginSync()
{
    m_syncDirty = 0;
}

void QQuickPathItemGenericRenderer::setPath(const QQuickPath *path)
{
    m_path = path ? path->path() : QPainterPath();
    m_syncDirty |= DirtyGeom;
}

void QQuickPathItemGenericRenderer::setStrokeColor(const QColor &color)
{
    m_strokeColor = colorToColor4ub(color);
    m_syncDirty |= DirtyColor;
}

void QQuickPathItemGenericRenderer::setStrokeWidth(qreal w)
{
    m_strokeWidth = w;
    if (w >= 0.0f)
        m_pen.setWidthF(w);
    m_syncDirty |= DirtyGeom;
}

void QQuickPathItemGenericRenderer::setFillColor(const QColor &color)
{
    m_fillColor = colorToColor4ub(color);
    m_syncDirty |= DirtyColor;
}

void QQuickPathItemGenericRenderer::setFillRule(QQuickPathItem::FillRule fillRule)
{
    m_fillRule = Qt::FillRule(fillRule);
    m_syncDirty |= DirtyGeom;
}

void QQuickPathItemGenericRenderer::setJoinStyle(QQuickPathItem::JoinStyle joinStyle, int miterLimit)
{
    m_pen.setJoinStyle(Qt::PenJoinStyle(joinStyle));
    m_pen.setMiterLimit(miterLimit);
    m_syncDirty |= DirtyGeom;
}

void QQuickPathItemGenericRenderer::setCapStyle(QQuickPathItem::CapStyle capStyle)
{
    m_pen.setCapStyle(Qt::PenCapStyle(capStyle));
    m_syncDirty |= DirtyGeom;
}

void QQuickPathItemGenericRenderer::setStrokeStyle(QQuickPathItem::StrokeStyle strokeStyle,
                                                   qreal dashOffset, const QVector<qreal> &dashPattern)
{
    m_pen.setStyle(Qt::PenStyle(strokeStyle));
    if (strokeStyle == QQuickPathItem::DashLine) {
        m_pen.setDashPattern(dashPattern);
        m_pen.setDashOffset(dashOffset);
    }
    m_syncDirty |= DirtyGeom;
}

void QQuickPathItemGenericRenderer::setFillGradient(QQuickPathGradient *gradient)
{
    m_fillGradientActive = gradient != nullptr;
    if (gradient) {
        m_fillGradient.stops = gradient->sortedGradientStops();
        m_fillGradient.spread = gradient->spread();
        if (QQuickPathLinearGradient *g  = qobject_cast<QQuickPathLinearGradient *>(gradient)) {
            m_fillGradient.start = QPointF(g->x1(), g->y1());
            m_fillGradient.end = QPointF(g->x2(), g->y2());
        } else {
            Q_UNREACHABLE();
        }
    }
    m_syncDirty |= DirtyFillGradient;
}

void QQuickPathItemGenericRenderer::endSync()
{
    if (!m_syncDirty)
        return;

    // Use a shadow dirty flag in order to avoid losing state in case there are
    // multiple syncs with different dirty flags before we get to
    // updatePathRenderNode() on the render thread (with the gui thread
    // blocked). For our purposes here m_syncDirty is still required since
    // geometry regeneration must only happen when there was an actual change
    // in this particular sync round.
    m_effectiveDirty |= m_syncDirty;

    if (m_path.isEmpty()) {
        m_fillVertices.clear();
        m_fillIndices.clear();
        m_strokeVertices.clear();
        return;
    }

    triangulateFill();
    triangulateStroke();
}

void QQuickPathItemGenericRenderer::triangulateFill()
{
    m_path.setFillRule(m_fillRule);

    const QVectorPath &vp = qtVectorPathForPath(m_path);

    QTriangleSet ts = qTriangulate(vp, QTransform::fromScale(SCALE, SCALE));
    const int vertexCount = ts.vertices.count() / 2; // just a qreal vector with x,y hence the / 2
    m_fillVertices.resize(vertexCount);
    ColoredVertex *vdst = reinterpret_cast<ColoredVertex *>(m_fillVertices.data());
    const qreal *vsrc = ts.vertices.constData();
    for (int i = 0; i < vertexCount; ++i)
        vdst[i].set(vsrc[i * 2] / SCALE, vsrc[i * 2 + 1] / SCALE, m_fillColor);

    m_fillIndices.resize(ts.indices.size());
    quint16 *idst = m_fillIndices.data();
    if (ts.indices.type() == QVertexIndexVector::UnsignedShort) {
        memcpy(idst, ts.indices.data(), m_fillIndices.count() * sizeof(quint16));
    } else {
        const quint32 *isrc = (const quint32 *) ts.indices.data();
        for (int i = 0; i < m_fillIndices.count(); ++i)
            idst[i] = isrc[i];
    }
}

void QQuickPathItemGenericRenderer::triangulateStroke()
{
    const QVectorPath &vp = qtVectorPathForPath(m_path);

    const QRectF clip(0, 0, m_item->width(), m_item->height());
    const qreal inverseScale = 1.0 / SCALE;
    m_stroker.setInvScale(inverseScale);
    if (m_pen.style() == Qt::SolidLine) {
        m_stroker.process(vp, m_pen, clip, 0);
    } else {
        m_dashStroker.setInvScale(inverseScale);
        m_dashStroker.process(vp, m_pen, clip, 0);
        QVectorPath dashStroke(m_dashStroker.points(), m_dashStroker.elementCount(),
                               m_dashStroker.elementTypes(), 0);
        m_stroker.process(dashStroke, m_pen, clip, 0);
    }

    if (!m_stroker.vertexCount()) {
        m_strokeVertices.clear();
        return;
    }

    const int vertexCount = m_stroker.vertexCount() / 2; // just a float vector with x,y hence the / 2
    m_strokeVertices.resize(vertexCount);
    ColoredVertex *vdst = reinterpret_cast<ColoredVertex *>(m_strokeVertices.data());
    const float *vsrc = m_stroker.vertices();
    for (int i = 0; i < vertexCount; ++i)
        vdst[i].set(vsrc[i * 2], vsrc[i * 2 + 1], m_strokeColor);
}

void QQuickPathItemGenericRenderer::setRootNode(QQuickPathItemGenericRootRenderNode *rn)
{
    if (m_rootNode != rn) {
        m_rootNode = rn;
        // Scenegraph nodes can be destroyed and then replaced by new ones over
        // time; hence it is important to mark everything dirty for
        // updatePathRenderNode(). We can assume the renderer has a full sync
        // of the data at this point.
        m_effectiveDirty = DirtyAll;
    }
}

// on the render thread with gui blocked
void QQuickPathItemGenericRenderer::updatePathRenderNode()
{
    if (!m_effectiveDirty || !m_rootNode)
        return;

    if (m_fillColor.a == 0) {
        delete m_rootNode->m_fillNode;
        m_rootNode->m_fillNode = nullptr;
    } else if (!m_rootNode->m_fillNode) {
        m_rootNode->m_fillNode = new QQuickPathItemGenericRenderNode(m_item->window(), m_rootNode);
        if (m_rootNode->m_strokeNode)
            m_rootNode->removeChildNode(m_rootNode->m_strokeNode);
        m_rootNode->appendChildNode(m_rootNode->m_fillNode);
        if (m_rootNode->m_strokeNode)
            m_rootNode->appendChildNode(m_rootNode->m_strokeNode);
        m_effectiveDirty |= DirtyGeom;
    }

    if (m_strokeWidth < 0.0f || m_strokeColor.a == 0) {
        delete m_rootNode->m_strokeNode;
        m_rootNode->m_strokeNode = nullptr;
    } else if (!m_rootNode->m_strokeNode) {
        m_rootNode->m_strokeNode = new QQuickPathItemGenericRenderNode(m_item->window(), m_rootNode);
        m_rootNode->appendChildNode(m_rootNode->m_strokeNode);
        m_effectiveDirty |= DirtyGeom;
    }

    updateFillNode();
    updateStrokeNode();

    m_effectiveDirty = 0;
}

void QQuickPathItemGenericRenderer::updateFillNode()
{
    if (!m_rootNode->m_fillNode)
        return;

    QQuickPathItemGenericRenderNode *n = m_rootNode->m_fillNode;
    QSGGeometry *g = &n->m_geometry;
    if (m_fillVertices.isEmpty()) {
        g->allocate(0, 0);
        n->markDirty(QSGNode::DirtyGeometry);
        return;
    }

    if (m_fillGradientActive) {
        n->activateMaterial(QQuickPathItemGenericRenderNode::MatLinearGradient);
        if (m_effectiveDirty & DirtyFillGradient) {
            // Make a copy of the data that will be accessed by the material on
            // the render thread.
            n->m_fillGradient = m_fillGradient;
            // Gradients are implemented via a texture-based material.
            n->markDirty(QSGNode::DirtyMaterial);
            // stop here if only the gradient changed; no need to touch the geometry
            if (!(m_effectiveDirty & DirtyGeom))
                return;
        }
    } else {
        n->activateMaterial(QQuickPathItemGenericRenderNode::MatSolidColor);
        // fast path for updating only color values when no change in vertex positions
        if ((m_effectiveDirty & DirtyColor) && !(m_effectiveDirty & DirtyGeom)) {
            ColoredVertex *vdst = reinterpret_cast<ColoredVertex *>(g->vertexData());
            for (int i = 0; i < g->vertexCount(); ++i)
                vdst[i].set(vdst[i].x, vdst[i].y, m_fillColor);
            n->markDirty(QSGNode::DirtyGeometry);
            return;
        }
    }

    g->allocate(m_fillVertices.count(), m_fillIndices.count());
    g->setDrawingMode(QSGGeometry::DrawTriangles);
    memcpy(g->vertexData(), m_fillVertices.constData(), g->vertexCount() * g->sizeOfVertex());
    memcpy(g->indexData(), m_fillIndices.constData(), g->indexCount() * g->sizeOfIndex());

    n->markDirty(QSGNode::DirtyGeometry);
}

void QQuickPathItemGenericRenderer::updateStrokeNode()
{
    if (!m_rootNode->m_strokeNode)
        return;
    if (m_effectiveDirty == DirtyFillGradient) // not applicable
        return;

    QQuickPathItemGenericRenderNode *n = m_rootNode->m_strokeNode;
    n->markDirty(QSGNode::DirtyGeometry);

    QSGGeometry *g = &n->m_geometry;
    if (m_strokeVertices.isEmpty()) {
        g->allocate(0, 0);
        return;
    }

    if ((m_effectiveDirty & DirtyColor) && !(m_effectiveDirty & DirtyGeom)) {
        ColoredVertex *vdst = reinterpret_cast<ColoredVertex *>(g->vertexData());
        for (int i = 0; i < g->vertexCount(); ++i)
            vdst[i].set(vdst[i].x, vdst[i].y, m_strokeColor);
        return;
    }

    g->allocate(m_strokeVertices.count(), 0);
    g->setDrawingMode(QSGGeometry::DrawTriangleStrip);
    memcpy(g->vertexData(), m_strokeVertices.constData(), g->vertexCount() * g->sizeOfVertex());
}

QSGMaterial *QQuickPathItemGenericMaterialFactory::createVertexColor(QQuickWindow *window)
{
    QSGRendererInterface::GraphicsApi api = window->rendererInterface()->graphicsApi();

#ifndef QT_NO_OPENGL
    if (api == QSGRendererInterface::OpenGL) // ### so much for "generic"...
        return new QSGVertexColorMaterial;
#endif

    qWarning("Vertex-color material: Unsupported graphics API %d", api);
    return nullptr;
}

QSGMaterial *QQuickPathItemGenericMaterialFactory::createLinearGradient(QQuickWindow *window,
                                                                        QQuickPathItemGenericRenderNode *node)
{
    QSGRendererInterface::GraphicsApi api = window->rendererInterface()->graphicsApi();

#ifndef QT_NO_OPENGL
    if (api == QSGRendererInterface::OpenGL) // ### so much for "generic"...
        return new QQuickPathItemLinearGradientMaterial(node);
#endif

    qWarning("Linear gradient material: Unsupported graphics API %d", api);
    return nullptr;
}

#ifndef QT_NO_OPENGL

QSGMaterialType QQuickPathItemLinearGradientShader::type;

QQuickPathItemLinearGradientShader::QQuickPathItemLinearGradientShader()
{
    setShaderSourceFile(QOpenGLShader::Vertex,
                        QStringLiteral(":/qt-project.org/items/shaders/lineargradient.vert"));
    setShaderSourceFile(QOpenGLShader::Fragment,
                        QStringLiteral(":/qt-project.org/items/shaders/lineargradient.frag"));
}

void QQuickPathItemLinearGradientShader::initialize()
{
    m_opacityLoc = program()->uniformLocation("opacity");
    m_matrixLoc = program()->uniformLocation("matrix");
    m_gradStartLoc = program()->uniformLocation("gradStart");
    m_gradEndLoc = program()->uniformLocation("gradEnd");
}

void QQuickPathItemLinearGradientShader::updateState(const RenderState &state, QSGMaterial *mat, QSGMaterial *)
{
    QQuickPathItemLinearGradientMaterial *m = static_cast<QQuickPathItemLinearGradientMaterial *>(mat);

    if (state.isOpacityDirty())
        program()->setUniformValue(m_opacityLoc, state.opacity());

    if (state.isMatrixDirty())
        program()->setUniformValue(m_matrixLoc, state.combinedMatrix());

    QQuickPathItemGenericRenderNode *node = m->node();
    program()->setUniformValue(m_gradStartLoc, QVector2D(node->m_fillGradient.start));
    program()->setUniformValue(m_gradEndLoc, QVector2D(node->m_fillGradient.end));

    QSGTexture *tx = QQuickPathItemGradientCache::currentCache()->get(node->m_fillGradient);
    tx->bind();
}

char const *const *QQuickPathItemLinearGradientShader::attributeNames() const
{
    static const char *const attr[] = { "vertexCoord", "vertexColor", nullptr };
    return attr;
}

int QQuickPathItemLinearGradientMaterial::compare(const QSGMaterial *other) const
{
    Q_ASSERT(other && type() == other->type());
    const QQuickPathItemLinearGradientMaterial *m = static_cast<const QQuickPathItemLinearGradientMaterial *>(other);

    QQuickPathItemGenericRenderNode *a = node();
    QQuickPathItemGenericRenderNode *b = m->node();
    Q_ASSERT(a && b);
    if (a == b)
        return 0;

    const QQuickPathItemGradientCache::GradientDesc *ga = &a->m_fillGradient;
    const QQuickPathItemGradientCache::GradientDesc *gb = &b->m_fillGradient;
    if (int d = ga->start.x() - gb->start.x())
        return d;
    if (int d = ga->start.y() - gb->start.y())
        return d;
    if (int d = ga->end.x() - gb->end.x())
        return d;
    if (int d = ga->end.y() - gb->end.y())
        return d;

    if (int d = ga->stops.count() - gb->stops.count())
        return d;

    for (int i = 0; i < ga->stops.count(); ++i) {
        if (int d = ga->stops[i].first - gb->stops[i].first)
            return d;
        if (int d = ga->stops[i].second.rgba() - gb->stops[i].second.rgba())
            return d;
    }

    return 0;
}

#endif // QT_NO_OPENGL

QT_END_NAMESPACE
