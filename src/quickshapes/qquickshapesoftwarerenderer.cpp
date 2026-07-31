// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qquickshapesoftwarerenderer_p.h"
#include <private/qquickpath_p_p.h>

QT_BEGIN_NAMESPACE

void QQuickShapeSoftwareRenderer::beginSync(int totalCount, bool *countChanged)
{
    if (m_sp.size() != totalCount) {
        m_sp.resize(totalCount);
        m_accDirty |= DirtyList;
        *countChanged = true;
    } else {
        *countChanged = false;
    }
}

void QQuickShapeSoftwareRenderer::setPath(int index, const QPainterPath &path, QQuickShapePath::PathHints)
{
    ShapePathGuiData &d(m_sp[index]);
    d.path = path;
    d.dirty |= DirtyPath;
    m_accDirty |= DirtyPath;
}

void QQuickShapeSoftwareRenderer::setStrokeColor(int index, const QColor &color)
{
    ShapePathGuiData &d(m_sp[index]);
    d.pen.setColor(color);
    d.dirty |= DirtyPen;
    m_accDirty |= DirtyPen;
}

void QQuickShapeSoftwareRenderer::setStrokeWidth(int index, qreal w)
{
    ShapePathGuiData &d(m_sp[index]);
    d.strokeWidth = w;
    if (w > 0.0f)
        d.pen.setWidthF(w);
    d.dirty |= DirtyPen;
    m_accDirty |= DirtyPen;
}

void QQuickShapeSoftwareRenderer::setCosmeticStroke(int index, bool c)
{
    ShapePathGuiData &d(m_sp[index]);
    d.pen.setCosmetic(c);
    d.dirty |= DirtyPen;
    m_accDirty |= DirtyPen;
}

void QQuickShapeSoftwareRenderer::setFillColor(int index, const QColor &color)
{
    ShapePathGuiData &d(m_sp[index]);
    d.fillColor = color;
    d.brush.setColor(color);
    d.dirty |= DirtyBrush;
    m_accDirty |= DirtyBrush;
}

void QQuickShapeSoftwareRenderer::setVisible(int index, bool visible)
{
    ShapePathGuiData &d(m_sp[index]);
    d.visible = visible;
    d.dirty |= DirtyVisible;
    m_accDirty |= DirtyVisible;
}

void QQuickShapeSoftwareRenderer::setFillRule(int index, QQuickShapePath::FillRule fillRule)
{
    ShapePathGuiData &d(m_sp[index]);
    d.fillRule = Qt::FillRule(fillRule);
    d.dirty |= DirtyFillRule;
    m_accDirty |= DirtyFillRule;
}

void QQuickShapeSoftwareRenderer::setJoinStyle(int index, QQuickShapePath::JoinStyle joinStyle, int miterLimit)
{
    ShapePathGuiData &d(m_sp[index]);
    d.pen.setJoinStyle(Qt::PenJoinStyle(joinStyle));
    d.pen.setMiterLimit(miterLimit);
    d.dirty |= DirtyPen;
    m_accDirty |= DirtyPen;
}

void QQuickShapeSoftwareRenderer::setCapStyle(int index, QQuickShapePath::CapStyle capStyle)
{
    ShapePathGuiData &d(m_sp[index]);
    d.pen.setCapStyle(Qt::PenCapStyle(capStyle));
    d.dirty |= DirtyPen;
    m_accDirty |= DirtyPen;
}

void QQuickShapeSoftwareRenderer::setStrokeStyle(int index, QQuickShapePath::StrokeStyle strokeStyle,
                                                    qreal dashOffset, const QList<qreal> &dashPattern)
{
    ShapePathGuiData &d(m_sp[index]);
    switch (strokeStyle) {
    case QQuickShapePath::SolidLine:
        d.pen.setStyle(Qt::SolidLine);
        break;
    case QQuickShapePath::DashLine:
        d.pen.setStyle(Qt::CustomDashLine);
        d.pen.setDashPattern(dashPattern);
        d.pen.setDashOffset(dashOffset);
        break;
    default:
        break;
    }
    d.dirty |= DirtyPen;
    m_accDirty |= DirtyPen;
}

static inline void setupPainterGradient(QGradient *painterGradient, const QQuickShapeGradient &g)
{
    painterGradient->setStops(g.gradientStops()); // sorted
    switch (g.spread()) {
    case QQuickShapeGradient::PadSpread:
        painterGradient->setSpread(QGradient::PadSpread);
        break;
    case QQuickShapeGradient::RepeatSpread:
        painterGradient->setSpread(QGradient::RepeatSpread);
        break;
    case QQuickShapeGradient::ReflectSpread:
        painterGradient->setSpread(QGradient::ReflectSpread);
        break;
    default:
        break;
    }
}

static QBrush copyGradient(const QQuickShapeGradient *gradient, const QColor &color)
{
    if (const QQuickShapeLinearGradient *g = qobject_cast<const QQuickShapeLinearGradient *>(gradient)) {
        QLinearGradient painterGradient(g->x1(), g->y1(), g->x2(), g->y2());
        setupPainterGradient(&painterGradient, *g);
        return QBrush(painterGradient);
    } else if (const QQuickShapeRadialGradient *g = qobject_cast<const QQuickShapeRadialGradient *>(gradient)) {
        QRadialGradient painterGradient(g->centerX(), g->centerY(), g->centerRadius(),
                                        g->focalX(), g->focalY(), g->focalRadius());
        setupPainterGradient(&painterGradient, *g);
        return QBrush(painterGradient);
    } else if (const QQuickShapeConicalGradient *g = qobject_cast<const QQuickShapeConicalGradient *>(gradient)) {
        QConicalGradient painterGradient(g->centerX(), g->centerY(), g->angle());
        setupPainterGradient(&painterGradient, *g);
        return QBrush(painterGradient);
    } else {
        return QBrush(color);
    }
}

void QQuickShapeSoftwareRenderer::setFillGradient(int index, QQuickShapeGradient *gradient)
{
    ShapePathGuiData &d(m_sp[index]);
    d.brush = copyGradient(gradient, d.fillColor);
    d.dirty |= DirtyBrush;
    m_accDirty |= DirtyBrush;
}

void QQuickShapeSoftwareRenderer::setStrokeGradient(int index, QQuickShapeGradient *gradient)
{
    ShapePathGuiData &d(m_sp[index]);
    d.pen.setBrush(copyGradient(gradient, d.pen.color()));
    d.dirty |= DirtyPen;
    m_accDirty |= DirtyPen;
}

void QQuickShapeSoftwareRenderer::setFillTextureProvider(int index, QQuickItem *textureProviderItem)
{
    Q_UNUSED(index);
    Q_UNUSED(textureProviderItem);
}

void QQuickShapeSoftwareRenderer::handleSceneChange(QQuickWindow *window)
{
    Q_UNUSED(window);
    // No action needed
}

void QQuickShapeSoftwareRenderer::setFillTransform(int index, const QSGTransform &transform)
{
    ShapePathGuiData &d(m_sp[index]);
    if (!(transform.isIdentity() && d.brush.transform().isIdentity())) // No need to copy if both==I
        d.brush.setTransform(transform.matrix().toTransform());
    d.dirty |= DirtyBrush;
    m_accDirty |= DirtyBrush;
}

void QQuickShapeSoftwareRenderer::setTriangulationScale(int index, qreal scale)
{
    ShapePathGuiData &d(m_sp[index]);
    d.triangulationScale = scale;
    d.dirty |= DirtyPen;
    m_accDirty |= DirtyPen;
}

void QQuickShapeSoftwareRenderer::endSync(bool)
{
}

void QQuickShapeSoftwareRenderer::setNode(QQuickShapeSoftwareRenderNode *node)
{
    m_node = node;
    m_accDirty |= DirtyList;
}

void QQuickShapeSoftwareRenderer::updateNode()
{
    if (!m_accDirty)
        return;

    const int count = m_sp.size();
    const bool listChanged = m_accDirty & DirtyList;
    if (listChanged)
        m_node->m_sp.resize(count);

    m_node->m_boundingRect = QRectF();

    for (int i = 0; i < count; ++i) {
        ShapePathGuiData &src(m_sp[i]);
        QQuickShapeSoftwareRenderNode::ShapePathRenderData &dst(m_node->m_sp[i]);

        if (listChanged || (src.dirty & DirtyPath)) {
            dst.path = src.path;
            dst.path.setFillRule(src.fillRule);
        }

        if (listChanged || (src.dirty & DirtyFillRule))
            dst.path.setFillRule(src.fillRule);

        if (listChanged || (src.dirty & DirtyPen)) {
            dst.pen = src.pen;
            dst.strokeWidth = src.strokeWidth;
            dst.triangulationScale = src.triangulationScale;
        }

        if (listChanged || (src.dirty & DirtyBrush))
            dst.brush = src.brush;

        if (listChanged || (src.dirty & DirtyVisible))
            dst.visible = src.visible;

        src.dirty = 0;

        if (!dst.visible)
            continue;

        QRectF br = dst.path.boundingRect();
        float sw = qMax(1.0f, dst.strokeWidth);
        if (dst.pen.isCosmetic())
            sw *= 2.0f / dst.triangulationScale;
        br.adjust(-sw, -sw, sw, sw);
        m_node->m_boundingRect |= br;
    }

    m_node->markDirty(QSGNode::DirtyMaterial);
    m_accDirty = 0;
}

QQuickShapeSoftwareRenderNode::QQuickShapeSoftwareRenderNode(QQuickShape *item)
    : m_item(item)
{
}

QQuickShapeSoftwareRenderNode::~QQuickShapeSoftwareRenderNode()
{
    releaseResources();
}

void QQuickShapeSoftwareRenderNode::releaseResources()
{
}

void QQuickShapeSoftwareRenderNode::render(const RenderState *state)
{
    if (m_sp.isEmpty())
        return;

    QSGRendererInterface *rif = m_item->window()->rendererInterface();
    QPainter *p = static_cast<QPainter *>(rif->getResource(m_item->window(), QSGRendererInterface::PainterResource));
    Q_ASSERT(p);

    const QRegion *clipRegion = state->clipRegion();
    if (clipRegion && !clipRegion->isEmpty())
        p->setClipRegion(*clipRegion, Qt::ReplaceClip); // must be done before setTransform

    p->setTransform(matrix()->toTransform());
    p->setOpacity(inheritedOpacity());

    for (const ShapePathRenderData &d : std::as_const(m_sp)) {
        if (!d.visible)
            continue;
        p->setPen(d.strokeWidth > 0.0f && d.pen.color() != Qt::transparent ? d.pen : Qt::NoPen);
        p->setBrush(d.brush.color() != Qt::transparent ? d.brush : Qt::NoBrush);
        p->drawPath(d.path);
    }
}

QSGRenderNode::StateFlags QQuickShapeSoftwareRenderNode::changedStates() const
{
    return {};
}

QSGRenderNode::RenderingFlags QQuickShapeSoftwareRenderNode::flags() const
{
    return BoundedRectRendering; // avoid fullscreen updates by saying we won't draw outside rect()
}

QRectF QQuickShapeSoftwareRenderNode::rect() const
{
    return m_boundingRect;
}

QT_END_NAMESPACE
