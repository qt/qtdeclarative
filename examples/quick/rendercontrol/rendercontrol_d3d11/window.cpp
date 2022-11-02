// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "window.h"
#include <QCoreApplication>
#include <QMouseEvent>
#include <QQuickRenderControl>
#include <QQuickWindow>
#include <QQuickItem>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQuickRenderTarget>
#include <QQuickGraphicsDevice>

#include "quad.vert.inc"
#include "quad.frag.inc"

// In this example the Qt Quick scene will always render at 720p regardless of
// the window size.
const int QML_WIDTH = 1280;
const int QML_HEIGHT = 720;

// Set to 4 or 8 to enable MSAA. This will lead to creating a multisample
// texture, passing in the sample count to Qt Quick (so it sets the graphics
// pipelines up as appropriate), and then doing a resolve to a non-multisample
// texture every time Quick has rendered its content.
const int SAMPLE_COUNT = 1;

// by subclassing QQuickRenderControl we gain the ability to report a QWindow
// to which certain operations, such as the querying of devicePixelRatio()
// should be redirected
class RenderControl : public QQuickRenderControl
{
public:
    RenderControl(QWindow *w) : m_window(w) { }
    QWindow *renderWindow(QPoint *offset) override;

private:
    QWindow *m_window;
};

QWindow *RenderControl::renderWindow(QPoint *offset)
{
    if (offset)
        *offset = QPoint(0, 0);
    return m_window;
}

Window::Window(Engine *engine)
    : m_engine(engine)
{
    setSurfaceType(QSurface::OpenGLSurface);

    m_renderControl = new RenderControl(this);

    // Whenever something changed in the Quick scene, or rendering was
    // requested via others means (e.g. QQuickWindow::update()), it should
    // trigger rendering into the texture when preparing the next frame.
    connect(m_renderControl, &QQuickRenderControl::renderRequested, this, [this] { m_quickDirty = true; });
    connect(m_renderControl, &QQuickRenderControl::sceneChanged, this, [this] { m_quickDirty = true; });

    // Note that on its own this is not sufficient to get MSAA, the render
    // target (the texture in this case) must be set up accordingly as well,
    // and the sample count also needs to be passed to
    // QQuickRenderTarget::fromNativeTexture().
    m_renderControl->setSamples(SAMPLE_COUNT);

    // Create a QQuickWindow that is associated with out render control. Note that this
    // window never gets created or shown, meaning that it will never get an underlying
    // native (platform) window.
    m_quickWindow = new QQuickWindow(m_renderControl);

    m_qmlEngine = new QQmlEngine;
    m_qmlComponent = new QQmlComponent(
                m_qmlEngine, QUrl(QLatin1String("qrc:/qt/qml/rendercontrol/demo.qml")));
    if (m_qmlComponent->isError()) {
        for (const QQmlError &error : m_qmlComponent->errors())
            qWarning() << error.url() << error.line() << error;
    }

    QObject *rootObject = m_qmlComponent->create();
    if (m_qmlComponent->isError()) {
        for (const QQmlError &error : m_qmlComponent->errors())
            qWarning() << error.url() << error.line() << error;
    }

    m_rootItem = qobject_cast<QQuickItem *>(rootObject);
    m_rootItem->setSize(QSize(QML_WIDTH, QML_HEIGHT));
    m_quickWindow->contentItem()->setSize(m_rootItem->size());
    m_quickWindow->setGeometry(0, 0, m_rootItem->width(), m_rootItem->height());

    m_rootItem->setParentItem(m_quickWindow->contentItem());
}

Window::~Window()
{
    delete m_qmlComponent;
    delete m_qmlEngine;
    delete m_quickWindow;
    delete m_renderControl;

    releaseResources();

    // Often a no-op (if we already got SurfaceAboutToBeDestroyed), but there
    // are cases when that's not sent.
    m_swapchain.destroy();
}

// Expose (and UpdateRequest) are all the events we need: resize and screen dpr
// changes are handled implicitly since every render() checks for size changes
// so no separate event handlers are needed for that.

void Window::exposeEvent(QExposeEvent *)
{
    if (isExposed()) {
        // initialize if this is the first expose
        if (!m_swapchain.swapchain)
            m_swapchain = m_engine->createSwapchain(this);
        // must always render and present a frame on expose
        if (!size().isEmpty())
            render();
    }
}

// Input is severly limited in this example: there is no mapping or projection
// of any kind, so it all behaves as if the Qt Quick content was covering the
// entire window. The example only cares about button down/up, not the position
// so this is acceptable here.

void Window::mousePressEvent(QMouseEvent *e)
{
    QMouseEvent mappedEvent(e->type(), e->position(), e->globalPosition(), e->button(), e->buttons(), e->modifiers());
    QCoreApplication::sendEvent(m_quickWindow, &mappedEvent);
}

void Window::mouseReleaseEvent(QMouseEvent *e)
{
    QMouseEvent mappedEvent(e->type(), e->position(), e->globalPosition(), e->button(), e->buttons(), e->modifiers());
    QCoreApplication::sendEvent(m_quickWindow, &mappedEvent);
}

void Window::keyPressEvent(QKeyEvent *e)
{
    QCoreApplication::sendEvent(m_quickWindow, e);
}

void Window::keyReleaseEvent(QKeyEvent *e)
{
    QCoreApplication::sendEvent(m_quickWindow, e);
}

bool Window::event(QEvent *e)
{
    switch (e->type()) {
    case QEvent::UpdateRequest:
        render();
        break;

    case QEvent::PlatformSurface:
        // trying to be nice, not strictly required for D3D
        if (static_cast<QPlatformSurfaceEvent *>(e)->surfaceEventType() == QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed)
            m_swapchain.destroy();
        break;

    default:
        break;
    }

    return QWindow::event(e);
}

bool Window::initResources()
{
    ID3D11Device *dev = m_engine->device();

    // vertex and pixel shader to render a textured quad

    HRESULT hr = dev->CreateVertexShader(g_quad_vs_main, sizeof(g_quad_vs_main), nullptr, &m_res.vertexShader);
    if (FAILED(hr)) {
        qWarning("Failed to create vertex shader: %s", qPrintable(comErrorMessage(hr)));
        return false;
    }

    hr = dev->CreatePixelShader(g_quad_ps_main, sizeof(g_quad_ps_main), nullptr, &m_res.pixelShader);
    if (FAILED(hr)) {
        qWarning("Failed to create pixel shader: %s", qPrintable(comErrorMessage(hr)));
        return false;
    }

    // texture into which Qt Quick will render and which we will then sample

    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = QML_WIDTH;
    texDesc.Height = QML_HEIGHT;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    if (SAMPLE_COUNT > 1) {
        texDesc.SampleDesc.Count = SAMPLE_COUNT;
        texDesc.SampleDesc.Quality = UINT(D3D11_STANDARD_MULTISAMPLE_PATTERN);
    } else {
        texDesc.SampleDesc.Count = 1;
    }
    // we have to use BIND_SHADER_RESOURCE even if the texture is MSAA because
    // an SRV may still get created internally by Qt Quick
    texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
    texDesc.Usage = D3D11_USAGE_DEFAULT;

    hr = dev->CreateTexture2D(&texDesc, nullptr, &m_res.texture);
    if (FAILED(hr)) {
        qWarning("Failed to create texture: %s", qPrintable(comErrorMessage(hr)));
        return false;
    }

    if (SAMPLE_COUNT > 1) {
        texDesc.SampleDesc.Count = 1;
        texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        hr = dev->CreateTexture2D(&texDesc, nullptr, &m_res.resolveTexture);
        if (FAILED(hr)) {
            qWarning("Failed to create resolve texture: %s", qPrintable(comErrorMessage(hr)));
            return false;
        }
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;

    hr = dev->CreateShaderResourceView(SAMPLE_COUNT > 1 ? m_res.resolveTexture : m_res.texture, &srvDesc, &m_res.textureSrv);
    if (FAILED(hr)) {
        qWarning("Failed to create srv: %s", qPrintable(comErrorMessage(hr)));
        return false;
    }

    D3D11_SAMPLER_DESC sampDesc = {};
    sampDesc.Filter = D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.MaxAnisotropy = 1.0f;

    hr = dev->CreateSamplerState(&sampDesc, &m_res.sampler);
    if (FAILED(hr)) {
        qWarning("Failed to create sampler state: %s", qPrintable(comErrorMessage(hr)));
        return false;
    }

    m_res.valid = true;
    return true;
}

void Window::releaseResources()
{
    RELEASE(m_res.vertexShader);
    RELEASE(m_res.pixelShader);
    RELEASE(m_res.texture);
    RELEASE(m_res.resolveTexture);
    RELEASE(m_res.textureSrv);
    RELEASE(m_res.sampler);

    m_res.valid = false;
}

void Window::render()
{
    if (!isExposed() || !m_swapchain.swapchain || !m_swapchain.tex || !m_swapchain.rtv)
        return;

    // if the window got resized, the swapchain buffers must be resized as well
    if (m_swapchain.pixelSize != m_engine->swapchainSizeForWindow(this))
        m_engine->resizeSwapchain(&m_swapchain, this);

    if (!m_res.valid) {
        if (!initResources())
            return;
    }

    // get some content into m_res.texture from Qt Quick
    updateQuick();

    // now onto our own drawing, targeting the window
    ID3D11DeviceContext *ctx = m_engine->context();
    const QSize viewSize = m_swapchain.pixelSize;

    const float clearColor[] = { 0.4f, 0.7f, 0.0f, 1.0f };
    ctx->ClearRenderTargetView(m_swapchain.rtv, clearColor);
    ctx->ClearDepthStencilView(m_swapchain.dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    ctx->OMSetRenderTargets(1, &m_swapchain.rtv, m_swapchain.dsv);

    const D3D11_VIEWPORT viewport = { 0.0f, 0.0f, float(viewSize.width()), float(viewSize.height()),
                                      0.f, 1.0f };
    ctx->RSSetViewports(1, &viewport);

    // draw a textured quad

    ctx->VSSetShader(m_res.vertexShader, nullptr, 0);
    ctx->PSSetShader(m_res.pixelShader, nullptr, 0);

    ctx->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->IASetInputLayout(nullptr);
    ctx->OMSetDepthStencilState(nullptr, 0);
    ctx->OMSetBlendState(nullptr, nullptr, 0xffffffff);
    ctx->RSSetState(nullptr);

    ctx->PSSetShaderResources(0, 1, &m_res.textureSrv);
    ctx->PSSetSamplers(0, 1, &m_res.sampler);

    ctx->Draw(6, 0);

    m_swapchain.swapchain->Present(1, 0);

    requestUpdate(); // will lead to eventually getting a QEvent::UpdateRequest
}

void Window::updateQuick()
{
    if (!m_quickDirty)
        return;

    m_quickDirty = false;

    if (!m_quickInitialized) {
        // In addition to setGraphicsApi(), we need a call to
        // setGraphicsDevice to tell Qt Quick what ID3D11Device(Context) to use
        // (i.e. we want it to use ours, not to create new ones).
        m_quickWindow->setGraphicsDevice(QQuickGraphicsDevice::fromDeviceAndContext(m_engine->device(), m_engine->context()));

        // Now we can kick off the scenegraph.
        if (!m_renderControl->initialize())
            qWarning("Failed to initialize redirected Qt Quick rendering");

        // Redirect Qt Quick's output.
        m_quickWindow->setRenderTarget(QQuickRenderTarget::fromD3D11Texture(m_res.texture,
                                                                            QSize(QML_WIDTH, QML_HEIGHT),
                                                                            SAMPLE_COUNT));

        // Ensure key events are received by the root Rectangle.
        m_rootItem->forceActiveFocus();

        m_quickInitialized = true;
    }

    m_renderControl->polishItems();

    m_renderControl->beginFrame();
    m_renderControl->sync();
    m_renderControl->render();
    m_renderControl->endFrame(); // Qt Quick's rendering commands are submitted to the device context here

    if (SAMPLE_COUNT > 1)
        m_engine->context()->ResolveSubresource(m_res.resolveTexture, 0, m_res.texture, 0, DXGI_FORMAT_R8G8B8A8_UNORM);
}
