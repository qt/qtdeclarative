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
#ifndef RENDERINGVISITOR_H
#define RENDERINGVISITOR_H

#include <private/qsgadaptationlayer_p.h>

class RenderingVisitor : public QSGNodeVisitorEx
{
public:
    RenderingVisitor(QPainter *painter);

    virtual bool visit(QSGTransformNode *node);
    virtual void endVisit(QSGTransformNode *);
    virtual bool visit(QSGClipNode *node);
    virtual void endVisit(QSGClipNode *node);
    virtual bool visit(QSGGeometryNode *node);
    virtual void endVisit(QSGGeometryNode *node);
    virtual bool visit(QSGOpacityNode *node);
    virtual void endVisit(QSGOpacityNode *node);
    virtual bool visit(QSGImageNode *node);
    virtual void endVisit(QSGImageNode *node);
    virtual bool visit(QSGRectangleNode *node);
    virtual void endVisit(QSGRectangleNode *node);
    virtual bool visit(QSGGlyphNode *node);
    virtual void endVisit(QSGGlyphNode *node);
    virtual bool visit(QSGNinePatchNode *node);
    virtual void endVisit(QSGNinePatchNode *);
    virtual bool visit(QSGRootNode *);
    virtual void endVisit(QSGRootNode *);

private:
    QPainter *painter;
};

#endif // RENDERINGVISITOR_H
