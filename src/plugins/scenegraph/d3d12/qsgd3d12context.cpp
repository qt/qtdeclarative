/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qsgd3d12context_p.h"
#include "qsgd3d12rendercontext_p.h"
#include "qsgd3d12rectanglenode_p.h"
#include "qsgd3d12imagenode_p.h"
#include "qsgd3d12glyphnode_p.h"
#include "qsgd3d12layer_p.h"
#include "qsgd3d12shadereffectnode_p.h"

QT_BEGIN_NAMESPACE

QSGRenderContext *QSGD3D12Context::createRenderContext()
{
    return new QSGD3D12RenderContext(this);
}

QSGRectangleNode *QSGD3D12Context::createRectangleNode()
{
    return new QSGD3D12RectangleNode;
}

QSGImageNode *QSGD3D12Context::createImageNode()
{
    return new QSGD3D12ImageNode;
}

QSGPainterNode *QSGD3D12Context::createPainterNode(QQuickPaintedItem *item)
{
    Q_UNUSED(item);
    Q_UNREACHABLE();
    return nullptr;
}

QSGGlyphNode *QSGD3D12Context::createGlyphNode(QSGRenderContext *rc, bool preferNativeGlyphNode)
{
    Q_UNUSED(preferNativeGlyphNode);
    // ### distance field text rendering is not supported atm

    return new QSGD3D12GlyphNode(static_cast<QSGD3D12RenderContext *>(rc));
}

QSGNinePatchNode *QSGD3D12Context::createNinePatchNode()
{
    Q_UNREACHABLE();
    return nullptr;
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
    return QSurfaceFormat::defaultFormat();
}

QSGRendererInterface *QSGD3D12Context::rendererInterface(QSGRenderContext *renderContext)
{
    QSGD3D12RenderContext *rc = static_cast<QSGD3D12RenderContext *>(renderContext);
    if (!rc->engine()) {
        qWarning("No D3D12 engine available yet (no render thread due to window not exposed?)");
        return nullptr;
    }
    return rc->engine();
}

QT_END_NAMESPACE
