// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsgsoftwarecontext_p.h"

#include "qsgsoftwareinternalrectanglenode_p.h"
#include "qsgsoftwareinternalimagenode_p.h"
#include "qsgsoftwarepainternode_p.h"
#include "qsgsoftwarepixmaptexture_p.h"
#include "qsgsoftwareglyphnode_p.h"
#include "qsgsoftwarepublicnodes_p.h"
#include "qsgsoftwarelayer_p.h"
#include "qsgsoftwarerenderer_p.h"
#if QT_CONFIG(quick_sprite)
#include "qsgsoftwarespritenode_p.h"
#endif

#include <QtCore/QCoreApplication>
#include <QtCore/QElapsedTimer>

#include <QtGui/QWindow>
#include <QtQuick/private/qquickwindow_p.h>
#include <QtQuick/private/qquickitem_p.h>

// Used for very high-level info about the renderering and gl context
// Includes GL_VERSION, type of render loop, atlas size, etc.
Q_LOGGING_CATEGORY(QSG_RASTER_LOG_INFO,                "qt.scenegraph.info")

// Used to debug the renderloop logic. Primarily useful for platform integrators
// and when investigating the render loop logic.
Q_LOGGING_CATEGORY(QSG_RASTER_LOG_RENDERLOOP,          "qt.scenegraph.renderloop")

// GLSL shader compilation
Q_LOGGING_CATEGORY(QSG_RASTER_LOG_TIME_COMPILATION,    "qt.scenegraph.time.compilation")

// polish, animations, sync, render and swap in the render loop
Q_LOGGING_CATEGORY(QSG_RASTER_LOG_TIME_RENDERLOOP,     "qt.scenegraph.time.renderloop")

// Texture uploads and swizzling
Q_LOGGING_CATEGORY(QSG_RASTER_LOG_TIME_TEXTURE,        "qt.scenegraph.time.texture")

// Glyph preparation (only for distance fields atm)
Q_LOGGING_CATEGORY(QSG_RASTER_LOG_TIME_GLYPH,          "qt.scenegraph.time.glyph")

// Timing inside the renderer base class
Q_LOGGING_CATEGORY(QSG_RASTER_LOG_TIME_RENDERER,       "qt.scenegraph.time.renderer")

QT_BEGIN_NAMESPACE

QSGSoftwareRenderContext::QSGSoftwareRenderContext(QSGContext *ctx)
    : QSGRenderContext(ctx)
    , m_initialized(false)
    , m_activePainter(nullptr)
{
}

QSGSoftwareContext::QSGSoftwareContext(QObject *parent)
    : QSGContext(parent)
{
}

QSGInternalRectangleNode *QSGSoftwareContext::createInternalRectangleNode()
{
    return new QSGSoftwareInternalRectangleNode();
}

QSGInternalImageNode *QSGSoftwareContext::createInternalImageNode(QSGRenderContext *renderContext)
{
    Q_UNUSED(renderContext);
    return new QSGSoftwareInternalImageNode();
}

QSGPainterNode *QSGSoftwareContext::createPainterNode(QQuickPaintedItem *item)
{
    return new QSGSoftwarePainterNode(item);
}

QSGGlyphNode *QSGSoftwareContext::createGlyphNode(QSGRenderContext *rc, bool preferNativeGlyphNode, int renderTypeQuality)
{
    Q_UNUSED(rc);
    Q_UNUSED(preferNativeGlyphNode);
    Q_UNUSED(renderTypeQuality);
    return new QSGSoftwareGlyphNode();
}

QSGLayer *QSGSoftwareContext::createLayer(QSGRenderContext *renderContext)
{
    return new QSGSoftwareLayer(renderContext);
}

QSurfaceFormat QSGSoftwareContext::defaultSurfaceFormat() const
{
    QSurfaceFormat format = QSurfaceFormat::defaultFormat();
    format.setRenderableType(QSurfaceFormat::DefaultRenderableType);
    format.setMajorVersion(0);
    format.setMinorVersion(0);
    return format;
}

void QSGSoftwareRenderContext::initializeIfNeeded()
{
    if (m_initialized)
        return;
    m_initialized = true;
    emit initialized();
}

void QSGSoftwareRenderContext::invalidate()
{
    qDeleteAll(m_texturesToDelete);
    m_texturesToDelete.clear();

    qDeleteAll(m_textures);
    m_textures.clear();

    Q_ASSERT(m_fontEnginesToClean.isEmpty());

    qDeleteAll(m_glyphCaches);
    m_glyphCaches.clear();

    m_sg->renderContextInvalidated(this);
    emit invalidated();
}

QSGTexture *QSGSoftwareRenderContext::createTexture(const QImage &image, uint flags) const
{
    return new QSGSoftwarePixmapTexture(image, flags);
}

QSGRenderer *QSGSoftwareRenderContext::createRenderer(QSGRendererInterface::RenderMode)
{
    return new QSGSoftwareRenderer(this);
}


void QSGSoftwareRenderContext::renderNextFrame(QSGRenderer *renderer)
{
    renderer->renderScene();
}

int QSGSoftwareRenderContext::maxTextureSize() const
{
    return 2048;
}

QSGRendererInterface *QSGSoftwareContext::rendererInterface(QSGRenderContext *renderContext)
{
    Q_UNUSED(renderContext);
    return this;
}

QSGRectangleNode *QSGSoftwareContext::createRectangleNode()
{
    return new QSGSoftwareRectangleNode;
}

QSGImageNode *QSGSoftwareContext::createImageNode()
{
    return new QSGSoftwareImageNode;
}

QSGNinePatchNode *QSGSoftwareContext::createNinePatchNode()
{
    return new QSGSoftwareNinePatchNode;
}

#if QT_CONFIG(quick_sprite)
QSGSpriteNode *QSGSoftwareContext::createSpriteNode()
{
    return new QSGSoftwareSpriteNode;
}
#endif

QSGRendererInterface::GraphicsApi QSGSoftwareContext::graphicsApi() const
{
    return Software;
}

QSGRendererInterface::ShaderType QSGSoftwareContext::shaderType() const
{
    return UnknownShadingLanguage;
}

QSGRendererInterface::ShaderCompilationTypes QSGSoftwareContext::shaderCompilationType() const
{
    return {};
}

QSGRendererInterface::ShaderSourceTypes QSGSoftwareContext::shaderSourceType() const
{
    return {};
}

void *QSGSoftwareContext::getResource(QQuickWindow *window, Resource resource) const
{
    if (!window)
        return nullptr;

    auto cd = QQuickWindowPrivate::get(window);

    if (resource == PainterResource)
        return window->isSceneGraphInitialized() ? static_cast<QSGSoftwareRenderContext *>(cd->context)->m_activePainter : nullptr;
    else if (resource == RedirectPaintDevice)
        return cd->redirect.rt.paintDevice;

    return nullptr;
}

QT_END_NAMESPACE

#include "moc_qsgsoftwarecontext_p.cpp"
