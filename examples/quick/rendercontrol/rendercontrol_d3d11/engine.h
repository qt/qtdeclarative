/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
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

#ifndef ENGINE_H
#define ENGINE_H

#include <QWindow>

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
    ID3D11DeviceContext1 *context() { return m_context; }

private:
    void createSwapchainBuffers(Swapchain *sc);

    IDXGIFactory1 *m_dxgiFactory = nullptr;
    ID3D11Device *m_device = nullptr;
    ID3D11DeviceContext1 *m_context = nullptr;
    D3D_FEATURE_LEVEL m_featureLevel;
};

#endif
