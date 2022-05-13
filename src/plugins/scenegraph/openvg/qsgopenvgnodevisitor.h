// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSGOPENVGNODEVISITOR_H
#define QSGOPENVGNODEVISITOR_H

#include <private/qsgadaptationlayer_p.h>
#include <QtCore/QStack>

#include "qopenvgmatrix.h"

#include <VG/openvg.h>

QT_BEGIN_NAMESPACE

class QSGOpenVGRenderable;
class QSGOpenVGNodeVisitor : public QSGNodeVisitorEx
{
public:
    QSGOpenVGNodeVisitor();

    bool visit(QSGTransformNode *) override;
    void endVisit(QSGTransformNode *) override;
    bool visit(QSGClipNode *) override;
    void endVisit(QSGClipNode *) override;
    bool visit(QSGGeometryNode *) override;
    void endVisit(QSGGeometryNode *) override;
    bool visit(QSGOpacityNode *) override;
    void endVisit(QSGOpacityNode *) override;
    bool visit(QSGInternalImageNode *) override;
    void endVisit(QSGInternalImageNode *) override;
    bool visit(QSGPainterNode *) override;
    void endVisit(QSGPainterNode *) override;
    bool visit(QSGInternalRectangleNode *) override;
    void endVisit(QSGInternalRectangleNode *) override;
    bool visit(QSGGlyphNode *) override;
    void endVisit(QSGGlyphNode *) override;
    bool visit(QSGRootNode *) override;
    void endVisit(QSGRootNode *) override;
#if QT_CONFIG(quick_sprite)
    bool visit(QSGSpriteNode *) override;
    void endVisit(QSGSpriteNode *) override;
#endif
    bool visit(QSGRenderNode *) override;
    void endVisit(QSGRenderNode *) override;

private:
    VGPath generateClipPath(const QRectF &rect) const;
    void renderRenderableNode(QSGOpenVGRenderable *node);

    QStack<QOpenVGMatrix> m_transformStack;
    QStack<float> m_opacityState;
    QStack<VGPath> m_clipStack;
};

QT_END_NAMESPACE

#endif // QSGOPENVGNODEVISITOR_H
