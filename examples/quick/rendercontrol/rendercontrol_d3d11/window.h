// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef WINDOW_H
#define WINDOW_H

#include <QWindow>
#include "engine.h"

QT_BEGIN_NAMESPACE
class QQuickRenderControl;
class QQuickWindow;
class QQuickItem;
class QQmlEngine;
class QQmlComponent;
QT_END_NAMESPACE

class Window : public QWindow
{
public:
    Window(Engine *engine);
    ~Window();

protected:
    void exposeEvent(QExposeEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void keyPressEvent(QKeyEvent *e) override;
    void keyReleaseEvent(QKeyEvent *e) override;
    bool event(QEvent *e) override;

private:
    void render();
    bool initResources();
    void releaseResources();
    void updateQuick();

    Engine *m_engine;
    QQuickRenderControl *m_renderControl;
    QQuickWindow *m_quickWindow;
    QQmlEngine *m_qmlEngine;
    QQmlComponent *m_qmlComponent;
    QQuickItem *m_rootItem;
    bool m_quickInitialized = false;
    bool m_quickDirty = true;

    Swapchain m_swapchain = {};
    struct {
        bool valid = false;
        ID3D11VertexShader *vertexShader = nullptr;
        ID3D11PixelShader *pixelShader = nullptr;
        ID3D11Texture2D *texture = nullptr;
        ID3D11Texture2D *resolveTexture = nullptr;
        ID3D11ShaderResourceView *textureSrv = nullptr;
        ID3D11SamplerState *sampler = nullptr;
    } m_res;
};

#endif
