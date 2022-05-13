// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsgsoftwarerenderlistbuilder_p.h"

#include "qsgsoftwarerenderablenode_p.h"
#include "qsgabstractsoftwarerenderer_p.h"
#include "qsgsoftwareinternalimagenode_p.h"
#include "qsgsoftwareinternalrectanglenode_p.h"
#include "qsgsoftwareglyphnode_p.h"
#include "qsgsoftwarepublicnodes_p.h"
#include "qsgsoftwarepainternode_p.h"
#include "qsgsoftwarepixmaptexture_p.h"

#include <QtQuick/qsgsimplerectnode.h>
#include <QtQuick/qsgsimpletexturenode.h>
#include <QtQuick/qsgrendernode.h>

QT_BEGIN_NAMESPACE

QSGSoftwareRenderListBuilder::QSGSoftwareRenderListBuilder(QSGAbstractSoftwareRenderer *renderer)
    : m_renderer(renderer)
{

}

bool QSGSoftwareRenderListBuilder::visit(QSGTransformNode *)
{
    return true;
}

void QSGSoftwareRenderListBuilder::endVisit(QSGTransformNode *)
{
}

bool QSGSoftwareRenderListBuilder::visit(QSGClipNode *)
{
    return true;
}

void QSGSoftwareRenderListBuilder::endVisit(QSGClipNode *)
{
}

bool QSGSoftwareRenderListBuilder::visit(QSGGeometryNode *node)
{
    return addRenderableNode(node);
}

void QSGSoftwareRenderListBuilder::endVisit(QSGGeometryNode *)
{
}

bool QSGSoftwareRenderListBuilder::visit(QSGOpacityNode *)
{
    return true;
}

void QSGSoftwareRenderListBuilder::endVisit(QSGOpacityNode *)
{
}

bool QSGSoftwareRenderListBuilder::visit(QSGInternalImageNode *node)
{
    return addRenderableNode(node);
}

void QSGSoftwareRenderListBuilder::endVisit(QSGInternalImageNode *)
{
}

bool QSGSoftwareRenderListBuilder::visit(QSGPainterNode *node)
{
    return addRenderableNode(node);
}

void QSGSoftwareRenderListBuilder::endVisit(QSGPainterNode *)
{
}

bool QSGSoftwareRenderListBuilder::visit(QSGInternalRectangleNode *node)
{
    return addRenderableNode(node);
}

void QSGSoftwareRenderListBuilder::endVisit(QSGInternalRectangleNode *)
{
}

bool QSGSoftwareRenderListBuilder::visit(QSGGlyphNode *node)
{
    return addRenderableNode(node);
}

void QSGSoftwareRenderListBuilder::endVisit(QSGGlyphNode *)
{
}

bool QSGSoftwareRenderListBuilder::visit(QSGRootNode *)
{
    return true;
}

void QSGSoftwareRenderListBuilder::endVisit(QSGRootNode *)
{
}

#if QT_CONFIG(quick_sprite)
bool QSGSoftwareRenderListBuilder::visit(QSGSpriteNode *node)
{
    return addRenderableNode(node);
}

void QSGSoftwareRenderListBuilder::endVisit(QSGSpriteNode *)
{

}
#endif

bool QSGSoftwareRenderListBuilder::visit(QSGRenderNode *node)
{
    return addRenderableNode(node);
}

void QSGSoftwareRenderListBuilder::endVisit(QSGRenderNode *)
{
}

bool QSGSoftwareRenderListBuilder::addRenderableNode(QSGNode *node)
{
    auto renderableNode = m_renderer->renderableNode(node);
    if (renderableNode == nullptr) {
        // Not a node we can render
        return false;
    }
    m_renderer->appendRenderableNode(renderableNode);
    return true;
}

QT_END_NAMESPACE
