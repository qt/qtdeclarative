// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsgdefaultrendercontext_p.h"
#include "qsgcurveglyphatlas_p.h"

#include <QtGui/QGuiApplication>

#include <QtQuick/private/qsgbatchrenderer_p.h>
#include <QtQuick/private/qsgrenderer_p.h>
#include <QtQuick/private/qsgrhiatlastexture_p.h>
#include <QtQuick/private/qsgrhidistancefieldglyphcache_p.h>
#include <QtQuick/private/qsgmaterialshader_p.h>

#include <QtQuick/private/qsgcompressedtexture_p.h>

#include <QtQuick/qsgrendererinterface.h>
#include <QtQuick/qquickgraphicsconfiguration.h>

QT_BEGIN_NAMESPACE

QSGDefaultRenderContext::QSGDefaultRenderContext(QSGContext *context)
    : QSGRenderContext(context)
    , m_rhi(nullptr)
    , m_maxTextureSize(0)
    , m_rhiAtlasManager(nullptr)
    , m_currentFrameCommandBuffer(nullptr)
    , m_currentFrameRenderPass(nullptr)
    , m_useDepthBufferFor2D(true)
    , m_glyphCacheResourceUpdates(nullptr)
{
}

/*!
    Initializes the scene graph render context with the GL context \a context. This also
    emits the ready() signal so that the QML graph can start building scene graph nodes.
 */
void QSGDefaultRenderContext::initialize(const QSGRenderContext::InitParams *params)
{
    if (!m_sg)
        return;

    const InitParams *initParams = static_cast<const InitParams *>(params);
    if (initParams->sType != INIT_PARAMS_MAGIC)
        qFatal("QSGDefaultRenderContext: Invalid parameters passed to initialize()");

    m_initParams = *initParams;

    m_rhi = m_initParams.rhi;
    m_maxTextureSize = m_rhi->resourceLimit(QRhi::TextureSizeMax);
    if (!m_rhiAtlasManager)
        m_rhiAtlasManager = new QSGRhiAtlasTexture::Manager(this, m_initParams.initialSurfacePixelSize, m_initParams.maybeSurface);

    m_glyphCacheResourceUpdates = nullptr;

    m_sg->renderContextInitialized(this);

    emit initialized();
}

void QSGDefaultRenderContext::invalidateGlyphCaches()
{
    {
        auto it = m_glyphCaches.begin();
        while (it != m_glyphCaches.end()) {
            if (!(*it)->isActive()) {
                delete *it;
                it = m_glyphCaches.erase(it);
            } else {
                ++it;
            }
        }
    }

    qDeleteAll(m_curveGlyphAtlases);
    m_curveGlyphAtlases.clear();

    {
        auto it = m_fontEnginesToClean.begin();
        while (it != m_fontEnginesToClean.end()) {
            if (it.value() == 0) {
                it.key()->clearGlyphCache(this);
                if (!it.key()->ref.deref())
                    delete it.key();
                it = m_fontEnginesToClean.erase(it);
            } else {
                ++it;
            }
        }
    }
}

void QSGDefaultRenderContext::invalidate()
{
    if (!m_rhi)
        return;

    qDeleteAll(m_texturesToDelete);
    m_texturesToDelete.clear();

    qDeleteAll(m_textures);
    m_textures.clear();

    for (auto it = m_depthStencilBuffers.cbegin(), end = m_depthStencilBuffers.cend(); it != end; ++it) {
        QSharedPointer<QSGDepthStencilBuffer> buf = it.value().toStrongRef();
        delete buf->ds;
        buf->ds = nullptr;
        buf->rc = nullptr;
    }
    m_depthStencilBuffers.clear();

    /* The cleanup of the atlas textures is a bit intriguing.
       As part of the cleanup in the threaded render loop, we
       do:
       1. call this function
       2. call QCoreApp::sendPostedEvents() to immediately process
          any pending deferred deletes.
       3. delete the GL context.

       As textures need the atlas manager while cleaning up, the
       manager needs to be cleaned up after the textures, so
       we post a deleteLater here at the very bottom so it gets
       deferred deleted last.

       Another alternative would be to use a QPointer in
       QSGOpenGLAtlasTexture::Texture, but this seemed simpler.
     */
    if (m_rhiAtlasManager) {
        m_rhiAtlasManager->invalidate();
        m_rhiAtlasManager->deleteLater();
        m_rhiAtlasManager = nullptr;
    }

    // The following piece of code will read/write to the font engine's caches,
    // potentially from different threads. However, this is safe because this
    // code is only called from QQuickWindow's shutdown which is called
    // only when the GUI is blocked, and multiple threads will call it in
    // sequence. (see qsgdefaultglyphnode_p.cpp's init())
    for (auto it = m_fontEnginesToClean.constBegin(); it != m_fontEnginesToClean.constEnd(); ++it) {
        it.key()->clearGlyphCache(this);
        if (!it.key()->ref.deref())
            delete it.key();
    }
    m_fontEnginesToClean.clear();

    qDeleteAll(m_curveGlyphAtlases);
    m_curveGlyphAtlases.clear();

    qDeleteAll(m_glyphCaches);
    m_glyphCaches.clear();

    resetGlyphCacheResources();

    m_rhi = nullptr;

    if (m_sg)
        m_sg->renderContextInvalidated(this);

    emit invalidated();
}

void QSGDefaultRenderContext::prepareSync(qreal devicePixelRatio,
                                          QRhiCommandBuffer *cb,
                                          const QQuickGraphicsConfiguration &config)
{
    m_currentDevicePixelRatio = devicePixelRatio;
    m_useDepthBufferFor2D = config.isDepthBufferEnabledFor2D();

    // we store the command buffer already here, in case there is something in
    // an updatePaintNode() implementation that leads to needing it (for
    // example, an updateTexture() call on a QSGRhiLayer)
    m_currentFrameCommandBuffer = cb;
}

void QSGDefaultRenderContext::beginNextFrame(QSGRenderer *renderer, const QSGRenderTarget &renderTarget,
                                             RenderPassCallback mainPassRecordingStart,
                                             RenderPassCallback mainPassRecordingEnd,
                                             void *callbackUserData)
{
    renderer->setRenderTarget(renderTarget);
    renderer->setRenderPassRecordingCallbacks(mainPassRecordingStart, mainPassRecordingEnd, callbackUserData);

    m_currentFrameCommandBuffer = renderTarget.cb; // usually the same as what was passed to prepareSync() but cannot count on that having been called
    m_currentFrameRenderPass = renderTarget.rpDesc;
}

void QSGDefaultRenderContext::renderNextFrame(QSGRenderer *renderer)
{
    renderer->renderScene();
}

void QSGDefaultRenderContext::endNextFrame(QSGRenderer *renderer)
{
    Q_UNUSED(renderer);
    m_currentFrameCommandBuffer = nullptr;
    m_currentFrameRenderPass = nullptr;
}

QSGTexture *QSGDefaultRenderContext::createTexture(const QImage &image, uint flags) const
{
    bool atlas = flags & CreateTexture_Atlas;
    bool mipmap = flags & CreateTexture_Mipmap;
    bool alpha = flags & CreateTexture_Alpha;

    // The atlas implementation is only supported from the render thread and
    // does not support mipmaps.
    if (m_rhi) {
        if (!mipmap && atlas && QThread::currentThread() == m_rhi->thread()) {
            QSGTexture *t = m_rhiAtlasManager->create(image, alpha);
            if (t)
                return t;
        }
    }

    QSGPlainTexture *texture = new QSGPlainTexture;
    texture->setImage(image);
    if (texture->hasAlphaChannel() && !alpha)
        texture->setHasAlphaChannel(false);

    return texture;
}

QSGRenderer *QSGDefaultRenderContext::createRenderer(QSGRendererInterface::RenderMode renderMode)
{
    return new QSGBatchRenderer::Renderer(this, renderMode);
}

QSGTexture *QSGDefaultRenderContext::compressedTextureForFactory(const QSGCompressedTextureFactory *factory) const
{
    // This is only used for atlasing compressed textures. Returning null implies no atlas.

    if (m_rhi && QThread::currentThread() == m_rhi->thread())
        return m_rhiAtlasManager->create(factory);

    return nullptr;
}

void QSGDefaultRenderContext::initializeRhiShader(QSGMaterialShader *shader, QShader::Variant shaderVariant)
{
    QSGMaterialShaderPrivate::get(shader)->prepare(shaderVariant);
}

void QSGDefaultRenderContext::preprocess()
{
    for (auto it = m_glyphCaches.begin(); it != m_glyphCaches.end(); ++it) {
        it.value()->processPendingGlyphs();
        it.value()->update();
    }
}

QSGCurveGlyphAtlas *QSGDefaultRenderContext::curveGlyphAtlas(const QRawFont &font)
{
    FontKey key = FontKey(font, 0);
    QSGCurveGlyphAtlas *atlas = m_curveGlyphAtlases.value(key, nullptr);
    if (atlas == nullptr) {
        atlas = new QSGCurveGlyphAtlas(font);
        m_curveGlyphAtlases.insert(key, atlas);
    }

    return atlas;
}

QSGDistanceFieldGlyphCache *QSGDefaultRenderContext::distanceFieldGlyphCache(const QRawFont &font, int renderTypeQuality)
{
    FontKey key(font, renderTypeQuality);
    QSGDistanceFieldGlyphCache *cache = m_glyphCaches.value(key, 0);
    if (!cache && font.isValid()) {
        cache = new QSGRhiDistanceFieldGlyphCache(this, font, renderTypeQuality);
        m_glyphCaches.insert(key, cache);
    }

    return cache;
}

QRhiResourceUpdateBatch *QSGDefaultRenderContext::maybeGlyphCacheResourceUpdates()
{
    return m_glyphCacheResourceUpdates;
}

QRhiResourceUpdateBatch *QSGDefaultRenderContext::glyphCacheResourceUpdates()
{
    if (!m_glyphCacheResourceUpdates)
        m_glyphCacheResourceUpdates = m_rhi->nextResourceUpdateBatch();

    return m_glyphCacheResourceUpdates;
}

void QSGDefaultRenderContext::deferredReleaseGlyphCacheTexture(QRhiTexture *texture)
{
    if (texture)
        m_pendingGlyphCacheTextures.insert(texture);
}

void QSGDefaultRenderContext::resetGlyphCacheResources()
{
    if (m_glyphCacheResourceUpdates) {
        m_glyphCacheResourceUpdates->release();
        m_glyphCacheResourceUpdates = nullptr;
    }

    for (QRhiTexture *t : std::as_const(m_pendingGlyphCacheTextures))
        t->deleteLater(); // the QRhiTexture object stays valid for the current frame

    m_pendingGlyphCacheTextures.clear();
}

QSGDepthStencilBuffer::~QSGDepthStencilBuffer()
{
    if (rc && ds) {
        const auto key = std::pair(ds->pixelSize(), ds->sampleCount());
        rc->m_depthStencilBuffers.remove(key);
    }
    delete ds;
}

QSharedPointer<QSGDepthStencilBuffer> QSGDefaultRenderContext::getDepthStencilBuffer(const QSize &size, int sampleCount)
{
    const auto key = std::pair(size, sampleCount);
    auto it = m_depthStencilBuffers.constFind(key);
    if (it != m_depthStencilBuffers.cend()) {
        if (it.value())
            return it.value().toStrongRef();
    }

    QRhiRenderBuffer *ds = m_rhi->newRenderBuffer(QRhiRenderBuffer::DepthStencil, size, sampleCount);
    if (!ds->create()) {
        qWarning("Failed to build depth-stencil buffer for layer");
        delete ds;
        return {};
    }
    QSharedPointer<QSGDepthStencilBuffer> buf(new QSGDepthStencilBuffer(ds));
    addDepthStencilBuffer(buf);
    return buf;
}

void QSGDefaultRenderContext::addDepthStencilBuffer(const QSharedPointer<QSGDepthStencilBuffer> &ds)
{
    if (ds) {
        const auto key = std::pair(ds->ds->pixelSize(), ds->ds->sampleCount());
        ds->rc = this;
        m_depthStencilBuffers.insert(key, ds.toWeakRef());
    }
}

QT_END_NAMESPACE

#include "moc_qsgdefaultrendercontext_p.cpp"
