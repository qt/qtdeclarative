/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "metalrenderer.h"
#include <QQuickItem>
#include <QQuickWindow>

#include <Metal/Metal.h>

using FuncAndLib = QPair<id<MTLFunction>, id<MTLLibrary> >;

const int MAX_FRAMES_IN_FLIGHT = 3;

struct {
    id<MTLDevice> dev = nil;
    QByteArray vsSource;
    FuncAndLib vs;
    QByteArray fsSource;
    FuncAndLib fs;
    id<MTLBuffer> vbuf[MAX_FRAMES_IN_FLIGHT];
    id<MTLBuffer> ubuf[MAX_FRAMES_IN_FLIGHT];
    id<MTLDepthStencilState> stencilEnabledDsState = nil;
    id<MTLRenderPipelineState> pipeline = nil;
} g;

static FuncAndLib compileShaderFromSource(const QByteArray &src, const QByteArray &entryPoint)
{
    FuncAndLib fl;

    NSString *srcstr = [NSString stringWithUTF8String: src.constData()];
    MTLCompileOptions *opts = [[MTLCompileOptions alloc] init];
    opts.languageVersion = MTLLanguageVersion1_2;
    NSError *err = nil;
    fl.second = [g.dev newLibraryWithSource: srcstr options: opts error: &err];
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

const int VERTEX_SIZE = 6 * sizeof(float);

static float colors[] = {
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 1.0f
};

void MetalRenderNodeResourceBuilder::build()
{
    if (!g.dev) {
        QSGRendererInterface *rif = m_window->rendererInterface();
        Q_ASSERT(rif->graphicsApi() == QSGRendererInterface::MetalRhi);

        g.dev = (id<MTLDevice>) rif->getResource(m_window, QSGRendererInterface::DeviceResource);
        Q_ASSERT(g.dev);
    }

    if (g.vsSource.isEmpty()) {
        const QString filename = QLatin1String(":/scenegraph/rendernode/metalshader.vert");
        QFile f(filename);
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
            qFatal("Failed to read shader %s", qPrintable(filename));
        g.vsSource = f.readAll();
        g.vs = compileShaderFromSource(g.vsSource, QByteArrayLiteral("main0"));
    }

    if (g.fsSource.isEmpty()) {
        const QString filename = QLatin1String(":/scenegraph/rendernode/metalshader.frag");
        QFile f(filename);
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
            qFatal("Failed to read shader %s", qPrintable(filename));
        g.fsSource = f.readAll();
        g.fs = compileShaderFromSource(g.fsSource, QByteArrayLiteral("main0"));
    }

    const int framesInFlight = m_window->graphicsStateInfo().framesInFlight;

    // For simplicity's sake we use shared mode (something like host visible +
    // host coherent) for everything.

    for (int i = 0; i < framesInFlight; ++i) {
        // Have multiple versions for vertex too since we'll just memcpy new
        // vertices based on item width and height on every render(). This could
        // be optimized further however.
        if (!g.vbuf[i]) {
            g.vbuf[i] = [g.dev newBufferWithLength: VERTEX_SIZE + sizeof(colors) options: MTLResourceStorageModeShared];
            char *p = (char *) [g.vbuf[i] contents];
            memcpy(p + VERTEX_SIZE, colors, sizeof(colors));
        }

        if (!g.ubuf[i])
            g.ubuf[i] = [g.dev newBufferWithLength: 256 options: MTLResourceStorageModeShared];
    }

    if (!g.stencilEnabledDsState) {
        MTLDepthStencilDescriptor *dsDesc = [[MTLDepthStencilDescriptor alloc] init];
        dsDesc.frontFaceStencil = [[MTLStencilDescriptor alloc] init];
        dsDesc.frontFaceStencil.stencilFailureOperation = MTLStencilOperationKeep;
        dsDesc.frontFaceStencil.depthFailureOperation = MTLStencilOperationKeep;
        dsDesc.frontFaceStencil.depthStencilPassOperation = MTLStencilOperationKeep;
        dsDesc.frontFaceStencil.stencilCompareFunction = MTLCompareFunctionEqual;
        dsDesc.frontFaceStencil.readMask = 0xFF;
        dsDesc.frontFaceStencil.writeMask = 0xFF;

        dsDesc.backFaceStencil = [[MTLStencilDescriptor alloc] init];
        dsDesc.backFaceStencil.stencilFailureOperation = MTLStencilOperationKeep;
        dsDesc.backFaceStencil.depthFailureOperation = MTLStencilOperationKeep;
        dsDesc.backFaceStencil.depthStencilPassOperation = MTLStencilOperationKeep;
        dsDesc.backFaceStencil.stencilCompareFunction = MTLCompareFunctionEqual;
        dsDesc.backFaceStencil.readMask = 0xFF;
        dsDesc.backFaceStencil.writeMask = 0xFF;

        g.stencilEnabledDsState = [g.dev newDepthStencilStateWithDescriptor: dsDesc];
        [dsDesc release];
    }

    if (!g.pipeline) {
        MTLVertexDescriptor *inputLayout = [MTLVertexDescriptor vertexDescriptor];
        inputLayout.attributes[0].format = MTLVertexFormatFloat2;
        inputLayout.attributes[0].offset = 0;
        inputLayout.attributes[0].bufferIndex = 1; // ubuf is 0, vbuf is 1 and 2
        inputLayout.attributes[1].format = MTLVertexFormatFloat3;
        inputLayout.attributes[1].offset = 0;
        inputLayout.attributes[1].bufferIndex = 2;
        inputLayout.layouts[1].stride = 2 * sizeof(float);
        inputLayout.layouts[2].stride = 3 * sizeof(float);

        MTLRenderPipelineDescriptor *rpDesc = [[MTLRenderPipelineDescriptor alloc] init];
        rpDesc.vertexDescriptor = inputLayout;

        rpDesc.vertexFunction = g.vs.first;
        rpDesc.fragmentFunction = g.fs.first;

        rpDesc.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
        rpDesc.colorAttachments[0].blendingEnabled = true;
        rpDesc.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorOne;
        rpDesc.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorOne;
        rpDesc.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        rpDesc.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;

        if (g.dev.depth24Stencil8PixelFormatSupported) {
            rpDesc.depthAttachmentPixelFormat = MTLPixelFormatDepth24Unorm_Stencil8;
            rpDesc.stencilAttachmentPixelFormat = MTLPixelFormatDepth24Unorm_Stencil8;
        } else {
            rpDesc.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float_Stencil8;
            rpDesc.stencilAttachmentPixelFormat = MTLPixelFormatDepth32Float_Stencil8;
        }

        NSError *err = nil;
        g.pipeline = [g.dev newRenderPipelineStateWithDescriptor: rpDesc error: &err];
        if (!g.pipeline) {
            const QString msg = QString::fromNSString(err.localizedDescription);
            qFatal("Failed to create render pipeline state: %s", qPrintable(msg));
        }
        [rpDesc release];
    }
}

MetalRenderNode::MetalRenderNode()
{
    g.vs.first = g.fs.first = nil;
    g.vs.second = g.fs.second = nil;

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        g.vbuf[i] = nil;
        g.ubuf[i] = nil;
    }
}

MetalRenderNode::~MetalRenderNode()
{
    releaseResources();
}

void MetalRenderNode::releaseResources()
{
    [g.stencilEnabledDsState release];
    g.stencilEnabledDsState = nil;

    [g.pipeline release];
    g.pipeline = nil;

    [g.vs.first release];
    [g.vs.second release];

    [g.fs.first release];
    [g.fs.second release];

    g.vs.first = g.fs.first = nil;
    g.vs.second = g.fs.second = nil;

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        [g.vbuf[i] release];
        g.vbuf[i] = nil;
        [g.ubuf[i] release];
        g.ubuf[i] = nil;
    }
}

void MetalRenderNode::render(const RenderState *state)
{
    Q_ASSERT(m_window);
    const QQuickWindow::GraphicsStateInfo &stateInfo(m_window->graphicsStateInfo());
    id<MTLBuffer> vbuf = g.vbuf[stateInfo.currentFrameSlot];
    id<MTLBuffer> ubuf = g.ubuf[stateInfo.currentFrameSlot];

    QPointF p0(m_width - 1, m_height - 1);
    QPointF p1(0, 0);
    QPointF p2(0, m_height - 1);

    float vertices[6] = { float(p0.x()), float(p0.y()),
                          float(p1.x()), float(p1.y()),
                          float(p2.x()), float(p2.y()) };
    char *p = (char *) [vbuf contents];
    memcpy(p, vertices, VERTEX_SIZE);

    const QMatrix4x4 mvp = *state->projectionMatrix() * *matrix();
    const float opacity = inheritedOpacity();

    p = (char *) [ubuf contents];
    memcpy(p, mvp.constData(), 64);
    memcpy(p + 64, &opacity, 4);

    QSGRendererInterface *rif = m_window->rendererInterface();
    id<MTLRenderCommandEncoder> encoder = (id<MTLRenderCommandEncoder>) rif->getResource(
                m_window, QSGRendererInterface::CommandEncoderResource);
    Q_ASSERT(encoder);

    [encoder setVertexBuffer: vbuf offset: 0 atIndex: 1];
    [encoder setVertexBuffer: vbuf offset: VERTEX_SIZE atIndex: 2];

    [encoder setVertexBuffer: ubuf offset: 0 atIndex: 0];
    [encoder setFragmentBuffer: ubuf offset: 0 atIndex: 0];

    // Clip support.
    if (state->scissorEnabled()) {
        const QRect r = state->scissorRect(); // bottom-up
        MTLScissorRect s;
        s.x = r.x();
        s.y = m_outputHeight - (r.y() + r.height());
        s.width = r.width();
        s.height = r.height();
        [encoder setScissorRect: s];
    }
    if (state->stencilEnabled()) {
        [encoder setDepthStencilState: g.stencilEnabledDsState];
        [encoder setStencilReferenceValue: state->stencilValue()];
    }

    [encoder setRenderPipelineState: g.pipeline];
    [encoder drawPrimitives: MTLPrimitiveTypeTriangle vertexStart: 0 vertexCount: 3 instanceCount: 1 baseInstance: 0];
}

QSGRenderNode::StateFlags MetalRenderNode::changedStates() const
{
    return BlendState | ScissorState | StencilState;
}

QSGRenderNode::RenderingFlags MetalRenderNode::flags() const
{
    return BoundedRectRendering | DepthAwareRendering;
}

QRectF MetalRenderNode::rect() const
{
    return QRect(0, 0, m_width, m_height);
}

void MetalRenderNode::sync(QQuickItem *item)
{
    m_window = item->window();
    m_width = item->width();
    m_height = item->height();
    m_outputHeight = m_window->height() * m_window->effectiveDevicePixelRatio();
}
