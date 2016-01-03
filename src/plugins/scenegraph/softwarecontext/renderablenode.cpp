/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick 2D Renderer module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "renderablenode.h"

#include "imagenode.h"
#include "rectanglenode.h"
#include "glyphnode.h"
#include "ninepatchnode.h"
#include "painternode.h"
#include "pixmaptexture.h"

#include <QtQuick/QSGSimpleRectNode>
#include <QtQuick/qsgsimpletexturenode.h>
#include <private/qsgtexture_p.h>
#include <private/qquickshadereffectnode_p.h>

Q_LOGGING_CATEGORY(lcRenderable, "qt.scenegraph.softwarecontext.renderable")

namespace SoftwareContext{

RenderableNode::RenderableNode(NodeType type, QSGNode *node)
    : m_nodeType(type)
    , m_isOpaque(true)
    , m_isDirty(true)
    , m_opacity(1.0f)
{
    switch (m_nodeType) {
    case RenderableNode::SimpleRect:
        m_handle.simpleRectNode = static_cast<QSGSimpleRectNode*>(node);
        break;
    case RenderableNode::SimpleTexture:
        m_handle.simpleTextureNode = static_cast<QSGSimpleTextureNode*>(node);
        break;
    case RenderableNode::Image:
        m_handle.imageNode = static_cast<ImageNode*>(node);
        break;
    case RenderableNode::Painter:
        m_handle.painterNode = static_cast<PainterNode*>(node);
        break;
    case RenderableNode::Rectangle:
        m_handle.rectangleNode = static_cast<RectangleNode*>(node);
        break;
    case RenderableNode::Glyph:
        m_handle.glpyhNode = static_cast<GlyphNode*>(node);
        break;
    case RenderableNode::NinePatch:
        m_handle.ninePatchNode = static_cast<NinePatchNode*>(node);
        break;
    case RenderableNode::Invalid:
        m_handle.simpleRectNode = nullptr;
        break;
    }
}

RenderableNode::~RenderableNode()
{

}

void RenderableNode::update()
{
    // Update the Node properties
    m_isDirty = true;

    QRect boundingRect;

    switch (m_nodeType) {
    case RenderableNode::SimpleRect:
        if (m_handle.simpleRectNode->color().alpha() == 255 && !m_transform.isRotating())
            m_isOpaque = true;
        else
            m_isOpaque = false;

        boundingRect = m_handle.simpleRectNode->rect().toRect();
        break;
    case RenderableNode::SimpleTexture:
        if (!m_handle.simpleTextureNode->texture()->hasAlphaChannel() && !m_transform.isRotating())
            m_isOpaque = true;
        else
            m_isOpaque = false;

        boundingRect = m_handle.simpleTextureNode->rect().toRect();
        break;
    case RenderableNode::Image:
        // There isn't a way to tell, so assume it's not
        m_isOpaque = false;

        boundingRect = m_handle.imageNode->rect().toRect();
        break;
    case RenderableNode::Painter:
        if (m_handle.painterNode->opaquePainting() && !m_transform.isRotating())
            m_isOpaque = true;
        else
            m_isOpaque = false;

        boundingRect = QRect(0, 0, m_handle.painterNode->size().width(), m_handle.painterNode->size().height());
        break;
    case RenderableNode::Rectangle:
        if (m_handle.rectangleNode->isOpaque() && !m_transform.isRotating())
            m_isOpaque = true;
        else
            m_isOpaque = false;

        boundingRect = m_handle.rectangleNode->rect().toRect();
        break;
    case RenderableNode::Glyph:
        // Always has alpha
        m_isOpaque = false;

        boundingRect = m_handle.glpyhNode->boundingRect().toAlignedRect();
        break;
    case RenderableNode::NinePatch:
        // Difficult to tell, assume non-opaque
        m_isOpaque = false;

        boundingRect = m_handle.ninePatchNode->bounds().toRect();
        break;
    default:
        break;
    }

    m_boundingRect = m_transform.mapRect(boundingRect);

    if (m_clipRect.isValid()) {
        m_boundingRect = m_boundingRect.intersected(m_clipRect.toRect());
    }

    // Overrides
    if (m_opacity < 1.0f)
        m_isOpaque = false;

    m_dirtyRegion = QRegion(m_boundingRect);
}

QRegion RenderableNode::renderNode(QPainter *painter, bool forceOpaquePainting)
{
    Q_ASSERT(painter);

    // Check for don't paint conditions
    if (!m_isDirty || qFuzzyIsNull(m_opacity) || m_dirtyRegion.isEmpty()) {
        m_isDirty = false;
        m_dirtyRegion = QRegion();
        return QRegion();
    }

    painter->save();
    painter->setOpacity(m_opacity);

    // Set clipRegion to m_dirtyRegion (in world coordinates)
    // as m_dirtyRegion already accounts for clipRegion
    painter->setClipRegion(m_dirtyRegion, Qt::ReplaceClip);

    painter->setTransform(m_transform, false); //precalculated worldTransform
    if (forceOpaquePainting || m_isOpaque)
        painter->setCompositionMode(QPainter::CompositionMode_Source);

    switch (m_nodeType) {
    case RenderableNode::SimpleRect:
        painter->fillRect(m_handle.simpleRectNode->rect(), m_handle.simpleRectNode->color());
        break;
    case RenderableNode::SimpleTexture:
    {
        QSGTexture *texture = m_handle.simpleTextureNode->texture();
        if (PixmapTexture *pt = dynamic_cast<PixmapTexture *>(texture)) {
            const QPixmap &pm = pt->pixmap();
            painter->drawPixmap(m_handle.simpleTextureNode->rect(), pm, QRectF(0, 0, pm.width(), pm.height()));
        } else if (QSGPlainTexture *pt = dynamic_cast<QSGPlainTexture *>(texture)) {
            const QImage &im = pt->image();
            painter->drawImage(m_handle.simpleTextureNode->rect(), im, QRectF(0, 0, im.width(), im.height()));
        }
    }
        break;
    case RenderableNode::Image:
        m_handle.imageNode->paint(painter);
        break;
    case RenderableNode::Painter:
        m_handle.painterNode->paint(painter);
        break;
    case RenderableNode::Rectangle:
        m_handle.rectangleNode->paint(painter);
        break;
    case RenderableNode::Glyph:
        m_handle.glpyhNode->paint(painter);
        break;
    case RenderableNode::NinePatch:
        m_handle.ninePatchNode->paint(painter);
        break;
    default:
        break;
    }

    painter->restore();

    QRegion areaToBeFlushed = m_dirtyRegion;
    m_previousDirtyRegion = QRegion(m_boundingRect);
    m_isDirty = false;
    m_dirtyRegion = QRegion();

    return areaToBeFlushed;
}

QRect RenderableNode::boundingRect() const
{
    // This returns the bounding area of a renderable node in world coordinates
    return m_boundingRect;
}

bool RenderableNode::isDirtyRegionEmpty() const
{
    return m_dirtyRegion.isEmpty();
}

void RenderableNode::setTransform(const QTransform &transform)
{
    if (m_transform == transform)
        return;
    m_transform = transform;
    update();
}

void RenderableNode::setClipRect(const QRectF &clipRect)
{
    if (m_clipRect == clipRect)
        return;

    m_clipRect = clipRect;
    update();
}

void RenderableNode::setOpacity(float opacity)
{
    if (qFuzzyCompare(m_opacity, opacity))
        return;

    m_opacity = opacity;
    update();
}

void RenderableNode::markGeometryDirty()
{
    update();
}

void RenderableNode::markMaterialDirty()
{
    update();
}

void RenderableNode::addDirtyRegion(const QRegion &dirtyRegion, bool forceDirty)
{
    // Check if the dirty region applys to this node
    QRegion prev = m_dirtyRegion;
    if (dirtyRegion.intersects(boundingRect())) {
        if (forceDirty)
            m_isDirty = true;
        m_dirtyRegion += dirtyRegion.intersected(boundingRect());
    }
    qCDebug(lcRenderable) << "addDirtyRegion: " << dirtyRegion << "old dirtyRegion: " << prev << "new dirtyRegion: " << m_dirtyRegion;
}

void RenderableNode::subtractDirtyRegion(const QRegion &dirtyRegion)
{
    QRegion prev = m_dirtyRegion;
    if (m_isDirty) {
        // Check if this rect concerns us
        if (dirtyRegion.intersects(QRegion(boundingRect()))) {
            m_dirtyRegion -= dirtyRegion;
            if (m_dirtyRegion.isEmpty())
                m_isDirty = false;
        }
    }
    qCDebug(lcRenderable) << "subtractDirtyRegion: " << dirtyRegion << "old dirtyRegion" << prev << "new dirtyRegion: " << m_dirtyRegion;
}

QRegion RenderableNode::previousDirtyRegion() const
{
    return m_previousDirtyRegion.subtracted(QRegion(m_boundingRect));
}

QRegion RenderableNode::dirtyRegion() const
{
    return m_dirtyRegion;
}

} // namespace
