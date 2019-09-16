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

#include "metaltextureimport.h"
#include <QtGui/QScreen>
#include <QtQuick/QQuickWindow>
#include <QtQuick/QSGTextureProvider>
#include <QtQuick/QSGSimpleTextureNode>

#include <Metal/Metal.h>

//! [1]
class CustomTextureNode : public QSGTextureProvider, public QSGSimpleTextureNode
{
    Q_OBJECT

public:
    CustomTextureNode(QQuickItem *item);
    ~CustomTextureNode();

    QSGTexture *texture() const override;

    void sync();
//! [1]
private slots:
    void render();

private:
    enum Stage {
        VertexStage,
        FragmentStage
    };
    void prepareShader(Stage stage);
    using FuncAndLib = QPair<id<MTLFunction>, id<MTLLibrary> >;
    FuncAndLib compileShaderFromSource(const QByteArray &src, const QByteArray &entryPoint);

    QQuickItem *m_item;
    QQuickWindow *m_window;
    QSize m_size;
    qreal m_dpr;
    id<MTLDevice> m_device = nil;
    id<MTLTexture> m_texture = nil;

    bool m_initialized = false;
    QByteArray m_vert;
    QByteArray m_vertEntryPoint;
    QByteArray m_frag;
    QByteArray m_fragEntryPoint;
    FuncAndLib m_vs;
    FuncAndLib m_fs;
    id<MTLBuffer> m_vbuf;
    id<MTLBuffer> m_ubuf[3];
    id<MTLRenderPipelineState> m_pipeline;

    float m_t;
};

CustomTextureItem::CustomTextureItem()
{
    setFlag(ItemHasContents, true);
}

// The beauty of using a true QSGNode: no need for complicated cleanup
// arrangements, unlike in other examples like metalunderqml, because the
// scenegraph will handle destroying the node at the appropriate time.

void CustomTextureItem::invalidateSceneGraph() // called on the render thread when the scenegraph is invalidated
{
    m_node = nullptr;
}

void CustomTextureItem::releaseResources() // called on the gui thread if the item is removed from scene
{
    m_node = nullptr;
}

//! [2]
QSGNode *CustomTextureItem::updatePaintNode(QSGNode *node, UpdatePaintNodeData *)
{
    CustomTextureNode *n = static_cast<CustomTextureNode *>(node);

    if (!n && (width() <= 0 || height() <= 0))
        return nullptr;

    if (!n) {
        m_node = new CustomTextureNode(this);
        n = m_node;
    }

    m_node->sync();

    n->setTextureCoordinatesTransform(QSGSimpleTextureNode::NoTransform);
    n->setFiltering(QSGTexture::Linear);
    n->setRect(0, 0, width(), height());

    window()->update(); // ensure getting to beforeRendering() at some point

    return n;
}
//! [2]

void CustomTextureItem::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickItem::geometryChanged(newGeometry, oldGeometry);

    if (newGeometry.size() != oldGeometry.size())
        update();
}

void CustomTextureItem::setT(qreal t)
{
    if (t == m_t)
        return;

    m_t = t;
    emit tChanged();

    update();
}

//! [3]
CustomTextureNode::CustomTextureNode(QQuickItem *item)
    : m_item(item)
{
    m_window = m_item->window();
    connect(m_window, &QQuickWindow::beforeRendering, this, &CustomTextureNode::render);
    connect(m_window, &QQuickWindow::screenChanged, this, [this]() {
        if (m_window->effectiveDevicePixelRatio() != m_dpr)
            m_item->update();
    });
//! [3]
    m_vs.first = nil;
    m_vs.second = nil;

    m_fs.first = nil;
    m_fs.second = nil;

    m_vbuf = nil;

    for (int i = 0; i < 3; ++i)
        m_ubuf[i] = nil;

    m_pipeline = nil;

    qDebug("renderer created");
}

CustomTextureNode::~CustomTextureNode()
{
    [m_pipeline release];

    [m_vbuf release];

    for (int i = 0; i < 3; ++i)
        [m_ubuf[i] release];

    [m_vs.first release];
    [m_vs.second release];

    [m_fs.first release];
    [m_fs.second release];

    delete texture();
    [m_texture release];

    qDebug("renderer destroyed");
}

QSGTexture *CustomTextureNode::texture() const
{
    return QSGSimpleTextureNode::texture();
}

static const float vertices[] = {
    -1, -1,
    1, -1,
    -1, 1,
    1, 1
};

const int UBUF_SIZE = 4;

//! [4]
void CustomTextureNode::sync()
{
    m_dpr = m_window->effectiveDevicePixelRatio();
    const QSize newSize = m_window->size() * m_dpr;
    bool needsNew = false;

    if (!texture())
        needsNew = true;

    if (newSize != m_size) {
        needsNew = true;
        m_size = newSize;
    }

    if (needsNew) {
        delete texture();
        [m_texture release];

        QSGRendererInterface *rif = m_window->rendererInterface();
        m_device = (id<MTLDevice>) rif->getResource(m_window, QSGRendererInterface::DeviceResource);
        Q_ASSERT(m_device);

        MTLTextureDescriptor *desc = [[MTLTextureDescriptor alloc] init];
        desc.textureType = MTLTextureType2D;
        desc.pixelFormat = MTLPixelFormatRGBA8Unorm;
        desc.width = m_size.width();
        desc.height = m_size.height();
        desc.mipmapLevelCount = 1;
        desc.resourceOptions = MTLResourceStorageModePrivate;
        desc.storageMode = MTLStorageModePrivate;
        desc.usage = MTLTextureUsageShaderRead | MTLTextureUsageRenderTarget;
        m_texture = [m_device newTextureWithDescriptor: desc];
        [desc release];

        QSGTexture *wrapper = m_window->createTextureFromNativeObject(QQuickWindow::NativeObjectTexture,
                                                                      &m_texture,
                                                                      0,
                                                                      m_size);

        qDebug() << "Got QSGTexture wrapper" << wrapper << "for an MTLTexture of size" << m_size;

        setTexture(wrapper);
    }
//! [4]
    if (!m_initialized && texture()) {
        m_initialized = true;

        prepareShader(VertexStage);
        prepareShader(FragmentStage);

        m_vs = compileShaderFromSource(m_vert, m_vertEntryPoint);
        m_fs = compileShaderFromSource(m_frag, m_fragEntryPoint);

        const int framesInFlight = m_window->graphicsStateInfo().framesInFlight;

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

        rpDesc.vertexFunction = m_vs.first;
        rpDesc.fragmentFunction = m_fs.first;

        rpDesc.colorAttachments[0].pixelFormat = MTLPixelFormatRGBA8Unorm;
        rpDesc.colorAttachments[0].blendingEnabled = true;
        rpDesc.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
        rpDesc.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorSourceAlpha;
        rpDesc.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOne;
        rpDesc.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOne;

        NSError *err = nil;
        m_pipeline = [m_device newRenderPipelineStateWithDescriptor: rpDesc error: &err];
        if (!m_pipeline) {
            const QString msg = QString::fromNSString(err.localizedDescription);
            qFatal("Failed to create render pipeline state: %s", qPrintable(msg));
        }
        [rpDesc release];

        qDebug("resources initialized");
    }

//! [5]
    m_t = float(static_cast<CustomTextureItem *>(m_item)->t());
//! [5]
}

// This is hooked up to beforeRendering() so we can start our own render
// command encoder. If we instead wanted to use the scenegraph's render command
// encoder (targeting the window), it should be connected to
// beforeRenderPassRecording() instead.
//! [6]
void CustomTextureNode::render()
{
    if (!m_initialized)
        return;

    // Render to m_texture.
    MTLRenderPassDescriptor *renderpassdesc = [MTLRenderPassDescriptor renderPassDescriptor];
    MTLClearColor c = MTLClearColorMake(0, 0, 0, 1);
    renderpassdesc.colorAttachments[0].loadAction = MTLLoadActionClear;
    renderpassdesc.colorAttachments[0].storeAction = MTLStoreActionStore;
    renderpassdesc.colorAttachments[0].clearColor = c;
    renderpassdesc.colorAttachments[0].texture = m_texture;

    QSGRendererInterface *rif = m_window->rendererInterface();
    id<MTLCommandBuffer> cb = (id<MTLCommandBuffer>) rif->getResource(m_window, QSGRendererInterface::CommandListResource);
    Q_ASSERT(cb);
    id<MTLRenderCommandEncoder> encoder = [cb renderCommandEncoderWithDescriptor: renderpassdesc];

    const QQuickWindow::GraphicsStateInfo &stateInfo(m_window->graphicsStateInfo());
    void *p = [m_ubuf[stateInfo.currentFrameSlot] contents];
    memcpy(p, &m_t, 4);

    MTLViewport vp;
    vp.originX = 0;
    vp.originY = 0;
    vp.width = m_size.width();
    vp.height = m_size.height();
    vp.znear = 0;
    vp.zfar = 1;
    [encoder setViewport: vp];

    [encoder setFragmentBuffer: m_ubuf[stateInfo.currentFrameSlot] offset: 0 atIndex: 0];
    [encoder setVertexBuffer: m_vbuf offset: 0 atIndex: 1];
    [encoder setRenderPipelineState: m_pipeline];
    [encoder drawPrimitives: MTLPrimitiveTypeTriangleStrip vertexStart: 0 vertexCount: 4 instanceCount: 1 baseInstance: 0];

    [encoder endEncoding];
}
//! [6]

void CustomTextureNode::prepareShader(Stage stage)
{
    QString filename;
    if (stage == VertexStage) {
        filename = QLatin1String(":/scenegraph/metaltextureimport/squircle.vert");
    } else {
        Q_ASSERT(stage == FragmentStage);
        filename = QLatin1String(":/scenegraph/metaltextureimport/squircle.frag");
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

CustomTextureNode::FuncAndLib CustomTextureNode::compileShaderFromSource(const QByteArray &src, const QByteArray &entryPoint)
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

#include "metaltextureimport.moc"
