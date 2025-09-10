// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsgopenvgrenderer_p.h"
#include "qsgopenvgcontext_p.h"
#include "qsgopenvgnodevisitor.h"
#include "qopenvgcontext_p.h"
#include "qsgopenvghelpers.h"

#include <QtGui/QWindow>

#include <VG/openvg.h>

QT_BEGIN_NAMESPACE

QSGOpenVGRenderer::QSGOpenVGRenderer(QSGRenderContext *context)
    : QSGRenderer(context)
{

}

QSGOpenVGRenderer::~QSGOpenVGRenderer()
{

}

void QSGOpenVGRenderer::render()
{
    //Clear the window geometry with the clear color
    vgSetfv(VG_CLEAR_COLOR, 4, QSGOpenVGHelpers::qColorToVGColor(clearColor()).constData());
    vgClear(0, 0, VG_MAXINT, VG_MAXINT);

    // Visit each node to render scene
    QSGOpenVGNodeVisitor rendererVisitor;
    rendererVisitor.visitChildren(rootNode());
}

void QSGOpenVGRenderer::nodeChanged(QSGNode *node, QSGNode::DirtyState state)
{
    QSGRenderer::nodeChanged(node, state);
}

QT_END_NAMESPACE
