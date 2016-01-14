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

#ifndef RENDERINGVISITOR_H
#define RENDERINGVISITOR_H

#include <private/qsgadaptationlayer_p.h>

class RenderingVisitor : public QSGNodeVisitorEx
{
public:
    RenderingVisitor(QPainter *painter);

    bool visit(QSGTransformNode *node) override;
    void endVisit(QSGTransformNode *) override;
    bool visit(QSGClipNode *node) override;
    void endVisit(QSGClipNode *node) override;
    bool visit(QSGGeometryNode *node) override;
    void endVisit(QSGGeometryNode *node) override;
    bool visit(QSGOpacityNode *node) override;
    void endVisit(QSGOpacityNode *node) override;
    bool visit(QSGImageNode *node) override;
    void endVisit(QSGImageNode *node) override;
    bool visit(QSGPainterNode *node) override;
    void endVisit(QSGPainterNode *) override;
    bool visit(QSGRectangleNode *node) override;
    void endVisit(QSGRectangleNode *node) override;
    bool visit(QSGGlyphNode *node) override;
    void endVisit(QSGGlyphNode *node) override;
    bool visit(QSGNinePatchNode *node) override;
    void endVisit(QSGNinePatchNode *) override;
    bool visit(QSGRootNode *) override;
    void endVisit(QSGRootNode *) override;

private:
    QPainter *painter;
};

#endif // RENDERINGVISITOR_H
