/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the demonstration applications of the Qt Toolkit.
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

#include "d3d11squircle.h"
#include <QtCore/QFile>
#include <QtCore/QRunnable>
#include <QtQuick/QQuickWindow>

#include <d3d11.h>
#include <d3dcompiler.h>

class SquircleRenderer : public QObject
{
    Q_OBJECT
public:
    SquircleRenderer();
    ~SquircleRenderer();

    void setT(qreal t) { m_t = t; }
    void setViewportSize(const QSize &size) { m_viewportSize = size; }
    void setWindow(QQuickWindow *window) { m_window = window; }

public slots:
    void frameStart();
    void mainPassRecordingStart();

private:
    enum Stage {
        VertexStage,
        FragmentStage
    };
    void prepareShader(Stage stage);
    QByteArray compileShader(Stage stage,
                             const QByteArray &source,
                             const QByteArray &entryPoint);
    void init();

    QSize m_viewportSize;
    qreal m_t;
    QQuickWindow *m_window;

    ID3D11Device *m_device = nullptr;
    ID3D11DeviceContext *m_context = nullptr;
    QByteArray m_vert;
    QByteArray m_vertEntryPoint;
    QByteArray m_frag;
    QByteArray m_fragEntryPoint;

    bool m_initialized = false;
    ID3D11Buffer *m_vbuf = nullptr;
    ID3D11Buffer *m_cbuf = nullptr;
    ID3D11VertexShader *m_vs = nullptr;
    ID3D11PixelShader *m_ps = nullptr;
    ID3D11InputLayout *m_inputLayout = nullptr;
    ID3D11RasterizerState *m_rastState = nullptr;
    ID3D11DepthStencilState *m_dsState = nullptr;
    ID3D11BlendState *m_blendState = nullptr;
};

D3D11Squircle::D3D11Squircle()
    : m_t(0)
    , m_renderer(nullptr)
{
    connect(this, &QQuickItem::windowChanged, this, &D3D11Squircle::handleWindowChanged);
}

void D3D11Squircle::setT(qreal t)
{
    if (t == m_t)
        return;
    m_t = t;
    emit tChanged();
    if (window())
        window()->update();
}

void D3D11Squircle::handleWindowChanged(QQuickWindow *win)
{
    if (win) {
        connect(win, &QQuickWindow::beforeSynchronizing, this, &D3D11Squircle::sync, Qt::DirectConnection);
        connect(win, &QQuickWindow::sceneGraphInvalidated, this, &D3D11Squircle::cleanup, Qt::DirectConnection);

        // Ensure we start with cleared to black. The squircle's blend mode relies on this.
        win->setColor(Qt::black);
    }
}

SquircleRenderer::SquircleRenderer()
    : m_t(0)
{
}

// The safe way to release custom graphics resources it to both connect to
// sceneGraphInvalidated() and implement releaseResources(). To support
// threaded render loops the latter performs the SquircleRenderer destruction
// via scheduleRenderJob(). Note that the D3D11Squircle may be gone by the time
// the QRunnable is invoked.

void D3D11Squircle::cleanup()
{
    delete m_renderer;
    m_renderer = nullptr;
}

class CleanupJob : public QRunnable
{
public:
    CleanupJob(SquircleRenderer *renderer) : m_renderer(renderer) { }
    void run() override { delete m_renderer; }
private:
    SquircleRenderer *m_renderer;
};

void D3D11Squircle::releaseResources()
{
    window()->scheduleRenderJob(new CleanupJob(m_renderer), QQuickWindow::BeforeSynchronizingStage);
    m_renderer = nullptr;
}

SquircleRenderer::~SquircleRenderer()
{
    qDebug("cleanup");

    if (m_vs)
        m_vs->Release();

    if (m_ps)
        m_ps->Release();

    if (m_vbuf)
        m_vbuf->Release();

    if (m_cbuf)
        m_cbuf->Release();

    if (m_inputLayout)
        m_inputLayout->Release();

    if (m_rastState)
        m_rastState->Release();

    if (m_dsState)
        m_dsState->Release();

    if (m_blendState)
        m_blendState->Release();
}

void D3D11Squircle::sync()
{
    if (!m_renderer) {
        m_renderer = new SquircleRenderer;
        connect(window(), &QQuickWindow::beforeRendering, m_renderer, &SquircleRenderer::frameStart, Qt::DirectConnection);
        connect(window(), &QQuickWindow::beforeRenderPassRecording, m_renderer, &SquircleRenderer::mainPassRecordingStart, Qt::DirectConnection);
    }
    m_renderer->setViewportSize(window()->size() * window()->devicePixelRatio());
    m_renderer->setT(m_t);
    m_renderer->setWindow(window());
}

void SquircleRenderer::frameStart()
{
    QSGRendererInterface *rif = m_window->rendererInterface();

    // We are not prepared for anything other than running with the RHI and its D3D11 backend.
    Q_ASSERT(rif->graphicsApi() == QSGRendererInterface::Direct3D11Rhi);

    m_device = reinterpret_cast<ID3D11Device *>(rif->getResource(m_window, QSGRendererInterface::DeviceResource));
    Q_ASSERT(m_device);
    m_context = reinterpret_cast<ID3D11DeviceContext *>(rif->getResource(m_window, QSGRendererInterface::DeviceContextResource));
    Q_ASSERT(m_context);

    if (m_vert.isEmpty())
        prepareShader(VertexStage);
    if (m_frag.isEmpty())
        prepareShader(FragmentStage);

    if (!m_initialized)
        init();
}

static const float vertices[] = {
    -1, -1,
    1, -1,
    -1, 1,
    1, 1
};

void SquircleRenderer::mainPassRecordingStart()
{
    m_window->beginExternalCommands();

    D3D11_MAPPED_SUBRESOURCE mp;
    // will copy the entire constant buffer every time -> pass WRITE_DISCARD -> prevent pipeline stalls
    HRESULT hr = m_context->Map(m_cbuf, 0, D3D11_MAP_WRITE_DISCARD, 0, &mp);
    if (SUCCEEDED(hr)) {
        float t = m_t;
        memcpy(mp.pData, &t, 4);
        m_context->Unmap(m_cbuf, 0);
    } else {
        qFatal("Failed to map constant buffer: 0x%x", hr);
    }

    D3D11_VIEWPORT v;
    v.TopLeftX = 0;
    v.TopLeftY = 0;
    v.Width = m_viewportSize.width();
    v.Height = m_viewportSize.height();
    v.MinDepth = 0;
    v.MaxDepth = 1;
    m_context->RSSetViewports(1, &v);

    m_context->VSSetShader(m_vs, nullptr, 0);
    m_context->PSSetShader(m_ps, nullptr, 0);
    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    m_context->IASetInputLayout(m_inputLayout);
    m_context->OMSetDepthStencilState(m_dsState, 0);
    float blendConstants[] = { 1, 1, 1, 1 };
    m_context->OMSetBlendState(m_blendState, blendConstants, 0xFFFFFFFF);
    m_context->RSSetState(m_rastState);

    const UINT stride = 2 * sizeof(float); // vec2
    const UINT offset = 0;
    m_context->IASetVertexBuffers(0, 1, &m_vbuf, &stride, &offset);
    m_context->PSSetConstantBuffers(0, 1, &m_cbuf);

    m_context->Draw(4, 0);

    m_window->endExternalCommands();
}

void SquircleRenderer::prepareShader(Stage stage)
{
    QString filename;
    if (stage == VertexStage) {
        filename = QLatin1String(":/scenegraph/d3d11underqml/squircle.vert");
    } else {
        Q_ASSERT(stage == FragmentStage);
        filename = QLatin1String(":/scenegraph/d3d11underqml/squircle.frag");
    }
    QFile f(filename);
    if (!f.open(QIODevice::ReadOnly))
        qFatal("Failed to read shader %s", qPrintable(filename));

    const QByteArray contents = f.readAll();

    if (stage == VertexStage) {
        m_vert = contents;
        Q_ASSERT(!m_vert.isEmpty());
        m_vertEntryPoint = QByteArrayLiteral("main");
    } else {
        m_frag = contents;
        Q_ASSERT(!m_frag.isEmpty());
        m_fragEntryPoint = QByteArrayLiteral("main");
    }
}

QByteArray SquircleRenderer::compileShader(Stage stage,
                                           const QByteArray &source,
                                           const QByteArray &entryPoint)
{
    const char *target;
    switch (stage) {
    case VertexStage:
        target = "vs_5_0";
        break;
    case FragmentStage:
        target = "ps_5_0";
        break;
    default:
        qFatal("Unknown shader stage %d", stage);
        return QByteArray();
    }

    ID3DBlob *bytecode = nullptr;
    ID3DBlob *errors = nullptr;
    HRESULT hr = D3DCompile(source.constData(), source.size(),
                            nullptr, nullptr, nullptr,
                            entryPoint.constData(), target, 0, 0, &bytecode, &errors);
    if (FAILED(hr) || !bytecode) {
        qWarning("HLSL shader compilation failed: 0x%x", uint(hr));
        if (errors) {
            const QByteArray msg(static_cast<const char *>(errors->GetBufferPointer()),
                                 errors->GetBufferSize());
            errors->Release();
            qWarning("%s", msg.constData());
        }
        return QByteArray();
    }

    QByteArray result;
    result.resize(bytecode->GetBufferSize());
    memcpy(result.data(), bytecode->GetBufferPointer(), result.size());
    bytecode->Release();

    return result;
}

void SquircleRenderer::init()
{
    qDebug("init");
    m_initialized = true;

    const QByteArray vs = compileShader(VertexStage, m_vert, m_vertEntryPoint);
    const QByteArray fs = compileShader(FragmentStage, m_frag, m_fragEntryPoint);

    HRESULT hr = m_device->CreateVertexShader(vs.constData(), vs.size(), nullptr, &m_vs);
    if (FAILED(hr))
        qFatal("Failed to create vertex shader: 0x%x", hr);

    hr = m_device->CreatePixelShader(fs.constData(), fs.size(), nullptr, &m_ps);
    if (FAILED(hr))
        qFatal("Failed to create pixel shader: 0x%x", hr);

    D3D11_BUFFER_DESC bufDesc;
    memset(&bufDesc, 0, sizeof(bufDesc));
    bufDesc.ByteWidth = sizeof(vertices);
    bufDesc.Usage = D3D11_USAGE_DEFAULT;
    bufDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    hr = m_device->CreateBuffer(&bufDesc, nullptr, &m_vbuf);
    if (FAILED(hr))
        qFatal("Failed to create buffer: 0x%x", hr);

    m_context->UpdateSubresource(m_vbuf, 0, nullptr, vertices, 0, 0);

    bufDesc.ByteWidth = 256;
    bufDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bufDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    hr = m_device->CreateBuffer(&bufDesc, nullptr, &m_cbuf);
    if (FAILED(hr))
        qFatal("Failed to create buffer: 0x%x", hr);

    D3D11_INPUT_ELEMENT_DESC inputDesc;
    memset(&inputDesc, 0, sizeof(inputDesc));
    // the output from SPIRV-Cross uses TEXCOORD<location> as the semantic
    inputDesc.SemanticName = "TEXCOORD";
    inputDesc.SemanticIndex = 0;
    inputDesc.Format = DXGI_FORMAT_R32G32_FLOAT; // vec2
    inputDesc.InputSlot = 0;
    inputDesc.AlignedByteOffset = 0;
    inputDesc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
    hr = m_device->CreateInputLayout(&inputDesc, 1, vs.constData(), vs.size(), &m_inputLayout);
    if (FAILED(hr))
        qFatal("Failed to create input layout: 0x%x", hr);

    D3D11_RASTERIZER_DESC rastDesc;
    memset(&rastDesc, 0, sizeof(rastDesc));
    rastDesc.FillMode = D3D11_FILL_SOLID;
    rastDesc.CullMode = D3D11_CULL_NONE;
    hr = m_device->CreateRasterizerState(&rastDesc, &m_rastState);
    if (FAILED(hr))
        qFatal("Failed to create rasterizer state: 0x%x", hr);

    D3D11_DEPTH_STENCIL_DESC dsDesc;
    memset(&dsDesc, 0, sizeof(dsDesc));
    hr = m_device->CreateDepthStencilState(&dsDesc, &m_dsState);
    if (FAILED(hr))
        qFatal("Failed to create depth/stencil state: 0x%x", hr);

    D3D11_BLEND_DESC blendDesc;
    memset(&blendDesc, 0, sizeof(blendDesc));
    blendDesc.IndependentBlendEnable = true;
    D3D11_RENDER_TARGET_BLEND_DESC blend;
    memset(&blend, 0, sizeof(blend));
    blend.BlendEnable = true;
    blend.SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blend.DestBlend = D3D11_BLEND_ONE;
    blend.BlendOp = D3D11_BLEND_OP_ADD;
    blend.SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
    blend.DestBlendAlpha = D3D11_BLEND_ONE;
    blend.BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blend.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    blendDesc.RenderTarget[0] = blend;
    hr = m_device->CreateBlendState(&blendDesc, &m_blendState);
    if (FAILED(hr))
        qFatal("Failed to create blend state: 0x%x", hr);
}

#include "d3d11squircle.moc"
