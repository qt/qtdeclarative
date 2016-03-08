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

#include "renderlistbuilder.h"

#include "renderablenode.h"
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


RenderListBuilder::RenderListBuilder(AbstractSoftwareRenderer *renderer)
    : m_renderer(renderer)
{

}

bool RenderListBuilder::visit(QSGTransformNode *)
{
    return true;
}

void RenderListBuilder::endVisit(QSGTransformNode *)
{
}

bool RenderListBuilder::visit(QSGClipNode *)
{
    return true;
}

void RenderListBuilder::endVisit(QSGClipNode *)
{
}

bool RenderListBuilder::visit(QSGGeometryNode *node)
{
    return addRenderableNode(node);
}

void RenderListBuilder::endVisit(QSGGeometryNode *)
{
}

bool RenderListBuilder::visit(QSGOpacityNode *)
{
    return true;
}

void RenderListBuilder::endVisit(QSGOpacityNode *)
{
}

bool RenderListBuilder::visit(QSGImageNode *node)
{
    return addRenderableNode(node);
}

void RenderListBuilder::endVisit(QSGImageNode *)
{
}

bool RenderListBuilder::visit(QSGPainterNode *node)
{
    return addRenderableNode(node);
}

void RenderListBuilder::endVisit(QSGPainterNode *)
{
}

bool RenderListBuilder::visit(QSGRectangleNode *node)
{
    return addRenderableNode(node);
}

void RenderListBuilder::endVisit(QSGRectangleNode *)
{
}

bool RenderListBuilder::visit(QSGGlyphNode *node)
{
    return addRenderableNode(node);
}

void RenderListBuilder::endVisit(QSGGlyphNode *)
{
}

bool RenderListBuilder::visit(QSGNinePatchNode *node)
{
    return addRenderableNode(node);
}

void RenderListBuilder::endVisit(QSGNinePatchNode *)
{
}

bool RenderListBuilder::visit(QSGRootNode *)
{
    return true;
}

void RenderListBuilder::endVisit(QSGRootNode *)
{
}

bool RenderListBuilder::addRenderableNode(QSGNode *node)
{
    auto renderableNode = m_renderer->renderableNode(node);
    if (renderableNode == nullptr) {
        // Not a node we can render
        return false;
    }
    m_renderer->appendRenderableNode(renderableNode);
    return true;
}

} // namespace

QT_END_NAMESPACE
