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

#ifndef ABSTRACTSOFTWARERENDERER_H
#define ABSTRACTSOFTWARERENDERER_H

#include <private/qsgrenderer_p.h>

#include <QtCore/QHash>
#include <QtCore/QLinkedList>

class QSGSimpleRectNode;

namespace SoftwareContext{

class RenderableNode;
class RenderableNodeUpdater;

class AbstractSoftwareRenderer : public QSGRenderer
{
public:
    AbstractSoftwareRenderer(QSGRenderContext *context);
    virtual ~AbstractSoftwareRenderer();

    RenderableNode *renderableNode(QSGNode *node) const;
    void addNodeMapping(QSGNode *node, RenderableNode *renderableNode);
    void appendRenderableNode(RenderableNode *node);

    void nodeChanged(QSGNode *node, QSGNode::DirtyState state) override;

protected:
    QRegion renderNodes(QPainter *painter);
    void buildRenderList();
    void optimizeRenderList();

    void setBackgroundColor(const QColor &color);
    void setBackgroundSize(const QSize &size);
    QColor backgroundColor();
    QSize backgroundSize();

private:
    void nodeAdded(QSGNode *node);
    void nodeRemoved(QSGNode *node);
    void nodeGeometryUpdated(QSGNode *node);
    void nodeMaterialUpdated(QSGNode *node);
    void nodeMatrixUpdated(QSGNode *node);
    void nodeOpacityUpdated(QSGNode *node);

    QHash<QSGNode*, RenderableNode*> m_nodes;
    QLinkedList<RenderableNode*> m_renderableNodes;

    QSGSimpleRectNode *m_background;

    QRegion m_dirtyRegion;
    QRegion m_obscuredRegion;

    RenderableNodeUpdater *m_nodeUpdater;
};

} // namespace

#endif // ABSTRACTSOFTWARERENDERER_H
