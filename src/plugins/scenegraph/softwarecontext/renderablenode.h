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

#ifndef RENDERABLENODE_H
#define RENDERABLENODE_H

#include <QtGui/QRegion>
#include <QtCore/QRect>
#include <QtGui/QTransform>

class QSGNode;
class QSGSimpleRectNode;
class QSGSimpleTextureNode;
class ImageNode;
class PainterNode;
class RectangleNode;
class GlyphNode;
class NinePatchNode;

namespace SoftwareContext{

class RenderableNode
{
public:
    enum NodeType {
        Invalid = -1,
        SimpleRect,
        SimpleTexture,
        Image,
        Painter,
        Rectangle,
        Glyph,
        NinePatch
    };

    RenderableNode(NodeType type, QSGNode *node);
    ~RenderableNode();

    void update();

    QRegion renderNode(QPainter *painter, bool forceOpaquePainting = false);
    QRect boundingRect() const;
    NodeType type() const { return m_nodeType; }
    bool isOpaque() const { return m_isOpaque; }
    bool isDirty() const { return m_isDirty; }
    bool isDirtyRegionEmpty() const;

    void setTransform(const QTransform &transform);
    void setClipRect(const QRectF &clipRect);
    void setOpacity(float opacity);
    QTransform transform() const { return m_transform; }
    QRectF clipRect() const { return m_clipRect; }
    float opacity() const { return m_opacity; }

    void markGeometryDirty();
    void markMaterialDirty();

    void addDirtyRegion(const QRegion &dirtyRegion, bool forceDirty = true);
    void subtractDirtyRegion(const QRegion &dirtyRegion);

    QRegion previousDirtyRegion() const;
    QRegion dirtyRegion() const;

private:
    union RenderableNodeHandle {
        QSGSimpleRectNode *simpleRectNode;
        QSGSimpleTextureNode *simpleTextureNode;
        ImageNode *imageNode;
        PainterNode *painterNode;
        RectangleNode *rectangleNode;
        GlyphNode *glpyhNode;
        NinePatchNode *ninePatchNode;
    };

    const NodeType m_nodeType;
    RenderableNodeHandle m_handle;

    bool m_isOpaque;

    bool m_isDirty;
    QRegion m_dirtyRegion;
    QRegion m_previousDirtyRegion;

    QTransform m_transform;
    QRectF m_clipRect;
    float m_opacity;

    QRect m_boundingRect;
};

} // namespace

#endif // RENDERABLENODE_H
