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

#ifndef RENDERABLENODEUPDATER_H
#define RENDERABLENODEUPDATER_H

#include "renderablenode.h"
#include "abstractsoftwarerenderer.h"

#include <private/qsgadaptationlayer_p.h>

#include <QTransform>
#include <QStack>
#include <QRectF>

namespace SoftwareContext {

class RenderableNodeUpdater : public QSGNodeVisitorEx
{
public:
    RenderableNodeUpdater(AbstractSoftwareRenderer *renderer);
    virtual ~RenderableNodeUpdater();

    bool visit(QSGTransformNode *) override;
    void endVisit(QSGTransformNode *) override;
    bool visit(QSGClipNode *) override;
    void endVisit(QSGClipNode *) override;
    bool visit(QSGGeometryNode *) override;
    void endVisit(QSGGeometryNode *) override;
    bool visit(QSGOpacityNode *) override;
    void endVisit(QSGOpacityNode *) override;
    bool visit(QSGImageNode *) override;
    void endVisit(QSGImageNode *) override;
    bool visit(QSGPainterNode *) override;
    void endVisit(QSGPainterNode *) override;
    bool visit(QSGRectangleNode *) override;
    void endVisit(QSGRectangleNode *) override;
    bool visit(QSGGlyphNode *) override;
    void endVisit(QSGGlyphNode *) override;
    bool visit(QSGNinePatchNode *) override;
    void endVisit(QSGNinePatchNode *) override;
    bool visit(QSGRootNode *) override;
    void endVisit(QSGRootNode *) override;

    void updateNodes(QSGNode *node, bool isNodeRemoved = false);

private:
    struct NodeState {
        float opacity;
        QRectF clip;
        QTransform transform;
        QSGNode *parent;
    };

    NodeState currentState(QSGNode *node) const;

    template<class NODE>
    bool updateRenderableNode(RenderableNode::NodeType type, NODE *node);

    AbstractSoftwareRenderer *m_renderer;
    QStack<float> m_opacityState;
    QStack<QRectF> m_clipState;
    QStack<QTransform> m_transformState;
    QHash<QSGNode*,NodeState> m_stateMap;
};

template<class NODE>
bool RenderableNodeUpdater::updateRenderableNode(RenderableNode::NodeType type, NODE *node)
{
    //Check if we already know about node
    auto renderableNode = m_renderer->renderableNode(node);
    if (renderableNode == nullptr) {
        renderableNode = new RenderableNode(type, node);
        m_renderer->addNodeMapping(node, renderableNode);
    }

    //Update the node
    renderableNode->setTransform(m_transformState.top());
    renderableNode->setOpacity(m_opacityState.top());
    renderableNode->setClipRect(m_clipState.top());

    renderableNode->update();
    m_stateMap[node] = currentState(node);

    return true;
}

} // namespace

#endif // RENDERABLENODEUPDATER_H
