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

#include "qquickpathitemsoftwarerenderer_p.h"
#include <private/qquickpath_p_p.h>

QT_BEGIN_NAMESPACE

void QQuickPathItemSoftwareRenderer::beginSync()
{
    // nothing to do here
}

void QQuickPathItemSoftwareRenderer::setPath(const QQuickPath *path)
{
    m_path = path ? path->path() : QPainterPath();
    m_dirty |= DirtyPath;
}

void QQuickPathItemSoftwareRenderer::setStrokeColor(const QColor &color)
{
    m_pen.setColor(color);
    m_dirty |= DirtyPen;
}

void QQuickPathItemSoftwareRenderer::setStrokeWidth(qreal w)
{
    m_strokeWidth = w;
    if (w >= 0.0f)
        m_pen.setWidthF(w);
    m_dirty |= DirtyPen;
}

void QQuickPathItemSoftwareRenderer::setFillColor(const QColor &color)
{
    m_fillColor = color;
    m_brush.setColor(m_fillColor);
    m_dirty |= DirtyBrush;
}

void QQuickPathItemSoftwareRenderer::setFillRule(QQuickPathItem::FillRule fillRule)
{
    m_fillRule = Qt::FillRule(fillRule);
    m_dirty |= DirtyFillRule;
}

void QQuickPathItemSoftwareRenderer::setJoinStyle(QQuickPathItem::JoinStyle joinStyle, int miterLimit)
{
    m_pen.setJoinStyle(Qt::PenJoinStyle(joinStyle));
    m_pen.setMiterLimit(miterLimit);
    m_dirty |= DirtyPen;
}

void QQuickPathItemSoftwareRenderer::setCapStyle(QQuickPathItem::CapStyle capStyle)
{
    m_pen.setCapStyle(Qt::PenCapStyle(capStyle));
    m_dirty |= DirtyPen;
}

void QQuickPathItemSoftwareRenderer::setStrokeStyle(QQuickPathItem::StrokeStyle strokeStyle,
                                                   qreal dashOffset, const QVector<qreal> &dashPattern)
{
    switch (strokeStyle) {
    case QQuickPathItem::SolidLine:
        m_pen.setStyle(Qt::SolidLine);
        break;
    case QQuickPathItem::DashLine:
        m_pen.setStyle(Qt::CustomDashLine);
        m_pen.setDashPattern(dashPattern);
        m_pen.setDashOffset(dashOffset);
        break;
    default:
        break;
    }
    m_dirty |= DirtyPen;
}

void QQuickPathItemSoftwareRenderer::setFillGradient(QQuickPathGradient *gradient)
{
    if (QQuickPathLinearGradient *linearGradient = qobject_cast<QQuickPathLinearGradient *>(gradient)) {
        QLinearGradient painterGradient(linearGradient->x1(), linearGradient->y1(),
                                        linearGradient->x2(), linearGradient->y2());
        painterGradient.setStops(linearGradient->sortedGradientStops());
        switch (gradient->spread()) {
        case QQuickPathGradient::PadSpread:
            painterGradient.setSpread(QGradient::PadSpread);
            break;
        case QQuickPathGradient::RepeatSpread:
            painterGradient.setSpread(QGradient::RepeatSpread);
            break;
        case QQuickPathGradient::ReflectSpread:
            painterGradient.setSpread(QGradient::ReflectSpread);
            break;
        default:
            break;
        }
        m_brush = QBrush(painterGradient);
    } else {
        m_brush = QBrush(m_fillColor);
    }
    m_dirty |= DirtyBrush;
}

void QQuickPathItemSoftwareRenderer::endSync()
{
    // nothing to do here
}

void QQuickPathItemSoftwareRenderer::setNode(QQuickPathItemSoftwareRenderNode *node)
{
    if (m_node != node) {
        m_node = node;
        // Scenegraph nodes can be destroyed and then replaced by new ones over
        // time; hence it is important to mark everything dirty for
        // updatePathRenderNode(). We can assume the renderer has a full sync
        // of the data at this point.
        m_dirty = DirtyAll;
    }
}

void QQuickPathItemSoftwareRenderer::updatePathRenderNode()
{
    if (!m_dirty)
        return;

    // updatePathRenderNode() can be called several times with different dirty
    // state before render() gets invoked. So accumulate.
    m_node->m_dirty |= m_dirty;

    if (m_dirty & DirtyPath) {
        m_node->m_path = m_path;
        m_node->m_path.setFillRule(m_fillRule);
    }

    if (m_dirty & DirtyFillRule)
        m_node->m_path.setFillRule(m_fillRule);

    if (m_dirty & DirtyPen) {
        m_node->m_pen = m_pen;
        m_node->m_strokeWidth = m_strokeWidth;
    }

    if (m_dirty & DirtyBrush)
        m_node->m_brush = m_brush;

    m_node->markDirty(QSGNode::DirtyMaterial);
    m_dirty = 0;
}

QQuickPathItemSoftwareRenderNode::QQuickPathItemSoftwareRenderNode(QQuickPathItem *item)
    : m_item(item)
{
}

QQuickPathItemSoftwareRenderNode::~QQuickPathItemSoftwareRenderNode()
{
    releaseResources();
}

void QQuickPathItemSoftwareRenderNode::releaseResources()
{
}

void QQuickPathItemSoftwareRenderNode::render(const RenderState *state)
{
    if (m_path.isEmpty())
        return;

    QSGRendererInterface *rif = m_item->window()->rendererInterface();
    QPainter *p = static_cast<QPainter *>(rif->getResource(m_item->window(), QSGRendererInterface::PainterResource));
    Q_ASSERT(p);

    const QRegion *clipRegion = state->clipRegion();
    if (clipRegion && !clipRegion->isEmpty())
        p->setClipRegion(*clipRegion, Qt::ReplaceClip); // must be done before setTransform

    p->setTransform(matrix()->toTransform());
    p->setOpacity(inheritedOpacity());

    p->setPen(m_strokeWidth >= 0.0f && m_pen.color() != Qt::transparent ? m_pen : Qt::NoPen);
    p->setBrush(m_brush.color() != Qt::transparent ? m_brush : Qt::NoBrush);
    p->drawPath(m_path);

    m_dirty = 0;
}

QSGRenderNode::StateFlags QQuickPathItemSoftwareRenderNode::changedStates() const
{
    return 0;
}

QSGRenderNode::RenderingFlags QQuickPathItemSoftwareRenderNode::flags() const
{
    return BoundedRectRendering; // avoid fullscreen updates by saying we won't draw outside rect()
}

QRectF QQuickPathItemSoftwareRenderNode::rect() const
{
    return QRect(0, 0, m_item->width(), m_item->height());
}

QT_END_NAMESPACE
