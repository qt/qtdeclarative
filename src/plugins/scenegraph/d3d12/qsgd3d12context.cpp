/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#include "qsgd3d12context_p.h"
#include "qsgd3d12rendercontext_p.h"
#include "qsgd3d12internalrectanglenode_p.h"
#include "qsgd3d12internalimagenode_p.h"
#include "qsgd3d12glyphnode_p.h"
#include "qsgd3d12layer_p.h"
#include "qsgd3d12shadereffectnode_p.h"
#include "qsgd3d12painternode_p.h"
#include "qsgd3d12publicnodes_p.h"
#include "qsgd3d12spritenode_p.h"
#include <QtQuick/qquickwindow.h>

QT_BEGIN_NAMESPACE

QSGRenderContext *QSGD3D12Context::createRenderContext()
{
    return new QSGD3D12RenderContext(this);
}

QSGInternalRectangleNode *QSGD3D12Context::createInternalRectangleNode()
{
    return new QSGD3D12InternalRectangleNode;
}

QSGInternalImageNode *QSGD3D12Context::createInternalImageNode(QSGRenderContext *renderContext)
{
    Q_UNUSED(renderContext);
    return new QSGD3D12InternalImageNode;
}

QSGPainterNode *QSGD3D12Context::createPainterNode(QQuickPaintedItem *item)
{
    return new QSGD3D12PainterNode(item);
}

QSGGlyphNode *QSGD3D12Context::createGlyphNode(QSGRenderContext *renderContext, bool preferNativeGlyphNode)
{
    Q_UNUSED(preferNativeGlyphNode);
    // ### distance field text rendering is not supported atm

    QSGD3D12RenderContext *rc = static_cast<QSGD3D12RenderContext *>(renderContext);
    return new QSGD3D12GlyphNode(rc);
}

QSGLayer *QSGD3D12Context::createLayer(QSGRenderContext *renderContext)
{
    QSGD3D12RenderContext *rc = static_cast<QSGD3D12RenderContext *>(renderContext);
    return new QSGD3D12Layer(rc);
}

QSGGuiThreadShaderEffectManager *QSGD3D12Context::createGuiThreadShaderEffectManager()
{
    return new QSGD3D12GuiThreadShaderEffectManager;
}

QSGShaderEffectNode *QSGD3D12Context::createShaderEffectNode(QSGRenderContext *renderContext,
                                                             QSGGuiThreadShaderEffectManager *mgr)
{
    QSGD3D12RenderContext *rc = static_cast<QSGD3D12RenderContext *>(renderContext);
    QSGD3D12GuiThreadShaderEffectManager *dmgr = static_cast<QSGD3D12GuiThreadShaderEffectManager *>(mgr);
    return new QSGD3D12ShaderEffectNode(rc, dmgr);
}

QSize QSGD3D12Context::minimumFBOSize() const
{
    return QSize(16, 16);
}

QSurfaceFormat QSGD3D12Context::defaultSurfaceFormat() const
{
    QSurfaceFormat format = QSurfaceFormat::defaultFormat();

    if (QQuickWindow::hasDefaultAlphaBuffer())
        format.setAlphaBufferSize(8);

    return format;
}

QSGRendererInterface *QSGD3D12Context::rendererInterface(QSGRenderContext *renderContext)
{
    return static_cast<QSGD3D12RenderContext *>(renderContext);
}

QSGRectangleNode *QSGD3D12Context::createRectangleNode()
{
    return new QSGD3D12RectangleNode;
}

QSGImageNode *QSGD3D12Context::createImageNode()
{
    return new QSGD3D12ImageNode;
}

QSGNinePatchNode *QSGD3D12Context::createNinePatchNode()
{
    return new QSGD3D12NinePatchNode;
}

QSGSpriteNode *QSGD3D12Context::createSpriteNode()
{
    return new QSGD3D12SpriteNode;
}

QT_END_NAMESPACE
