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

#include "metalsquircle.h"
#include <QtCore/QRunnable>
#include <QtQuick/QQuickWindow>

#include <Metal/Metal.h>

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
    using FuncAndLib = QPair<id<MTLFunction>, id<MTLLibrary> >;
    FuncAndLib compileShaderFromSource(const QByteArray &src, const QByteArray &entryPoint);
    void init(int framesInFlight);

    QSize m_viewportSize;
    qreal m_t;
    QQuickWindow *m_window;

    QByteArray m_vert;
    QByteArray m_vertEntryPoint;
    QByteArray m_frag;
    QByteArray m_fragEntryPoint;

    bool m_initialized = false;
    id<MTLDevice> m_device;
    id<MTLBuffer> m_vbuf;
    id<MTLBuffer> m_ubuf[3];
    FuncAndLib m_vs;
    FuncAndLib m_fs;
    id<MTLRenderPipelineState> m_pipeline;
};

MetalSquircle::MetalSquircle()
    : m_t(0)
    , m_renderer(nullptr)
{
    connect(this, &QQuickItem::windowChanged, this, &MetalSquircle::handleWindowChanged);
}

void MetalSquircle::setT(qreal t)
{
    if (t == m_t)
        return;
    m_t = t;
    emit tChanged();
    if (window())
        window()->update();
}

void MetalSquircle::handleWindowChanged(QQuickWindow *win)
{
    if (win) {
        connect(win, &QQuickWindow::beforeSynchronizing, this, &MetalSquircle::sync, Qt::DirectConnection);
        connect(win, &QQuickWindow::sceneGraphInvalidated, this, &MetalSquircle::cleanup, Qt::DirectConnection);

        // Ensure we start with cleared to black. The squircle's blend mode relies on this.
        win->setColor(Qt::black);
    }
}

SquircleRenderer::SquircleRenderer()
    : m_t(0)
{
    m_vbuf = nil;

    for (int i = 0; i < 3; ++i)
        m_ubuf[i] = nil;

    m_vs.first = nil;
    m_vs.second = nil;

    m_fs.first = nil;
    m_fs.second = nil;
}

// The safe way to release custom graphics resources is to both connect to
// sceneGraphInvalidated() and implement releaseResources(). To support
// threaded render loops the latter performs the SquircleRenderer destruction
// via scheduleRenderJob(). Note that the MetalSquircle may be gone by the time
// the QRunnable is invoked.

void MetalSquircle::cleanup()
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

void MetalSquircle::releaseResources()
{
    window()->scheduleRenderJob(new CleanupJob(m_renderer), QQuickWindow::BeforeSynchronizingStage);
    m_renderer = nullptr;
}

SquircleRenderer::~SquircleRenderer()
{
    qDebug("cleanup");

    [m_vbuf release];
    for (int i = 0; i < 3; ++i)
        [m_ubuf[i] release];

    [m_vs.first release];
    [m_vs.second release];

    [m_fs.first release];
    [m_fs.second release];
}

void MetalSquircle::sync()
{
    if (!m_renderer) {
        m_renderer = new SquircleRenderer;
        // Initializing resources is done before starting to encode render
        // commands, regardless of wanting an underlay or overlay.
        connect(window(), &QQuickWindow::beforeRendering, m_renderer, &SquircleRenderer::frameStart, Qt::DirectConnection);
        // Here we want an underlay and therefore connect to
        // beforeRenderPassRecording. Changing to afterRenderPassRecording
        // would render the squircle on top (overlay).
        connect(window(), &QQuickWindow::beforeRenderPassRecording, m_renderer, &SquircleRenderer::mainPassRecordingStart, Qt::DirectConnection);
    }
    m_renderer->setViewportSize(window()->size() * window()->devicePixelRatio());
    m_renderer->setT(m_t);
    m_renderer->setWindow(window());
}

void SquircleRenderer::frameStart()
{
    QSGRendererInterface *rif = m_window->rendererInterface();

    // We are not prepared for anything other than running with the RHI and its Metal backend.
    Q_ASSERT(rif->graphicsApi() == QSGRendererInterface::MetalRhi);

    m_device = (id<MTLDevice>) rif->getResource(m_window, QSGRendererInterface::DeviceResource);
    Q_ASSERT(m_device);

    if (m_vert.isEmpty())
        prepareShader(VertexStage);
    if (m_frag.isEmpty())
        prepareShader(FragmentStage);

    if (!m_initialized)
        init(m_window->graphicsStateInfo().framesInFlight);
}

static const float vertices[] = {
    -1, -1,
    1, -1,
    -1, 1,
    1, 1
};

const int UBUF_SIZE = 4;

void SquircleRenderer::mainPassRecordingStart()
{
    // This example demonstrates the simple case: prepending some commands to
    // the scenegraph's main renderpass. It does not create its own passes,
    // rendertargets, etc. so no synchronization is needed.

    const QQuickWindow::GraphicsStateInfo &stateInfo(m_window->graphicsStateInfo());

    QSGRendererInterface *rif = m_window->rendererInterface();
    id<MTLRenderCommandEncoder> encoder = (id<MTLRenderCommandEncoder>) rif->getResource(
                m_window, QSGRendererInterface::CommandEncoderResource);
    Q_ASSERT(encoder);

    m_window->beginExternalCommands();

    void *p = [m_ubuf[stateInfo.currentFrameSlot] contents];
    float t = m_t;
    memcpy(p, &t, 4);

    MTLViewport vp;
    vp.originX = 0;
    vp.originY = 0;
    vp.width = m_viewportSize.width();
    vp.height = m_viewportSize.height();
    vp.znear = 0;
    vp.zfar = 1;
    [encoder setViewport: vp];

    [encoder setFragmentBuffer: m_ubuf[stateInfo.currentFrameSlot] offset: 0 atIndex: 0];
    [encoder setVertexBuffer: m_vbuf offset: 0 atIndex: 1];
    [encoder setRenderPipelineState: m_pipeline];
    [encoder drawPrimitives: MTLPrimitiveTypeTriangleStrip vertexStart: 0 vertexCount: 4 instanceCount: 1 baseInstance: 0];

    m_window->endExternalCommands();
}

void SquircleRenderer::prepareShader(Stage stage)
{
    QString filename;
    if (stage == VertexStage) {
        filename = QLatin1String(":/scenegraph/metalunderqml/squircle.vert");
    } else {
        Q_ASSERT(stage == FragmentStage);
        filename = QLatin1String(":/scenegraph/metalunderqml/squircle.frag");
    }
    QFile f(filename);
    if (!f.open(QIODevice::ReadOnly))
        qFatal("Failed to read shader %s", qPrintable(filename));

    const QByteArray contents = f.readAll();

    if (stage == VertexStage) {
        m_vert = contents;
        Q_ASSERT(!m_vert.isEmpty());
        m_vertEntryPoint = QByteArrayLiteral("main0");
    } else {
        m_frag = contents;
        Q_ASSERT(!m_frag.isEmpty());
        m_fragEntryPoint = QByteArrayLiteral("main0");
    }
}

SquircleRenderer::FuncAndLib SquircleRenderer::compileShaderFromSource(const QByteArray &src, const QByteArray &entryPoint)
{
    FuncAndLib fl;

    NSString *srcstr = [NSString stringWithUTF8String: src.constData()];
    MTLCompileOptions *opts = [[MTLCompileOptions alloc] init];
    opts.languageVersion = MTLLanguageVersion1_2;
    NSError *err = nil;
    fl.second = [m_device newLibraryWithSource: srcstr options: opts error: &err];
    [opts release];
    // srcstr is autoreleased

    if (err) {
        const QString msg = QString::fromNSString(err.localizedDescription);
        qFatal("%s", qPrintable(msg));
        return fl;
    }

    NSString *name = [NSString stringWithUTF8String: entryPoint.constData()];
    fl.first = [fl.second newFunctionWithName: name];
    [name release];

    return fl;
}

void SquircleRenderer::init(int framesInFlight)
{
    qDebug("init");

    Q_ASSERT(framesInFlight <= 3);
    m_initialized = true;

    m_vbuf = [m_device newBufferWithLength: sizeof(vertices) options: MTLResourceStorageModeShared];
    void *p = [m_vbuf contents];
    memcpy(p, vertices, sizeof(vertices));

    for (int i = 0; i < framesInFlight; ++i)
        m_ubuf[i] = [m_device newBufferWithLength: UBUF_SIZE options: MTLResourceStorageModeShared];

    MTLVertexDescriptor *inputLayout = [MTLVertexDescriptor vertexDescriptor];
    inputLayout.attributes[0].format = MTLVertexFormatFloat2;
    inputLayout.attributes[0].offset = 0;
    inputLayout.attributes[0].bufferIndex = 1; // ubuf is 0, vbuf is 1
    inputLayout.layouts[1].stride = 2 * sizeof(float);

    MTLRenderPipelineDescriptor *rpDesc = [[MTLRenderPipelineDescriptor alloc] init];
    rpDesc.vertexDescriptor = inputLayout;

    m_vs = compileShaderFromSource(m_vert, m_vertEntryPoint);
    rpDesc.vertexFunction = m_vs.first;
    m_fs = compileShaderFromSource(m_frag, m_fragEntryPoint);
    rpDesc.fragmentFunction = m_fs.first;

    rpDesc.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
    rpDesc.colorAttachments[0].blendingEnabled = true;
    rpDesc.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
    rpDesc.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorSourceAlpha;
    rpDesc.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOne;
    rpDesc.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOne;

#ifdef Q_OS_MACOS
    if (m_device.depth24Stencil8PixelFormatSupported) {
        rpDesc.depthAttachmentPixelFormat = MTLPixelFormatDepth24Unorm_Stencil8;
        rpDesc.stencilAttachmentPixelFormat = MTLPixelFormatDepth24Unorm_Stencil8;
    } else
#endif
    {
        rpDesc.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float_Stencil8;
        rpDesc.stencilAttachmentPixelFormat = MTLPixelFormatDepth32Float_Stencil8;
    }

    NSError *err = nil;
    m_pipeline = [m_device newRenderPipelineStateWithDescriptor: rpDesc error: &err];
    if (!m_pipeline) {
        const QString msg = QString::fromNSString(err.localizedDescription);
        qFatal("Failed to create render pipeline state: %s", qPrintable(msg));
    }
    [rpDesc release];
}

#include "metalsquircle.moc"
