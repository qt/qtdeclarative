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
    virtual void visit(QSGSimpleRectangleNode *node);
    virtual void endVisit(QSGSimpleRectangleNode *node);
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
