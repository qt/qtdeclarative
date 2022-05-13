// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef ENGINE_H
#define ENGINE_H

#include <QWindow>
#include <QtCore/qt_windows.h> // suppress windows.h include coming from d3d11_1.h

#include <d3d11_1.h>
#include <dxgi1_3.h>

#define RELEASE(obj) { if (obj) { obj->Release(); obj = nullptr; } }

QString comErrorMessage(HRESULT hr);

struct Swapchain
{
    IDXGISwapChain1 *swapchain;
    ID3D11Texture2D *tex;
    ID3D11RenderTargetView *rtv;
    ID3D11Texture2D *ds;
    ID3D11DepthStencilView *dsv;
    QSize pixelSize;

    void destroy();
};

class Engine
{
public:
    ~Engine();
    bool create();
    QSize swapchainSizeForWindow(QWindow *window) const;
    Swapchain createSwapchain(QWindow *window);
    void resizeSwapchain(Swapchain *sc, QWindow *window);
    ID3D11Device *device() { return m_device; }
    ID3D11DeviceContext *context() { return m_context; }

private:
    void createSwapchainBuffers(Swapchain *sc);

    IDXGIFactory1 *m_dxgiFactory = nullptr;
    ID3D11Device *m_device = nullptr;
    ID3D11DeviceContext *m_context = nullptr;
    D3D_FEATURE_LEVEL m_featureLevel;
};

#endif
