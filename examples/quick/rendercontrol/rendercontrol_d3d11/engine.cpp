// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "engine.h"
#include <QLibrary>
#include <comdef.h>

#define ENABLE_DEBUG_LAYER

Engine::~Engine()
{
    RELEASE(m_context);
    RELEASE(m_device);
    RELEASE(m_dxgiFactory);
}

QString comErrorMessage(HRESULT hr)
{
    const _com_error comError(hr);
    QString result = QLatin1String("Error 0x") + QString::number(ulong(hr), 16);
    if (const wchar_t *msg = comError.ErrorMessage())
        result += QLatin1String(": ") + QString::fromWCharArray(msg);
    return result;
}

bool Engine::create()
{
    using PtrCreateDXGIFactory2 = HRESULT (WINAPI *)(UINT, REFIID, void **);
    QLibrary dxgilib(QStringLiteral("dxgi"));
    if (auto createDXGIFactory2 = reinterpret_cast<PtrCreateDXGIFactory2>(dxgilib.resolve("CreateDXGIFactory2"))) {
        const HRESULT hr = createDXGIFactory2(0, IID_IDXGIFactory2, reinterpret_cast<void **>(&m_dxgiFactory));
        if (FAILED(hr)) {
            qWarning("CreateDXGIFactory2() failed to create DXGI factory: %s", qPrintable(comErrorMessage(hr)));
            return false;
        }
    } else {
        qWarning("Unable to resolve CreateDXGIFactory2()");
        return false;
    }

    uint flags = 0;
#ifdef ENABLE_DEBUG_LAYER
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
    // use the default hardware adapter
    HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags,
                                   nullptr, 0, D3D11_SDK_VERSION,
                                   &m_device, &m_featureLevel, &m_context);
    if (FAILED(hr)) {
        qWarning("Failed to create D3D11 device and context: %s", qPrintable(comErrorMessage(hr)));
        return false;
    }

    return true;
}

QSize Engine::swapchainSizeForWindow(QWindow *window) const
{
    const QSize size = window->size() * window->devicePixelRatio();
    return QSize(qMax(8, size.width()), qMax(8, size.height()));
}

Swapchain Engine::createSwapchain(QWindow *window)
{
    Swapchain sc = {};
    const HWND hwnd = reinterpret_cast<HWND>(window->winId());
    const QSize pixelSize = swapchainSizeForWindow(window);

    // only care about flip discard swapchains here; the old stuff (discard) is
    // not supported in this example

    DXGI_SWAP_CHAIN_DESC1 desc = {};
    desc.Width = UINT(pixelSize.width());
    desc.Height = UINT(pixelSize.height());
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount = 2;
    desc.Scaling = DXGI_SCALING_STRETCH;
    desc.SwapEffect = DXGI_SWAP_EFFECT(4); // DXGI_SWAP_EFFECT_FLIP_DISCARD

    IDXGISwapChain1 *swapchain = nullptr;
    HRESULT hr = static_cast<IDXGIFactory2 *>(m_dxgiFactory)->CreateSwapChainForHwnd(m_device, hwnd, &desc,
                                                                                     nullptr, nullptr, &swapchain);
    if (FAILED(hr)) {
        qWarning("Failed to create D3D11 swapchain: %s", qPrintable(comErrorMessage(hr)));
        return sc;
    }

    sc.swapchain = swapchain;
    sc.pixelSize = pixelSize;
    createSwapchainBuffers(&sc);
    return sc;
}

void Engine::createSwapchainBuffers(Swapchain *sc)
{
    ID3D11Texture2D *tex = nullptr;
    HRESULT hr = sc->swapchain->GetBuffer(0, IID_ID3D11Texture2D, reinterpret_cast<void **>(&tex));
    if (FAILED(hr)) {
        qWarning("Failed to query swapchain backbuffer: %s", qPrintable(comErrorMessage(hr)));
        return;
    }

    ID3D11RenderTargetView *rtv = nullptr;
    D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    hr = m_device->CreateRenderTargetView(tex, &rtvDesc, &rtv);
    if (FAILED(hr)) {
        qWarning("Failed to create rtv for swapchain backbuffer: %s", qPrintable(comErrorMessage(hr)));
        tex->Release();
        return;
    }

    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = UINT(sc->pixelSize.width());
    texDesc.Height = UINT(sc->pixelSize.height());
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.SampleDesc.Count = 1;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    texDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    ID3D11Texture2D *ds = nullptr;
    hr = m_device->CreateTexture2D(&texDesc, nullptr, &ds);
    if (FAILED(hr)) {
        qWarning("Failed to create depth-stencil buffer: %s", qPrintable(comErrorMessage(hr)));
        tex->Release();
        rtv->Release();
        return;
    }

    ID3D11DepthStencilView *dsv = nullptr;
    D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    hr = m_device->CreateDepthStencilView(ds, &dsvDesc, &dsv);
    if (FAILED(hr)) {
        qWarning("Failed to create dsv: %s", qPrintable(comErrorMessage(hr)));
        tex->Release();
        rtv->Release();
        ds->Release();
        return;
    }

    sc->tex = tex;
    sc->rtv = rtv;
    sc->ds = ds;
    sc->dsv = dsv;
}

void Engine::resizeSwapchain(Swapchain *sc, QWindow *window)
{
    const QSize pixelSize = swapchainSizeForWindow(window);

    RELEASE(sc->dsv);
    RELEASE(sc->ds);
    RELEASE(sc->rtv);
    RELEASE(sc->tex);

    HRESULT hr = sc->swapchain->ResizeBuffers(2, UINT(pixelSize.width()), UINT(pixelSize.height()),
                                              DXGI_FORMAT_R8G8B8A8_UNORM, 0);
    if (FAILED(hr)) {
        qWarning("Failed to resize D3D11 swapchain: %s", qPrintable(comErrorMessage(hr)));
        return;
    }

    sc->pixelSize = pixelSize;
    createSwapchainBuffers(sc);
}

void Swapchain::destroy()
{
    RELEASE(dsv);
    RELEASE(ds);
    RELEASE(rtv);
    RELEASE(tex);
    RELEASE(swapchain);
    pixelSize = QSize();
}
