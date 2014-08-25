/****************************************************************************
**
** Copyright (C) 2014 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com
**
** This file is part of the Qt SceneGraph Raster Add-on.
**
** $QT_BEGIN_LICENSE$
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.
**
** If you have questions regarding the use of this file, please use
** contact form at http://qt.digia.com
** $QT_END_LICENSE$
**
****************************************************************************/
#include "renderingvisitor.h"

#include "imagenode.h"
#include "rectanglenode.h"
#include "glyphnode.h"
#include "ninepatchnode.h"
#include "painternode.h"

#include <QtQuick/QSGSimpleRectNode>

RenderingVisitor::RenderingVisitor(QPainter *painter)
    : painter(painter)
{

}

bool RenderingVisitor::visit(QSGTransformNode *node)
{
    painter->save();
    painter->setTransform(node->matrix().toTransform(), /*combine*/true);
    return true;
}

void RenderingVisitor::endVisit(QSGTransformNode *)
{
    painter->restore();
}

bool RenderingVisitor::visit(QSGClipNode *node)
{
    painter->save();
    painter->setClipRect(node->clipRect(), Qt::IntersectClip);
    return true;
}

void RenderingVisitor::endVisit(QSGClipNode *)
{
    painter->restore();
}

bool RenderingVisitor::visit(QSGGeometryNode *node)
{
    //Check for QSGSimpleRect
    QSGSimpleRectNode *rectNode = 0;
    rectNode = dynamic_cast<QSGSimpleRectNode *>(node);
    if (rectNode) {
        if (!rectNode->material()->flags() & QSGMaterial::Blending)
            painter->setCompositionMode(QPainter::CompositionMode_Source);
        painter->fillRect(rectNode->rect(), rectNode->color());
        painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
    }

    return true;
}

void RenderingVisitor::endVisit(QSGGeometryNode *node)
{

}

bool RenderingVisitor::visit(QSGOpacityNode *node)
{
    painter->save();

    const qreal newOpacity = painter->opacity() * node->opacity();
    if (qFuzzyIsNull(newOpacity))
        return false;

    painter->setOpacity(newOpacity);
    return true;
}

void RenderingVisitor::endVisit(QSGOpacityNode *node)
{
    painter->restore();
}

bool RenderingVisitor::visit(QSGImageNode *node)
{
    static_cast<ImageNode*>(node)->paint(painter);
    return true;
}

void RenderingVisitor::endVisit(QSGImageNode *)
{
}

bool RenderingVisitor::visit(QSGPainterNode *node)
{
    static_cast<PainterNode*>(node)->paint(painter);
    return true;
}

void RenderingVisitor::endVisit(QSGPainterNode *node)
{

}

bool RenderingVisitor::visit(QSGRectangleNode *node)
{
    static_cast<RectangleNode*>(node)->paint(painter);
    return true;
}

void RenderingVisitor::endVisit(QSGRectangleNode *)
{
}

bool RenderingVisitor::visit(QSGGlyphNode *node)
{
    static_cast<GlyphNode*>(node)->paint(painter);
    return true;
}

void RenderingVisitor::endVisit(QSGGlyphNode *)
{
}

bool RenderingVisitor::visit(QSGNinePatchNode *node)
{
    static_cast<NinePatchNode*>(node)->paint(painter);
    return true;
}

void RenderingVisitor::endVisit(QSGNinePatchNode *)
{
}

bool RenderingVisitor::visit(QSGRootNode *)
{
    return true;
}

void RenderingVisitor::endVisit(QSGRootNode *)
{
}
