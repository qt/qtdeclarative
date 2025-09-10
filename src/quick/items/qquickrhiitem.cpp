// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickrhiitem_p.h"

QT_BEGIN_NAMESPACE

/*!
    \class QQuickRhiItem
    \inmodule QtQuick
    \since 6.7

    \brief The QQuickRhiItem class is a portable alternative to
    QQuickFramebufferObject that is not tied to OpenGL, but rather allows
    integrating rendering with the QRhi APIs with Qt Quick.

    QQuickRhiItem is effectively the counterpart of \l QRhiWidget in the world of
    Qt Quick. Both of these are meant to be subclassed, and they both enable
    recording QRhi-based rendering that targets an offscreen color buffer. The
    resulting 2D image is then composited with the rest of the Qt Quick scene.

    \note While QQuickRhiItem is a public Qt API, the QRhi family of classes in
    the Qt Gui module, including QShader and QShaderDescription, offer limited
    compatibility guarantees. There are no source or binary compatibility
    guarantees for these classes, meaning the API is only guaranteed to work
    with the Qt version the application was developed against. Source
    incompatible changes are however aimed to be kept at a minimum and will
    only be made in minor releases (6.7, 6.8, and so on). \c{qquickrhiitem.h}
    does not directly include any QRhi-related headers. To use those classes
    when implementing a QQuickRhiItem subclass, link to
    \c{Qt::GuiPrivate} (if using CMake), and include the appropriate headers
    with the \c rhi prefix, for example \c{#include <rhi/qrhi.h>}.

    QQuickRhiItem is a replacement for the legacy \l QQuickFramebufferObject
    class. The latter is inherently tied to OpenGL / OpenGL ES, whereas
    QQuickRhiItem works with the QRhi classes, allowing to run the same
    rendering code with Vulkan, Metal, Direct 3D 11/12, and OpenGL / OpenGL ES.
    Conceptually and functionally they are very close, and migrating from
    QQuickFramebufferObject to QQuickRhiItem is straightforward.
    QQuickFramebufferObject continues to be available to ensure compatibility
    for existing application code that works directly with the OpenGL API.

    \note QQuickRhiItem will not be functional when using the \c software
    adaptation of the Qt Quick scene graph.

    On most platforms, the scene graph rendering, and thus the rendering
    performed by the QQuickRhiItem will occur on a \l {Scene Graph and
    Rendering}{dedicated thread}. For this reason, the QQuickRhiItem class
    enforces a strict separation between the item implementation (the
    QQuickItem subclass) and the actual rendering logic. All item logic, such
    as properties and UI-related helper functions exposed to QML must be
    located in the QQuickRhiItem subclass. Everything that relates to rendering
    must be located in the QQuickRhiItemRenderer class. To avoid race
    conditions and read/write issues from two threads it is important that the
    renderer and the item never read or write shared variables. Communication
    between the item and the renderer should primarily happen via the
    QQuickRhiItem::synchronize() function. This function will be called on the
    render thread while the GUI thread is blocked. Using queued connections or
    events for communication between item and renderer is also possible.

    Applications must subclass both QQuickRhiItem and QQuickRhiItemRenderer.
    The pure virtual createRenderer() function must be reimplemented to return
    a new instance of the QQuickRhiItemRenderer subclass.

    As with QRhiWidget, QQuickRhiItem automatically managed the color buffer,
    which is a 2D texture (QRhiTexture) normally, or a QRhiRenderBuffer when
    multisampling is in use. (some 3D APIs differentiate between textures and
    renderbuffers, while with some others the underlying native resource is the
    same; renderbuffers are used mainly to allow multisampling with OpenGL ES
    3.0)

    The size of the texture will by default adapt to the size of the item (with
    the \l{QQuickWindow::effectiveDevicePixelRatio()}{device pixel ratio} taken
    into account). If the item size changes, the texture is recreated with the
    correct size. If a fixed size is preferred, set \l fixedColorBufferWidth and
    \l fixedColorBufferHeight to non-zero values.

    QQuickRhiItem is a \l{QSGTextureProvider}{texture provider} and can be used
    directly in \l {ShaderEffect}{ShaderEffects} and other classes that consume
    texture providers.

    While not a primary use case, QQuickRhiItem also allows incorporating
    rendering code that directly uses a 3D graphics API such as Vulkan, Metal,
    Direct 3D, or OpenGL. See \l QRhiCommandBuffer::beginExternal() for details
    on recording native commands within a QRhi render pass, as well as
    \l QRhiTexture::createFrom() for a way to wrap an existing native texture and
    then use it with QRhi in a subsequent render pass. See also
    \l QQuickGraphicsConfiguration regarding configuring the native 3D API
    environment (e.g. device extensions) and note that the \l QQuickWindow can be
    associated with a custom \l QVulkanInstance by calling
    \l QWindow::setVulkanInstance() early enough.

    \note QQuickRhiItem always uses the same QRhi instance the QQuickWindow
    uses (and by extension, the same OpenGL context, Vulkan device, etc.). To
    choose which underlying 3D graphics API is used, call
    \l{QQuickWindow::setGraphicsApi()}{setGraphicsApi()} on the QQuickWindow
    early enough. Changing it is not possible once the scene graph has
    initialized, and all QQuickRhiItem instances in the scene will render using
    the same 3D API.

    \section2 A simple example

    Take the following subclass of QQuickRhiItem. It is shown here in complete
    form. It renders a single triangle with a perspective projection, where the
    triangle is rotated based on the \c angle property of the custom item.
    (meaning it can be driven for example with animations such as
    \l NumberAnimation from QML)

    \snippet qquickrhiitem/qquickrhiitem_intro.cpp 0

    It is notable that this simple class is almost exactly the same as the code
    shown in the \l QRhiWidget introduction. The vertex and fragment shaders are
    the same as well. These are provided as Vulkan-style GLSL source code and
    must be processed first by the Qt shader infrastructure first. This is
    achieved either by running the \c qsb command-line tool manually, or by
    using the \l{Qt Shader Tools Build System Integration}{qt_add_shaders()}
    function in CMake. The QQuickRhiItem loads these pre-processed \c{.qsb}
    files that are shipped with the application. See \l{Qt Shader Tools} for
    more information about Qt's shader translation infrastructure.

    \c{color.vert}

    \snippet qquickrhiitem/qquickrhiitem_intro.vert 0

    \c{color.frag}

    \snippet qquickrhiitem/qquickrhiitem_intro.frag 0

    Once exposed to QML (note the \c QML_NAMED_ELEMENT), our custom item can be
    instantiated in any scene. (after importing the appropriate \c URI specified
    for \l{qt6_add_qml_module}{qt_add_qml_module} in the CMake project)

    \code
    ExampleRhiItem {
        anchors.fill: parent
        anchors.margins: 10
        NumberAnimation on angle { from: 0; to: 360; duration: 5000; loops: Animation.Infinite }
    }
    \endcode

    See \l{Scene Graph - RHI Texture Item} for a more complex example.

    \sa QQuickRhiItemRenderer, {Scene Graph - RHI Texture Item}, QRhi, {Scene Graph and Rendering}
 */

/*!
    \class QQuickRhiItemRenderer
    \inmodule QtQuick
    \since 6.7

    \brief A QQuickRhiItemRenderer implements the rendering logic of a
    QQuickRhiItem.

    \preliminary

    \note QQuickRhiItem and QQuickRhiItemRenderer are in tech preview in Qt
    6.7. \b {The API is under development and subject to change.}

    \sa QQuickRhiItem, QRhi
 */

QQuickRhiItemNode::QQuickRhiItemNode(QQuickRhiItem *item)
    : m_item(item)
{
    m_window = m_item->window();
    connect(m_window, &QQuickWindow::beforeRendering, this, &QQuickRhiItemNode::render,
            Qt::DirectConnection);
    connect(m_window, &QQuickWindow::screenChanged, this, [this]() {
        if (m_window->effectiveDevicePixelRatio() != m_dpr)
            m_item->update();
    }, Qt::DirectConnection);
}

QSGTexture *QQuickRhiItemNode::texture() const
{
    return m_sgTexture.get();
}

void QQuickRhiItemNode::resetColorBufferObjects()
{
    // owns either m_colorTexture or m_resolveTexture
    m_sgTexture.reset();

    m_colorTexture = nullptr;
    m_resolveTexture = nullptr;

    m_msaaColorBuffer.reset();
}

void QQuickRhiItemNode::resetRenderTargetObjects()
{
    m_renderTarget.reset();
    m_renderPassDescriptor.reset();
    m_depthStencilBuffer.reset();
}

void QQuickRhiItemNode::sync()
{
    if (!m_rhi) {
        m_rhi = m_window->rhi();
        if (!m_rhi) {
            qWarning("No QRhi found for window %p, QQuickRhiItem will not be functional", m_window);
            return;
        }
    }

    m_dpr = m_window->effectiveDevicePixelRatio();
    const int minTexSize = m_rhi->resourceLimit(QRhi::TextureSizeMin);
    const int maxTexSize = m_rhi->resourceLimit(QRhi::TextureSizeMax);

    QQuickRhiItemPrivate *itemD = m_item->d_func();
    QSize newSize = QSize(itemD->fixedTextureWidth, itemD->fixedTextureHeight);
    if (newSize.isEmpty())
        newSize = QSize(int(m_item->width()), int(m_item->height())) * m_dpr;

    newSize.setWidth(qMin(maxTexSize, qMax(minTexSize, newSize.width())));
    newSize.setHeight(qMin(maxTexSize, qMax(minTexSize, newSize.height())));

    if (m_colorTexture) {
        if (m_colorTexture->format() != itemD->rhiTextureFormat
            || m_colorTexture->sampleCount() != itemD->samples)
        {
            resetColorBufferObjects();
            resetRenderTargetObjects();
        }
    }

    if (m_msaaColorBuffer) {
        if (m_msaaColorBuffer->backingFormat() != itemD->rhiTextureFormat
            || m_msaaColorBuffer->sampleCount() != itemD->samples)
        {
            resetColorBufferObjects();
            resetRenderTargetObjects();
        }
    }

    if (m_sgTexture && m_sgTexture->hasAlphaChannel() != itemD->blend) {
        resetColorBufferObjects();
        resetRenderTargetObjects();
    }

    if (!m_colorTexture && itemD->samples <= 1) {
        if (!m_rhi->isTextureFormatSupported(itemD->rhiTextureFormat)) {
            qWarning("QQuickRhiItem: The requested texture format (%d) is not supported by the "
                     "underlying 3D graphics API implementation", int(itemD->rhiTextureFormat));
        }
        m_colorTexture = m_rhi->newTexture(itemD->rhiTextureFormat, newSize, itemD->samples,
                                           QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource);
        if (!m_colorTexture->create()) {
            qWarning("Failed to create backing texture for QQuickRhiItem");
            delete m_colorTexture;
            m_colorTexture = nullptr;
            return;
        }
    }

    if (itemD->samples > 1) {
        if (!m_msaaColorBuffer) {
            if (!m_rhi->isFeatureSupported(QRhi::MultisampleRenderBuffer)) {
                qWarning("QQuickRhiItem: Multisample renderbuffers are reported as unsupported; "
                         "sample count %d will not work as expected", itemD->samples);
            }
            if (!m_rhi->isTextureFormatSupported(itemD->rhiTextureFormat)) {
                qWarning("QQuickRhiItem: The requested texture format (%d) is not supported by the "
                         "underlying 3D graphics API implementation", int(itemD->rhiTextureFormat));
            }
            m_msaaColorBuffer.reset(m_rhi->newRenderBuffer(QRhiRenderBuffer::Color, newSize, itemD->samples,
                                                           {}, itemD->rhiTextureFormat));
            if (!m_msaaColorBuffer->create()) {
                qWarning("Failed to create multisample color buffer for QQuickRhiItem");
                m_msaaColorBuffer.reset();
                return;
            }
        }
        if (!m_resolveTexture) {
            m_resolveTexture = m_rhi->newTexture(itemD->rhiTextureFormat, newSize, 1,
                                                 QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource);
            if (!m_resolveTexture->create()) {
                qWarning("Failed to create resolve texture for QQuickRhiItem");
                delete m_resolveTexture;
                m_resolveTexture = nullptr;
                return;
            }
        }
    } else if (m_resolveTexture) {
        m_resolveTexture->deleteLater();
        m_resolveTexture = nullptr;
    }

    if (m_colorTexture && m_colorTexture->pixelSize() != newSize) {
        m_colorTexture->setPixelSize(newSize);
        if (!m_colorTexture->create())
            qWarning("Failed to rebuild texture for QQuickRhiItem after resizing");
    }

    if (m_msaaColorBuffer && m_msaaColorBuffer->pixelSize() != newSize) {
        m_msaaColorBuffer->setPixelSize(newSize);
        if (!m_msaaColorBuffer->create())
            qWarning("Failed to rebuild multisample color buffer for QQuickRhiitem after resizing");
    }

    if (m_resolveTexture && m_resolveTexture->pixelSize() != newSize) {
        m_resolveTexture->setPixelSize(newSize);
        if (!m_resolveTexture->create())
            qWarning("Failed to rebuild resolve texture for QQuickRhiItem after resizing");
    }

    if (!m_sgTexture) {
        QQuickWindow::CreateTextureOptions options;
        if (itemD->blend)
            options |= QQuickWindow::TextureHasAlphaChannel;
        // the QSGTexture takes ownership of the QRhiTexture
        m_sgTexture.reset(m_window->createTextureFromRhiTexture(m_colorTexture ? m_colorTexture : m_resolveTexture,
                                                                options));
        setTexture(m_sgTexture.get());
    }

    if (itemD->autoRenderTarget) {
        const QSize pixelSize = m_colorTexture ? m_colorTexture->pixelSize()
                                               : m_msaaColorBuffer->pixelSize();
        if (!m_depthStencilBuffer) {
            m_depthStencilBuffer.reset(m_rhi->newRenderBuffer(QRhiRenderBuffer::DepthStencil, pixelSize, itemD->samples));
            if (!m_depthStencilBuffer->create()) {
                qWarning("Failed to create depth-stencil buffer for QQuickRhiItem");
                resetRenderTargetObjects();
                return;
            }
        } else if (m_depthStencilBuffer->pixelSize() != pixelSize) {
            m_depthStencilBuffer->setPixelSize(pixelSize);
            if (!m_depthStencilBuffer->create()) {
                qWarning("Failed to rebuild depth-stencil buffer for QQuickRhiItem with new size");
                return;
            }
        }
        if (!m_renderTarget) {
            QRhiColorAttachment color0;
            if (m_colorTexture)
                color0.setTexture(m_colorTexture);
            else
                color0.setRenderBuffer(m_msaaColorBuffer.get());
            if (itemD->samples > 1)
                color0.setResolveTexture(m_resolveTexture);
            QRhiTextureRenderTargetDescription rtDesc(color0, m_depthStencilBuffer.get());
            m_renderTarget.reset(m_rhi->newTextureRenderTarget(rtDesc));
            m_renderPassDescriptor.reset(m_renderTarget->newCompatibleRenderPassDescriptor());
            m_renderTarget->setRenderPassDescriptor(m_renderPassDescriptor.get());
            if (!m_renderTarget->create()) {
                qWarning("Failed to create render target for QQuickRhiitem");
                resetRenderTargetObjects();
                return;
            }
        }
    } else {
        resetRenderTargetObjects();
    }

    if (newSize != itemD->effectiveTextureSize) {
        itemD->effectiveTextureSize = newSize;
        emit m_item->effectiveColorBufferSizeChanged();
    }

    QRhiCommandBuffer *cb = queryCommandBuffer();
    if (cb)
        m_renderer->initialize(cb);

    m_renderer->synchronize(m_item);
}

QRhiCommandBuffer *QQuickRhiItemNode::queryCommandBuffer()
{
    QRhiSwapChain *swapchain = m_window->swapChain();
    QSGRendererInterface *rif = m_window->rendererInterface();

    // Handle both cases: on-screen QQuickWindow vs. off-screen QQuickWindow
    // e.g. by using QQuickRenderControl to redirect into a texture.
    QRhiCommandBuffer *cb = swapchain ? swapchain->currentFrameCommandBuffer()
                                      : static_cast<QRhiCommandBuffer *>(
                                          rif->getResource(m_window, QSGRendererInterface::RhiRedirectCommandBuffer));

    if (!cb) {
        qWarning("QQuickRhiItem: Neither swapchain nor redirected command buffer are available.");
        return nullptr;
    }

    return cb;
}

void QQuickRhiItemNode::render()
{
    // called before Qt Quick starts recording its main render pass

    if (!isValid() || !m_renderPending)
        return;

    QRhiCommandBuffer *cb = queryCommandBuffer();
    if (!cb)
        return;

    m_renderPending = false;
    m_renderer->render(cb);

    markDirty(QSGNode::DirtyMaterial);
    emit textureChanged();
}

/*!
    Constructs a new QQuickRhiItem with the given \a parent.
 */
QQuickRhiItem::QQuickRhiItem(QQuickItem *parent)
    : QQuickItem(*new QQuickRhiItemPrivate, parent)
{
    setFlag(ItemHasContents);
}

/*!
    Destructor.
*/
QQuickRhiItem::~QQuickRhiItem()
{
}

/*!
    \internal
 */
QSGNode *QQuickRhiItem::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    // Changing to an empty size should not involve destroying and then later
    // recreating the node, because we do not know how expensive the user's
    // renderer setup is. Rather, keep the node if it already exist, and clamp
    // all accesses to width and height. Hence the unusual !oldNode condition here.
    if (!oldNode && (width() <= 0 || height() <= 0))
        return nullptr;

    Q_D(QQuickRhiItem);
    QQuickRhiItemNode *n = static_cast<QQuickRhiItemNode *>(oldNode);
    if (!n) {
        if (!d->node)
            d->node = new QQuickRhiItemNode(this);
        if (!d->node->hasRenderer()) {
            QQuickRhiItemRenderer *r = createRenderer();
            if (r) {
                r->node = d->node;
                d->node->setRenderer(r);
            } else {
                qWarning("No QQuickRhiItemRenderer was created; the item will not render");
                delete d->node;
                d->node = nullptr;
                return nullptr;
            }
        }
        n = d->node;
    }

    n->sync();

    if (!n->isValid()) {
        delete n;
        d->node = nullptr;
        return nullptr;
    }

    if (window()->rhi()->isYUpInFramebuffer()) {
        n->setTextureCoordinatesTransform(d->mirrorVertically
                                          ? QSGSimpleTextureNode::NoTransform
                                          : QSGSimpleTextureNode::MirrorVertically);
    } else {
        n->setTextureCoordinatesTransform(d->mirrorVertically
                                          ? QSGSimpleTextureNode::MirrorVertically
                                          : QSGSimpleTextureNode::NoTransform);
    }
    n->setFiltering(d->smooth ? QSGTexture::Linear : QSGTexture::Nearest);
    n->setRect(0, 0, qMax<int>(0, width()), qMax<int>(0, height()));

    n->scheduleUpdate();

    return n;
}

/*!
    \reimp
 */
bool QQuickRhiItem::event(QEvent *e)
{
    return QQuickItem::event(e);
}

/*!
    \reimp
 */
void QQuickRhiItem::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickItem::geometryChange(newGeometry, oldGeometry);
    if (newGeometry.size() != oldGeometry.size())
        update();
}

/*!
    \reimp
 */
void QQuickRhiItem::releaseResources()
{
    // called on the gui thread if the item is removed from scene

    Q_D(QQuickRhiItem);
    d->node = nullptr;
}

void QQuickRhiItem::invalidateSceneGraph()
{
    // called on the render thread when the scenegraph is invalidated

    Q_D(QQuickRhiItem);
    d->node = nullptr;
}

/*!
    \reimp
 */
bool QQuickRhiItem::isTextureProvider() const
{
    return true;
}

/*!
    \reimp
 */
QSGTextureProvider *QQuickRhiItem::textureProvider() const
{
    if (QQuickItem::isTextureProvider()) // e.g. if Item::layer::enabled == true
        return QQuickItem::textureProvider();

    Q_D(const QQuickRhiItem);
    if (!d->node) // create a node to have a provider, the texture will be null but that's ok
        d->node = new QQuickRhiItemNode(const_cast<QQuickRhiItem *>(this));

    return d->node;
}

/*!
    \property QQuickRhiItem::sampleCount

    This property controls for sample count for multisample antialiasing.
    By default the value is \c 1 which means MSAA is disabled.

    Valid values are 1, 4, 8, and sometimes 16 and 32.
    \l QRhi::supportedSampleCounts() can be used to query the supported sample
    counts at run time, but typically applications should request 1 (no MSAA),
    4x (normal MSAA) or 8x (high MSAA).

    \note Setting a new value implies that all QRhiGraphicsPipeline objects
    created by the renderer must use the same sample count from then on.
    Existing QRhiGraphicsPipeline objects created with a different sample count
    must not be used anymore. When the value changes, all color and
    depth-stencil buffers are destroyed and recreated automatically, and
    \l {QQuickRhiItemRenderer::}{initialize()} is invoked again. However, when
    isAutoRenderTargetEnabled() is \c false, it will be up to the application to
    manage this with regards to the depth-stencil buffer or additional color
    buffers.

    Changing the sample count from the default 1 to a higher value implies that
    \l {QQuickRhiItemRenderer::}{colorTexture()} becomes \nullptr and
    \l {QQuickRhiItemRenderer::}{msaaColorBuffer()} starts returning a
    valid object. Switching back to 1 (or 0), implies the opposite: in the next
    call to initialize() msaaColorBuffer() is going to return \nullptr, whereas
    colorTexture() becomes once again valid. In addition,
    \l {QQuickRhiItemRenderer::}{resolveTexture()}
    returns a valid (non-multisample) QRhiTexture whenever the sample count is
    greater than 1 (i.e., MSAA is in use).

    \sa QQuickRhiItemRenderer::msaaColorBuffer(),
        QQuickRhiItemRenderer::resolveTexture()
 */

int QQuickRhiItem::sampleCount() const
{
    Q_D(const QQuickRhiItem);
    return d->samples;
}

void QQuickRhiItem::setSampleCount(int samples)
{
    Q_D(QQuickRhiItem);
    if (d->samples == samples)
        return;

    d->samples = samples;
    emit sampleCountChanged();
    update();
}

/*!
    \property QQuickRhiItem::colorBufferFormat

    This property controls the texture format for the texture used as the color
    buffer. The default value is TextureFormat::RGBA8. QQuickRhiItem supports
    rendering to a subset of the formats supported by \l QRhiTexture. Only
    formats that are reported as supported from
    \l QRhi::isTextureFormatSupported() should be specified, rendering will not be
    functional otherwise.

    \note Setting a new format when the item and its renderer are already
    initialized and have rendered implies that all QRhiGraphicsPipeline objects
    created by the renderer may become unusable, if the associated
    QRhiRenderPassDescriptor is now incompatible due to the different texture
    format. Similarly to changing
    \l sampleCount dynamically, this means that initialize() or render()
    implementations must then take care of releasing the existing pipelines and
    creating new ones.
 */

QQuickRhiItem::TextureFormat QQuickRhiItem::colorBufferFormat() const
{
    Q_D(const QQuickRhiItem);
    return d->itemTextureFormat;
}

void QQuickRhiItem::setColorBufferFormat(TextureFormat format)
{
    Q_D(QQuickRhiItem);
    if (d->itemTextureFormat == format)
        return;

    d->itemTextureFormat = format;
    switch (format) {
    case TextureFormat::RGBA8:
        d->rhiTextureFormat = QRhiTexture::RGBA8;
        break;
    case TextureFormat::RGBA16F:
        d->rhiTextureFormat = QRhiTexture::RGBA16F;
        break;
    case TextureFormat::RGBA32F:
        d->rhiTextureFormat = QRhiTexture::RGBA32F;
        break;
    case TextureFormat::RGB10A2:
        d->rhiTextureFormat = QRhiTexture::RGB10A2;
        break;
    }
    emit colorBufferFormatChanged();
    update();
}

/*!
    \return the current automatic depth-stencil buffer and render target management setting.

    By default this value is \c true.

    \sa setAutoRenderTarget()
 */
bool QQuickRhiItem::isAutoRenderTargetEnabled() const
{
    Q_D(const QQuickRhiItem);
    return d->autoRenderTarget;
}

/*!
    Controls if a depth-stencil QRhiRenderBuffer and a QRhiTextureRenderTarget
    is created and maintained automatically by the item. The default value is
    \c true. Call this function early on, for example from the derived class'
    constructor, with \a enabled set to \c false to disable this.

    In automatic mode, the size and sample count of the depth-stencil buffer
    follows the color buffer texture's settings. In non-automatic mode,
    renderTarget() and depthStencilBuffer() always return \nullptr and it is
    then up to the application's implementation of initialize() to take care of
    setting up and managing these objects.
 */
void QQuickRhiItem::setAutoRenderTarget(bool enabled)
{
    Q_D(QQuickRhiItem);
    if (d->autoRenderTarget == enabled)
        return;

    d->autoRenderTarget = enabled;
    emit autoRenderTargetChanged();
    update();
}

/*!
    \property QQuickRhiItem::mirrorVertically

    This property controls if texture UVs are flipped when drawing the textured
    quad. It has no effect on the contents of the offscreen color buffer and
    the rendering implemented by the QQuickRhiItemRenderer.

    The default value is \c false.
 */

bool QQuickRhiItem::isMirrorVerticallyEnabled() const
{
    Q_D(const QQuickRhiItem);
    return d->mirrorVertically;
}

void QQuickRhiItem::setMirrorVertically(bool enable)
{
    Q_D(QQuickRhiItem);
    if (d->mirrorVertically == enable)
        return;

    d->mirrorVertically = enable;
    emit mirrorVerticallyChanged();
    update();
}

/*!
    \property QQuickRhiItem::fixedColorBufferWidth

    The fixed width, in pixels, of the item's associated texture or
    renderbuffer. Relevant when a fixed color buffer size is desired that does
    not depend on the item's size. This size has no effect on the geometry of
    the item (its size and placement within the scene), which means the
    texture's content will appear stretched (scaled up) or scaled down onto the
    item's area.

    For example, setting a size that is exactly twice the item's (pixel) size
    effectively performs 2x supersampling (rendering at twice the resolution
    and then implicitly scaling down when texturing the quad corresponding to
    the item in the scene).

    By default the value is \c 0. A value of 0 means that texture's size
    follows the item's size. (\c{texture size} = \c{item size} * \c{device
    pixel ratio}).
 */
int QQuickRhiItem::fixedColorBufferWidth() const
{
    Q_D(const QQuickRhiItem);
    return d->fixedTextureWidth;
}

void QQuickRhiItem::setFixedColorBufferWidth(int width)
{
    Q_D(QQuickRhiItem);
    if (d->fixedTextureWidth == width)
        return;

    d->fixedTextureWidth = width;
    emit fixedColorBufferWidthChanged();
    update();
}

/*!
    \property QQuickRhiItem::fixedColorBufferHeight

    The fixed height, in pixels, of the item's associated texture. Relevant when
    a fixed texture size is desired that does not depend on the item's size.
    This size has no effect on the geometry of the item (its size and placement
    within the scene), which means the texture's content will appear stretched
    (scaled up) or scaled down onto the item's area.

    For example, setting a size that is exactly twice the item's (pixel) size
    effectively performs 2x supersampling (rendering at twice the resolution
    and then implicitly scaling down when texturing the quad corresponding to
    the item in the scene).

    By default the value is \c 0. A value of 0 means that texture's size
    follows the item's size. (\c{texture size} = \c{item size} * \c{device
    pixel ratio}).
 */

int QQuickRhiItem::fixedColorBufferHeight() const
{
    Q_D(const QQuickRhiItem);
    return d->fixedTextureHeight;
}

void QQuickRhiItem::setFixedColorBufferHeight(int height)
{
    Q_D(QQuickRhiItem);
    if (d->fixedTextureHeight == height)
        return;

    d->fixedTextureHeight = height;
    emit fixedColorBufferHeightChanged();
    update();
}

/*!
    \property QQuickRhiItem::effectiveColorBufferSize

    This property exposes the size, in pixels, of the underlying color buffer
    (the QRhiTexture or QRhiRenderBuffer). It is provided for use on the GUI
    (main) thread, in QML bindings or JavaScript.

    \note QQuickRhiItemRenderer implementations, operating on the scene graph
    render thread, should not use this property. Those should rather query the
    size from the
    \l{QQuickRhiItemRenderer::renderTarget()}{render target}.

    \note The value becomes available asynchronously from the main thread's
    perspective in the sense that the value changes when rendering happens on
    the render thread. This means that this property is useful mainly in QML
    bindings. Application code must not assume that the value is up to date
    already when the QQuickRhiItem object is constructed.

    This is a read-only property.
 */

QSize QQuickRhiItem::effectiveColorBufferSize() const
{
    Q_D(const QQuickRhiItem);
    return d->effectiveTextureSize;
}

/*!
    \property QQuickRhiItem::alphaBlending

    Controls if blending is always enabled when drawing the quad textured with
    the content generated by the QQuickRhiItem and its renderer.

    The default value is \c false. This is for performance reasons: if
    semi-transparency is not involved, because the QQuickRhiItemRenderer clears
    to an opaque color and never renders fragments with alpha smaller than 1,
    then there is no point in enabling blending.

    If the QQuickRhiItemRenderer subclass renders with semi-transparency involved,
    set this property to true.

    \note Under certain conditions blending is still going to happen regardless
    of the value of this property. For example, if the item's
    \l{QQuickItem::opacity}{opacity} (more precisely, the combined opacity
    inherited from the parent chain) is smaller than 1, blending will be
    automatically enabled even when this property is set to false.

    \note The Qt Quick scene graph relies on and expect pre-multiplied alpha.
    For example, if the intention is to clear the background in the renderer to
    an alpha value of 0.5, then make sure to multiply the red, green, and blue
    clear color values with 0.5 as well. Otherwise the blending results will be
    incorrect.
 */

bool QQuickRhiItem::alphaBlending() const
{
    Q_D(const QQuickRhiItem);
    return d->blend;
}

void QQuickRhiItem::setAlphaBlending(bool enable)
{
    Q_D(QQuickRhiItem);
    if (d->blend == enable)
        return;

    d->blend = enable;
    emit alphaBlendingChanged();
    update();
}

/*!
    Constructs a new renderer.

    This function is called on the rendering thread during the scene graph sync
    phase when the GUI thread is blocked.

    \sa QQuickRhiItem::createRenderer()
 */
QQuickRhiItemRenderer::QQuickRhiItemRenderer()
{
}

/*!
    The Renderer is automatically deleted when the scene graph resources for
    the QQuickRhiItem item are cleaned up.

    This function is called on the rendering thread.

    Under certain conditions it is normal and expected that the renderer object
    is destroyed and then recreated. This is because the renderer's lifetime
    effectively follows the underlying scene graph node. For example, when
    changing the parent of a QQuickRhiItem object so that it then belongs to a
    different \l QQuickWindow, the scene graph nodes are all dropped and
    recreated due to the window change. This will also involve dropping and
    creating a new QQuickRhiItemRenderer.

    Unlike \l QRhiWidget, QQuickRhiItemRenderer has no need to implement
    additional code paths for releasing (or early-relasing) graphics resources
    created via QRhi. It is sufficient to release everything in the destructor,
    or rely on smart pointers.
 */
QQuickRhiItemRenderer::~QQuickRhiItemRenderer()
{
}

/*!
    Call this function when the content of the offscreen color buffer should be
    updated. (i.e. to request that render() is called again; the call will
    happen at a later point, and note that updates are typically throttled to
    the presentation rate)

    This function can be called from render() to schedule an update.

    \note This function should be used from inside the renderer. To update
    the item on the GUI thread, use QQuickRhiItem::update().
 */
void QQuickRhiItemRenderer::update()
{
    if (node)
        node->scheduleUpdate();
}

/*!
    \return the current QRhi object.

    Must only be called from initialize() and render().
 */
QRhi *QQuickRhiItemRenderer::rhi() const
{
    return node ? node->m_rhi : nullptr;
}

/*!
    \return the texture serving as the color buffer for the item.

    Must only be called from initialize() and render().

    Unlike the depth-stencil buffer and the QRhiRenderTarget, this texture is
    always available and is managed by the QQuickRhiItem, independent of the
    value of \l {QQuickRhiItem::}{isAutoRenderTargetEnabled}.

    \note When \l {QQuickRhiItem::}{sampleCount} is larger than 1, and so
    multisample antialiasing is enabled, the return value is \nullptr. Instead,
    query the \l QRhiRenderBuffer by calling msaaColorBuffer().

    \note The backing texture size and sample count can also be queried via the
    QRhiRenderTarget returned from renderTarget(). This can be more convenient
    and compact than querying from the QRhiTexture or QRhiRenderBuffer, because
    it works regardless of multisampling is in use or not.

    \sa msaaColorBuffer(), depthStencilBuffer(), renderTarget(), resolveTexture()
 */
QRhiTexture *QQuickRhiItemRenderer::colorTexture() const
{
    return node ? node->m_colorTexture : nullptr;
}

/*!
    \return the renderbuffer serving as the multisample color buffer for the item.

    Must only be called from initialize() and render().

    When \l {QQuickRhiItem::}{sampleCount} is larger than 1, and so multisample
    antialising is enabled, the returned QRhiRenderBuffer has a matching sample
    count and serves as the color buffer. Graphics pipelines used to render
    into this buffer must be created with the same sample count, and the
    depth-stencil buffer's sample count must match as well. The multisample
    content is expected to be resolved into the texture returned from
    resolveTexture(). When \l {QQuickRhiItem::}{isAutoRenderTargetEnabled} is
    \c true, renderTarget() is set up automatically to do this, by setting up
    msaaColorBuffer() as the
    \l{QRhiColorAttachment::renderBuffer()}{renderbuffer} of color attachment 0
    and resolveTexture() as its
    \l{QRhiColorAttachment::resolveTexture()}{resolveTexture}.

    When MSAA is not in use, the return value is \nullptr. Use colorTexture()
    instead then.

    Depending on the underlying 3D graphics API, there may be no practical
    difference between multisample textures and color renderbuffers with a
    sample count larger than 1 (QRhi may just map both to the same native
    resource type). Some older APIs however may differentiate between textures
    and renderbuffers. In order to support OpenGL ES 3.0, where multisample
    renderbuffers are available, but multisample textures are not, QQuickRhiItem
    always performs MSAA by using a multisample QRhiRenderBuffer as the color
    attachment (and never a multisample QRhiTexture).

    \note The backing texture size and sample count can also be queried via the
    QRhiRenderTarget returned from renderTarget(). This can be more convenient
    and compact than querying from the QRhiTexture or QRhiRenderBuffer, because
    it works regardless of multisampling is in use or not.

    \sa colorTexture(), depthStencilBuffer(), renderTarget(), resolveTexture()
 */
QRhiRenderBuffer *QQuickRhiItemRenderer::msaaColorBuffer() const
{
    return node ? node->m_msaaColorBuffer.get() : nullptr;
}

/*!
    \return the non-multisample texture to which the multisample content is resolved.

    The result is \nullptr when multisample antialiasing is not enabled.

    Must only be called from initialize() and render().

    With MSAA enabled, this is the texture that gets used by the item's
    underlying scene graph node when texturing a quad in the main render pass
    of Qt Quick. However, the QQuickRhiItemRenderer's rendering must target the
    (multisample) QRhiRenderBuffer returned from msaaColorBuffer(). When \l
    {QQuickRhiItem::}{isAutoRenderTargetEnabled} is \c true, this is taken care
    of by the QRhiRenderTarget returned from renderTarget(). Otherwise, it is
    up to the subclass code to correctly configure a render target object with
    both the color buffer and resolve textures.

    \sa colorTexture()
 */
QRhiTexture *QQuickRhiItemRenderer::resolveTexture() const
{
    return node ? node->m_resolveTexture : nullptr;
}

/*!
    \return the depth-stencil buffer used by the item's rendering.

    Must only be called from initialize() and render().

    Available only when \l {QQuickRhiItem::}{isAutoRenderTargetEnabled} is \c
    true. Otherwise the returned value is \nullptr and it is up the
    reimplementation of initialize() to create and manage a depth-stencil
    buffer and a QRhiTextureRenderTarget.

    \sa colorTexture(), renderTarget()
 */
QRhiRenderBuffer *QQuickRhiItemRenderer::depthStencilBuffer() const
{
    return node ? node->m_depthStencilBuffer.get() : nullptr;
}

/*!
    \return the render target object that must be used with
    \l QRhiCommandBuffer::beginPass() in reimplementations of render().

    Must only be called from initialize() and render().

    Available only when \l {QQuickRhiItem::}{isAutoRenderTargetEnabled} is \c
    true. Otherwise the returned value is \nullptr and it is up the
    reimplementation of initialize() to create and manage a depth-stencil
    buffer and a QRhiTextureRenderTarget.

    When creating \l{QRhiGraphicsPipeline}{graphics pipelines}, a
    QRhiRenderPassDescriptor is needed. This can be queried from the returned
    QRhiTextureRenderTarget by calling
    \l{QRhiTextureRenderTarget::renderPassDescriptor()}{renderPassDescriptor()}.

    \note The returned QRhiTextureRenderTarget always reports a
    \l{QRhiTextureRenderTarget::}{devicePixelRatio()} of \c 1.
    This is because only swapchains and the associated window have a concept of
    device pixel ratio, not textures, and the render target here always refers
    to a texture. If the on-screen scale factor is relevant for rendering,
    query and store it via the item's
    \c{window()->effectiveDevicePixelRatio()} in \l synchronize().
    When doing so, always prefer using \l{QQuickWindow::}{effectiveDevicePixelRatio()}
    over the base class' \l{QWindow::}{devicePixelRatio()}.

    \sa colorTexture(), depthStencilBuffer(), QQuickWindow::effectiveDevicePixelRatio()
 */
QRhiRenderTarget *QQuickRhiItemRenderer::renderTarget() const
{
    return node ? node->m_renderTarget.get() : nullptr;
}

/*!
    \fn QQuickRhiItemRenderer *QQuickRhiItem::createRenderer()

    Reimplement this function to create and return a new instance of a
    QQuickRhiItemRenderer subclass.

    This function will be called on the rendering thread while the GUI thread
    is blocked.
 */

/*!
    \fn void QQuickRhiItemRenderer::initialize(QRhiCommandBuffer *cb)

    Called when the item is initialized for the first time, when the
    associated texture's size, format, or sample count changes, or when the
    QRhi or texture change for any reason. The function is expected to
    maintain (create if not yet created, adjust and rebuild if the size has
    changed) the graphics resources used by the rendering code in render().

    To query the QRhi, QRhiTexture, and other related objects, call rhi(),
    colorTexture(), depthStencilBuffer(), and renderTarget().

    When the item size changes, the QRhi object, the color buffer texture,
    and the depth stencil buffer objects are all the same instances (so the
    getters return the same pointers) as before, but the color and
    depth/stencil buffers will likely have been rebuilt, meaning the
    \l{QRhiTexture::pixelSize()}{size} and the underlying native texture
    resource may be different than in the last invocation.

    Reimplementations should also be prepared that the QRhi object and the
    color buffer texture may change between invocations of this function. For
    example, when the item is reparented so that it belongs to a new
    QQuickWindow, the the QRhi and all related resources managed by the
    QQuickRhiItem will be different instances than before in the subsequent
    call to this function. Is is then important that all existing QRhi
    resources previously created by the subclass are destroyed because they
    belong to the previous QRhi that should not be used anymore.

    When \l {QQuickRhiItem::}{isAutoRenderTargetEnabled} is \c true, which is
    the default, a depth-stencil QRhiRenderBuffer and a QRhiTextureRenderTarget
    associated with the colorTexture() (or msaaColorBuffer()) and the
    depth-stencil buffer are created and managed automatically.
    Reimplementations of initialize() and render() can query those objects via
    depthStencilBuffer() and renderTarget(). When \l
    {QQuickRhiItem::}{isAutoRenderTargetEnabled} is set to \c false, these
    objects are no longer created and managed automatically. Rather, it will be
    up the the initialize() implementation to create buffers and set up the
    render target as it sees fit. When manually managing additional color or
    depth-stencil attachments for the render target, their size and sample
    count must always follow the size and sample count of colorTexture() (or
    msaaColorBuffer()), otherwise rendering or 3D API validation errors may
    occur.

    The subclass-created graphics resources are expected to be released in the
    destructor implementation of the subclass.

    \a cb is the QRhiCommandBuffer for the current frame. The function is
    called with a frame being recorded, but without an active render pass. The
    command buffer is provided primarily to allow enqueuing
    \l{QRhiCommandBuffer::resourceUpdate()}{resource updates} without deferring
    to render().

    This function is called on the render thread, if there is one.

    \sa render()
 */

/*!
    \fn void QQuickRhiItemRenderer::synchronize(QQuickRhiItem *item)

    This function is called on the render thread, if there is one, while the
    main/GUI thread is blocked. It is called from
    \l{QQuickItem::updatePaintNode()}{the \a {item}'s synchronize step},
    and allows reading and writing data belonging to the main and render
    threads. Typically property values stored in the QQuickRhiItem are copied
    into the QQuickRhiItemRenderer, so that they can be safely read afterwards
    in render() when the render and main threads continue to work in parallel.

    \sa initialize(), render()
 */

/*!
    \fn void QQuickRhiItemRenderer::render(QRhiCommandBuffer *cb)

    Called when the backing color buffer's contents needs updating.

    There is always at least one call to initialize() before this function is
    called.

    To request updates, call \l QQuickItem::update() when calling from QML or
    from C++ code on the main/GUI thread (e.g. when in a property setter), or
    \l update() when calling from within a QQuickRhiItemRenderer callback.
    Calling QQuickRhiItemRenderer's update() from within
    render() will lead to triggering updates continuously.

    \a cb is the QRhiCommandBuffer for the current frame. The function is
    called with a frame being recorded, but without an active render pass.

    This function is called on the render thread, if there is one.

    \sa initialize(), synchronize()
 */

QT_END_NAMESPACE
