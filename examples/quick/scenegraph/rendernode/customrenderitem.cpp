/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "customrenderitem.h"
#include <QQuickWindow>
#include <QSGRendererInterface>

#include "openglrenderer.h"
#include "d3d12renderer.h"

CustomRenderNode::~CustomRenderNode()
{
    releaseResources();
}

void CustomRenderNode::render(const RenderState *state)
{
    QSGRendererInterface *ri = m_item->window()->rendererInterface();
    if (!ri)
        return;

    m_api = ri->graphicsAPI();

    if (!m_renderer) {
        switch (m_api) {
        case QSGRendererInterface::OpenGL:
#ifndef QT_NO_OPENGL
            m_renderer = new OpenGLRenderer(m_item, this);
#endif
            break;
        case QSGRendererInterface::Direct3D12:
#ifdef HAS_D3D12
            m_renderer = new D3D12Renderer(m_item, this);
#endif
            break;
        default:
            break;
        }
        Q_ASSERT(m_renderer);
        m_renderer->init();
    }

    m_renderer->render(state);
}

// No need to reimplement changedStates() since our rendering is so simple,
// without involving any state changes.

void CustomRenderNode::releaseResources()
{
    if (!m_renderer)
        return;

    delete m_renderer;
    m_renderer = nullptr;
}

CustomRenderItem::CustomRenderItem(QQuickItem *parent)
    : QQuickItem(parent)
{
    // Our item shows something so set the flag.
    setFlag(ItemHasContents);

    // We want the graphics API type to be exposed to QML. The value is easy to
    // get during rendering on the render thread in CustomRenderNode::render(),
    // but is more tricky here since the item gets a window associated later,
    // which in turn will get the underlying scenegraph started at some later
    // point. So defer.
    connect(this, &QQuickItem::windowChanged, this, &CustomRenderItem::onWindowChanged);
}

QSGNode *CustomRenderItem::updatePaintNode(QSGNode *node, UpdatePaintNodeData *)
{
    CustomRenderNode *n = static_cast<CustomRenderNode *>(node);
    if (!node)
        n = new CustomRenderNode(this);

    return n;
}

void CustomRenderItem::onWindowChanged(QQuickWindow *w)
{
    if (w) {
        if (w->isSceneGraphInitialized())
            updateGraphicsAPI();
        else
            connect(w, &QQuickWindow::sceneGraphInitialized, this, &CustomRenderItem::updateGraphicsAPI);
    } else {
        updateGraphicsAPI();
    }
}

void CustomRenderItem::updateGraphicsAPI()
{
    QString newAPI;
    if (!window()) {
        newAPI = QLatin1String("[no window]");
    } else {
        QSGRendererInterface *ri = window()->rendererInterface();
        if (!ri) {
            newAPI = QLatin1String("[no renderer interface]");
        } else {
            switch (ri->graphicsAPI()) {
            case QSGRendererInterface::OpenGL:
                newAPI = QLatin1String("OpenGL");
                break;
            case QSGRendererInterface::Direct3D12:
                newAPI = QLatin1String("D3D12");
                break;
            default:
                newAPI = QString(QLatin1String("[unsupported graphics API %1]")).arg(ri->graphicsAPI());
                break;
            }
        }
    }

    if (newAPI != m_api) {
        m_api = newAPI;
        emit graphicsAPIChanged();
    }
}
