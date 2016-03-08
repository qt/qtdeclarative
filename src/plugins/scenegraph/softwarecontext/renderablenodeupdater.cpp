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

#include "renderablenodeupdater.h"

#include "abstractsoftwarerenderer.h"
#include "imagenode.h"
#include "rectanglenode.h"
#include "glyphnode.h"
#include "ninepatchnode.h"
#include "painternode.h"
#include "pixmaptexture.h"

#include <QtQuick/QSGSimpleRectNode>
#include <QtQuick/qsgsimpletexturenode.h>

QT_BEGIN_NAMESPACE

namespace SoftwareContext {

RenderableNodeUpdater::RenderableNodeUpdater(AbstractSoftwareRenderer *renderer)
    : m_renderer(renderer)
{
    m_opacityState.push(1.0f);
    // Invalid RectF by default for no clip
    m_clipState.push(QRectF(0.0f, 0.0f, -1.0f, -1.0f));
    m_transformState.push(QTransform());
}

RenderableNodeUpdater::~RenderableNodeUpdater()
{

}

bool RenderableNodeUpdater::visit(QSGTransformNode *node)
{
    m_transformState.push(node->matrix().toTransform() * m_transformState.top());
    m_stateMap[node] = currentState(node);
    return true;
}

void RenderableNodeUpdater::endVisit(QSGTransformNode *)
{
    m_transformState.pop();
}

bool RenderableNodeUpdater::visit(QSGClipNode *node)
{
    // Make sure to translate the clip rect into world coordinates
    if (!m_clipState.top().isValid())
        m_clipState.push(m_transformState.top().mapRect(node->clipRect()));
    else
        m_clipState.push(m_transformState.top().mapRect(node->clipRect()).intersected(m_clipState.top()));
    m_stateMap[node] = currentState(node);
    return true;
}

void RenderableNodeUpdater::endVisit(QSGClipNode *)
{
    m_clipState.pop();
}

bool RenderableNodeUpdater::visit(QSGGeometryNode *node)
{
    if (QSGSimpleRectNode *rectNode = dynamic_cast<QSGSimpleRectNode *>(node)) {
        return updateRenderableNode(RenderableNode::SimpleRect, rectNode);
    } else if (QSGSimpleTextureNode *tn = dynamic_cast<QSGSimpleTextureNode *>(node)) {
        return updateRenderableNode(RenderableNode::SimpleTexture, tn);
    } else {
        // We dont know, so skip
        return false;
    }
}

void RenderableNodeUpdater::endVisit(QSGGeometryNode *)
{
}

bool RenderableNodeUpdater::visit(QSGOpacityNode *node)
{
    m_opacityState.push(m_opacityState.top() * node->opacity());
    m_stateMap[node] = currentState(node);
    return true;
}

void RenderableNodeUpdater::endVisit(QSGOpacityNode *)
{
    m_opacityState.pop();
}

bool RenderableNodeUpdater::visit(QSGImageNode *node)
{
    return updateRenderableNode(RenderableNode::Image, node);
}

void RenderableNodeUpdater::endVisit(QSGImageNode *)
{
}

bool RenderableNodeUpdater::visit(QSGPainterNode *node)
{
    return updateRenderableNode(RenderableNode::Painter, node);
}

void RenderableNodeUpdater::endVisit(QSGPainterNode *)
{
}

bool RenderableNodeUpdater::visit(QSGRectangleNode *node)
{
    return updateRenderableNode(RenderableNode::Rectangle, node);
}

void RenderableNodeUpdater::endVisit(QSGRectangleNode *)
{
}

bool RenderableNodeUpdater::visit(QSGGlyphNode *node)
{
    return updateRenderableNode(RenderableNode::Glyph, node);
}

void RenderableNodeUpdater::endVisit(QSGGlyphNode *)
{
}

bool RenderableNodeUpdater::visit(QSGNinePatchNode *node)
{
    return updateRenderableNode(RenderableNode::NinePatch, node);
}

void RenderableNodeUpdater::endVisit(QSGNinePatchNode *)
{
}

bool RenderableNodeUpdater::visit(QSGRootNode *node)
{
    m_stateMap[node] = currentState(node);
    return true;
}

void RenderableNodeUpdater::endVisit(QSGRootNode *)
{
}

void RenderableNodeUpdater::updateNodes(QSGNode *node, bool isNodeRemoved)
{
    m_opacityState.clear();
    m_clipState.clear();
    m_transformState.clear();

    auto parentNode = node->parent();
    // If the node was deleted, it will have no parent
    // check if the state map has the previous parent
    if ((!parentNode || isNodeRemoved ) && m_stateMap.contains(node))
        parentNode = m_stateMap[node].parent;

    // If we find a parent, use its state for updating the new children
    if (parentNode && m_stateMap.contains(parentNode)) {
        auto state = m_stateMap[parentNode];
        m_opacityState.push(state.opacity);
        m_transformState.push(state.transform);
        m_clipState.push(state.clip);

    } else {
        // There is no parent, and no previous parent, so likely a root node
        m_opacityState.push(1.0f);
        m_transformState.push(QTransform());
        m_clipState.push(QRectF(0.0f, 0.0f, -1.0f, -1.0f));
    }

    // If the node is being removed, then cleanup the state data
    // Then just visit the children without visiting the now removed node
    if (isNodeRemoved) {
        m_stateMap.remove(node);
        return;
    }

    // Visit the current node itself first
    switch (node->type()) {
    case QSGNode::ClipNodeType: {
        QSGClipNode *c = static_cast<QSGClipNode*>(node);
        if (visit(c))
            visitChildren(c);
        endVisit(c);
        break;
    }
    case QSGNode::TransformNodeType: {
        QSGTransformNode *c = static_cast<QSGTransformNode*>(node);
        if (visit(c))
            visitChildren(c);
        endVisit(c);
        break;
    }
    case QSGNode::OpacityNodeType: {
        QSGOpacityNode *c = static_cast<QSGOpacityNode*>(node);
        if (visit(c))
            visitChildren(c);
        endVisit(c);
        break;
    }
    case QSGNode::GeometryNodeType: {
        if (node->flags() & QSGNode::IsVisitableNode) {
            QSGVisitableNode *v = static_cast<QSGVisitableNode*>(node);
            v->accept(this);
        } else {
            QSGGeometryNode *c = static_cast<QSGGeometryNode*>(node);
            if (visit(c))
                visitChildren(c);
            endVisit(c);
        }
        break;
    }
    case QSGNode::RootNodeType: {
        QSGRootNode *root = static_cast<QSGRootNode*>(node);
        if (visit(root))
            visitChildren(root);
        endVisit(root);
        break;
    }
    case QSGNode::BasicNodeType: {
            visitChildren(node);
        break;
    }
    default:
        Q_UNREACHABLE();
        break;
    }
}

RenderableNodeUpdater::NodeState RenderableNodeUpdater::currentState(QSGNode *node) const
{
    NodeState state;
    state.opacity = m_opacityState.top();
    state.clip = m_clipState.top();
    state.transform = m_transformState.top();
    state.parent = node->parent();
    return state;
}

} // namespace

QT_END_NAMESPACE
