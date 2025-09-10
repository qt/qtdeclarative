// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsgopenvgcontext_p.h"
#include "qsgopenvgrenderer_p.h"
#include "qsgopenvgpublicnodes.h"
#include "qsgopenvgtexture.h"
#include "qsgopenvglayer.h"
#include "qsgopenvgglyphnode_p.h"
#include "qsgopenvgfontglyphcache.h"
#include "qsgopenvgpainternode.h"
#if QT_CONFIG(quick_sprite)
#include "qsgopenvgspritenode.h"
#endif

#include "qopenvgcontext_p.h"

#include <private/qsgrenderer_p.h>
#include "qsgopenvginternalrectanglenode.h"
#include "qsgopenvginternalimagenode.h"

// polish, animations, sync, render and swap in the render loop
Q_LOGGING_CATEGORY(QSG_OPENVG_LOG_TIME_RENDERLOOP,     "qt.scenegraph.time.renderloop")

QT_BEGIN_NAMESPACE

QSGOpenVGRenderContext::QSGOpenVGRenderContext(QSGContext *context)
    : QSGRenderContext(context)
    , m_vgContext(nullptr)
    , m_glyphCacheManager(nullptr)
{

}

void QSGOpenVGRenderContext::initialize(const QSGRenderContext::InitParams *params)
{
    const InitParams *vgparams = static_cast<const InitParams *>(params);
    if (vgparams->sType != INIT_PARAMS_MAGIC)
        qFatal("Invalid OpenVG render context parameters");

    m_vgContext = vgparams->context;
    QSGRenderContext::initialize(params);
    emit initialized();
}

void QSGOpenVGRenderContext::invalidate()
{
    m_vgContext = nullptr;
    delete m_glyphCacheManager;
    m_glyphCacheManager = nullptr;
    QSGRenderContext::invalidate();
    emit invalidated();
}

void QSGOpenVGRenderContext::renderNextFrame(QSGRenderer *renderer)
{
    renderer->renderScene();
}

QSGTexture *QSGOpenVGRenderContext::createTexture(const QImage &image, uint flags) const
{
    QImage tmp = image;

    // Make sure image is not larger than maxTextureSize
    int maxSize = maxTextureSize();
    if (tmp.width() > maxSize || tmp.height() > maxSize) {
        tmp = tmp.scaled(qMin(maxSize, tmp.width()), qMin(maxSize, tmp.height()), Qt::IgnoreAspectRatio, Qt::FastTransformation);
    }

    return new QSGOpenVGTexture(tmp, flags);
}

QSGRenderer *QSGOpenVGRenderContext::createRenderer(QSGRendererInterface::RenderMode)
{
    return new QSGOpenVGRenderer(this);
}

QSGOpenVGContext::QSGOpenVGContext(QObject *parent)
{
    Q_UNUSED(parent);
}

QSGRenderContext *QSGOpenVGContext::createRenderContext()
{
    return new QSGOpenVGRenderContext(this);
}

QSGRectangleNode *QSGOpenVGContext::createRectangleNode()
{
    return new QSGOpenVGRectangleNode;
}

QSGImageNode *QSGOpenVGContext::createImageNode()
{
    return new QSGOpenVGImageNode;
}

QSGPainterNode *QSGOpenVGContext::createPainterNode(QQuickPaintedItem *item)
{
    Q_UNUSED(item);
    return new QSGOpenVGPainterNode(item);
}

QSGGlyphNode *QSGOpenVGContext::createGlyphNode(QSGRenderContext *rc, bool preferNativeGlyphNode, int renderTypeQuality)
{
    Q_UNUSED(preferNativeGlyphNode);
    Q_UNUSED(renderTypeQuality);
    return new QSGOpenVGGlyphNode(rc);
}

QSGNinePatchNode *QSGOpenVGContext::createNinePatchNode()
{
    return new QSGOpenVGNinePatchNode;
}

QSGLayer *QSGOpenVGContext::createLayer(QSGRenderContext *renderContext)
{
    return new QSGOpenVGLayer(renderContext);
}

QSurfaceFormat QSGOpenVGContext::defaultSurfaceFormat() const
{
    QSurfaceFormat format = QSurfaceFormat::defaultFormat();
    format.setRenderableType(QSurfaceFormat::OpenVG);
    format.setMajorVersion(1);
    return format;
}

QSGInternalRectangleNode *QSGOpenVGContext::createInternalRectangleNode()
{
    return new QSGOpenVGInternalRectangleNode();
}

QSGInternalImageNode *QSGOpenVGContext::createInternalImageNode(QSGRenderContext *)
{
    return new QSGOpenVGInternalImageNode();
}

int QSGOpenVGRenderContext::maxTextureSize() const
{
    VGint width = vgGeti(VG_MAX_IMAGE_WIDTH);
    VGint height = vgGeti(VG_MAX_IMAGE_HEIGHT);

    return qMin(width, height);
}

#if QT_CONFIG(quick_sprite)
QSGSpriteNode *QSGOpenVGContext::createSpriteNode()
{
    return new QSGOpenVGSpriteNode();
}
#endif

QSGRendererInterface *QSGOpenVGContext::rendererInterface(QSGRenderContext *renderContext)
{
    return static_cast<QSGOpenVGRenderContext *>(renderContext);
}

QSGRendererInterface::GraphicsApi QSGOpenVGRenderContext::graphicsApi() const
{
    return OpenVG;
}

QSGRendererInterface::ShaderType QSGOpenVGRenderContext::shaderType() const
{
    return UnknownShadingLanguage;
}

QSGRendererInterface::ShaderCompilationTypes QSGOpenVGRenderContext::shaderCompilationType() const
{
    return 0;
}

QSGRendererInterface::ShaderSourceTypes QSGOpenVGRenderContext::shaderSourceType() const
{
    return 0;
}

QSGOpenVGFontGlyphCache *QSGOpenVGRenderContext::glyphCache(const QRawFont &rawFont)
{
    if (!m_glyphCacheManager)
        m_glyphCacheManager = new QSGOpenVGFontGlyphCacheManager;

    QSGOpenVGFontGlyphCache *cache = m_glyphCacheManager->cache(rawFont);
    if (!cache) {
        cache = new QSGOpenVGFontGlyphCache(m_glyphCacheManager, rawFont);
        m_glyphCacheManager->insertCache(rawFont, cache);
    }

    return cache;
}

QT_END_NAMESPACE
