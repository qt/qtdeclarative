/****************************************************************************
**
** Copyright (C) 2014 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com
**
** This file is part of the Qt Purchasing Add-on.
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

    virtual void visit(QSGTransformNode *node);
    virtual void endVisit(QSGTransformNode *);
    virtual void visit(QSGClipNode *node);
    virtual void endVisit(QSGClipNode *node);
    virtual void visit(QSGGeometryNode *node);
    virtual void endVisit(QSGGeometryNode *node);
    virtual void visit(QSGOpacityNode *node);
    virtual void endVisit(QSGOpacityNode *node);
    virtual void visit(QSGImageNode *node);
    virtual void endVisit(QSGImageNode *node);
    virtual void visit(QSGRectangleNode *node);
    virtual void endVisit(QSGRectangleNode *node);
    virtual void visit(QSGGlyphNode *node);
    virtual void endVisit(QSGGlyphNode *node);
    virtual void visit(QSGNinePatchNode *node);
    virtual void endVisit(QSGNinePatchNode *);

private:
    QPainter *painter;
};

#endif // RENDERINGVISITOR_H
