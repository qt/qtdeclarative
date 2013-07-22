/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtQuick/private/qsgcontext_p.h>
#include <QtQuick/private/qsgdefaultrenderer_p.h>
#include <QtQuick/private/qsgdistancefieldutil_p.h>
#include <QtQuick/private/qsgdefaultdistancefieldglyphcache_p.h>
#include <QtQuick/private/qsgdefaultrectanglenode_p.h>
#include <QtQuick/private/qsgdefaultimagenode_p.h>
#include <QtQuick/private/qsgdefaultglyphnode_p.h>
#include <QtQuick/private/qsgdistancefieldglyphnode_p.h>
#include <QtQuick/private/qsgdistancefieldglyphnode_p_p.h>
#include <QtQuick/private/qsgshareddistancefieldglyphcache_p.h>
#include <QtQuick/QSGFlatColorMaterial>

#include <QtQuick/private/qsgtexture_p.h>
#include <QtQuick/private/qquickpixmapcache_p.h>

#include <QGuiApplication>
#include <QOpenGLContext>
#include <QQuickWindow>
#include <QtGui/qopenglframebufferobject.h>

#include <private/qqmlglobal_p.h>

#include <QtQuick/private/qsgtexture_p.h>
#include <QtGui/private/qguiapplication_p.h>
#include <qpa/qplatformintegration.h>

#include <qpa/qplatformsharedgraphicscache.h>

#include <private/qobject_p.h>
#include <qmutex.h>

#include <private/qqmlprofilerservice_p.h>

DEFINE_BOOL_CONFIG_OPTION(qmlFlashMode, QML_FLASH_MODE)
DEFINE_BOOL_CONFIG_OPTION(qmlTranslucentMode, QML_TRANSLUCENT_MODE)
DEFINE_BOOL_CONFIG_OPTION(qmlDisableDistanceField, QML_DISABLE_DISTANCEFIELD)


#ifndef QSG_NO_RENDER_TIMING
static bool qsg_render_timing = !qgetenv("QSG_RENDER_TIMING").isEmpty();
static QElapsedTimer qsg_renderer_timer;
#endif

/*
    Comments about this class from Gunnar:

    The QSGContext class is right now two things.. The first is the
    adaptation layer and central storage ground for all the things
    in the scene graph, like textures and materials. This part really
    belongs inside the scene graph coreapi.

    The other part is the QML adaptation classes, like how to implement
    rectangle nodes. This is not part of the scene graph core API, but
    more part of the QML adaptation of scene graph.

    If we ever move the scene graph core API into its own thing, this class
    needs to be split in two. Right now its one because we're lazy when it comes
    to defining plugin interfaces..
*/


QT_BEGIN_NAMESPACE

class QSGContextPrivate : public QObjectPrivate
{
public:
    QSGContextPrivate()
        : gl(0)
        , depthStencilBufferManager(0)
        , distanceFieldCacheManager(0)
    #if !defined(QT_OPENGL_ES) || defined(QT_OPENGL_ES_2_ANGLE)
        , distanceFieldAntialiasing(QSGGlyphNode::HighQualitySubPixelAntialiasing)
    #else
        , distanceFieldAntialiasing(QSGGlyphNode::GrayAntialiasing)
    #endif
        , flashMode(qmlFlashMode())
        , distanceFieldDisabled(qmlDisableDistanceField())
    {
        renderAlpha = qmlTranslucentMode() ? 0.5 : 1;
    }

    ~QSGContextPrivate()
    {
    }

    QOpenGLContext *gl;

    QHash<QSGMaterialType *, QSGMaterialShader *> materials;
    QMutex textureMutex;
    QHash<QQuickTextureFactory *, QSGTexture *> textures;
    QSGDepthStencilBufferManager *depthStencilBufferManager;
    QSGDistanceFieldGlyphCacheManager *distanceFieldCacheManager;

    QSGDistanceFieldGlyphNode::AntialiasingMode distanceFieldAntialiasing;

    bool flashMode;
    float renderAlpha;
    bool distanceFieldDisabled;
};

class QSGTextureCleanupEvent : public QEvent
{
public:
    QSGTextureCleanupEvent(QSGTexture *t) : QEvent(QEvent::User), texture(t) { }
    ~QSGTextureCleanupEvent() { delete texture; }
    QSGTexture *texture;
};

/*!
    \class QSGContext

    \brief The QSGContext holds the scene graph entry points for one QML engine.

    The context is not ready for use until it has a QOpenGLContext. Once that happens,
    the scene graph population can start.

    \internal
 */

QSGContext::QSGContext(QObject *parent) :
    QObject(*(new QSGContextPrivate), parent)
{
    Q_D(QSGContext);
    QByteArray mode = qgetenv("QSG_DISTANCEFIELD_ANTIALIASING");
    if (mode == "subpixel")
        d->distanceFieldAntialiasing = QSGGlyphNode::HighQualitySubPixelAntialiasing;
    else if (mode == "subpixel-lowq")
        d->distanceFieldAntialiasing = QSGGlyphNode::LowQualitySubPixelAntialiasing;
    else if (mode == "gray")
        d->distanceFieldAntialiasing = QSGGlyphNode::GrayAntialiasing;
}


QSGContext::~QSGContext()
{
    invalidate();
}



void QSGContext::invalidate()
{
    Q_D(QSGContext);
    d->textureMutex.lock();
    qDeleteAll(d->textures.values());
    d->textures.clear();
    d->textureMutex.unlock();
    qDeleteAll(d->materials.values());
    d->materials.clear();
    delete d->depthStencilBufferManager;
    d->depthStencilBufferManager = 0;
    delete d->distanceFieldCacheManager;
    d->distanceFieldCacheManager = 0;

    d->gl = 0;

    emit invalidated();
}


QSGTexture *QSGContext::textureForFactory(QQuickTextureFactory *factory, QQuickWindow *window)
{
    Q_D(QSGContext);
    if (!factory)
        return 0;

    d->textureMutex.lock();
    QSGTexture *texture = d->textures.value(factory);
    if (!texture) {
        if (QQuickDefaultTextureFactory *dtf = qobject_cast<QQuickDefaultTextureFactory *>(factory))
            texture = createTexture(dtf->image());
        else
            texture = factory->createTexture(window);
        d->textures.insert(factory, texture);
        connect(factory, SIGNAL(destroyed(QObject *)), this, SLOT(textureFactoryDestroyed(QObject *)), Qt::DirectConnection);
    }
    d->textureMutex.unlock();
    return texture;
}


void QSGContext::textureFactoryDestroyed(QObject *o)
{
    Q_D(QSGContext);
    QQuickTextureFactory *f = static_cast<QQuickTextureFactory *>(o);

    d->textureMutex.lock();
    QSGTexture *t = d->textures.take(f);
    d->textureMutex.unlock();

    if (t) {
        if (t->thread() == thread())
            t->deleteLater();
        else
            QCoreApplication::postEvent(this, new QSGTextureCleanupEvent(t));
    }
}


QOpenGLContext *QSGContext::glContext() const
{
    Q_D(const QSGContext);
    return d->gl;
}

/*!
    Initializes the scene graph context with the GL context \a context. This also
    emits the ready() signal so that the QML graph can start building scene graph nodes.
 */
void QSGContext::initialize(QOpenGLContext *context)
{
    Q_D(QSGContext);

    // Sanity check the surface format, in case it was overridden by the application
    QSurfaceFormat requested = defaultSurfaceFormat();
    QSurfaceFormat actual = context->format();
    if (requested.depthBufferSize() > 0 && actual.depthBufferSize() <= 0)
        qWarning("QSGContext::initialize: depth buffer support missing, expect rendering errors");
    if (requested.stencilBufferSize() > 0 && actual.stencilBufferSize() <= 0)
        qWarning("QSGContext::initialize: stencil buffer support missing, expect rendering errors");

    Q_ASSERT(!d->gl);
    d->gl = context;

    precompileMaterials();

    emit initialized();
}

#define QSG_PRECOMPILE_MATERIAL(name) { name m; prepareMaterial(&m); }

/*
 * Some glsl compilers take their time compiling materials, and
 * the way the scene graph is being processed, these materials
 * get compiled when they are first taken into use. This can
 * easily lead to skipped frames. By precompiling the most
 * common materials, we potentially add a few milliseconds to the
 * start up, and reduce the chance of avoiding skipped frames
 * later on.
 */
void QSGContext::precompileMaterials()
{
    if (qEnvironmentVariableIsEmpty("QSG_NO_MATERIAL_PRELOADING")) {
        QSG_PRECOMPILE_MATERIAL(QSGVertexColorMaterial);
        QSG_PRECOMPILE_MATERIAL(QSGFlatColorMaterial);
        QSG_PRECOMPILE_MATERIAL(QSGOpaqueTextureMaterial);
        QSG_PRECOMPILE_MATERIAL(QSGTextureMaterial);
        QSG_PRECOMPILE_MATERIAL(QSGSmoothTextureMaterial);
        QSG_PRECOMPILE_MATERIAL(QSGSmoothColorMaterial);
        QSG_PRECOMPILE_MATERIAL(QSGDistanceFieldTextMaterial);
    }
}


/*!
    Returns if the scene graph context is ready or not, meaning that it has a valid
    GL context.
 */
bool QSGContext::isReady() const
{
    Q_D(const QSGContext);
    return d->gl;
}


void QSGContext::renderNextFrame(QSGRenderer *renderer, GLuint fboId)
{
    if (fboId) {
        QSGBindableFboId bindable(fboId);
        renderer->renderScene(bindable);
    } else {
        renderer->renderScene();
    }

}

/*!
    Factory function for scene graph backends of the Rectangle element.
 */
QSGRectangleNode *QSGContext::createRectangleNode()
{
    return new QSGDefaultRectangleNode;
}

/*!
    Factory function for scene graph backends of the Image element.
 */
QSGImageNode *QSGContext::createImageNode()
{
    return new QSGDefaultImageNode;
}

/*!
    Factory function for scene graph backends of the distance-field glyph cache.
 */
QSGDistanceFieldGlyphCache *QSGContext::distanceFieldGlyphCache(const QRawFont &font)
{
    Q_D(QSGContext);

    if (!d->distanceFieldCacheManager)
        d->distanceFieldCacheManager = new QSGDistanceFieldGlyphCacheManager;

    QSGDistanceFieldGlyphCache *cache = d->distanceFieldCacheManager->cache(font);
    if (!cache) {
        QPlatformIntegration *platformIntegration = QGuiApplicationPrivate::platformIntegration();
        if (platformIntegration != 0
            && platformIntegration->hasCapability(QPlatformIntegration::SharedGraphicsCache)) {
            QFontEngine *fe = QRawFontPrivate::get(font)->fontEngine;
            if (!fe->faceId().filename.isEmpty()) {
                QByteArray keyName = fe->faceId().filename;
                if (font.style() != QFont::StyleNormal)
                    keyName += QByteArray(" I");
                if (font.weight() != QFont::Normal)
                    keyName += ' ' + QByteArray::number(font.weight());
                keyName += QByteArray(" DF");
                QPlatformSharedGraphicsCache *sharedGraphicsCache =
                        platformIntegration->createPlatformSharedGraphicsCache(keyName);

                if (sharedGraphicsCache != 0) {
                    sharedGraphicsCache->ensureCacheInitialized(keyName,
                                                                QPlatformSharedGraphicsCache::OpenGLTexture,
                                                                QPlatformSharedGraphicsCache::Alpha8);

                    cache = new QSGSharedDistanceFieldGlyphCache(keyName,
                                                                sharedGraphicsCache,
                                                                d->distanceFieldCacheManager,
                                                                glContext(),
                                                                font);
                }
            }
        }
        if (!cache)
            cache = new QSGDefaultDistanceFieldGlyphCache(d->distanceFieldCacheManager, glContext(), font);
        d->distanceFieldCacheManager->insertCache(font, cache);
    }
    return cache;
}

/*!
    Factory function for scene graph backends of the Text elements which supports native
    text rendering. Used in special cases where native look and feel is a main objective.
*/
QSGGlyphNode *QSGContext::createNativeGlyphNode()
{
#if defined(QT_OPENGL_ES) && !defined(QT_OPENGL_ES_2_ANGLE)
    return createGlyphNode();
#else
    return new QSGDefaultGlyphNode;
#endif
}

/*!
    Factory function for scene graph backends of the Text elements;
 */
QSGGlyphNode *QSGContext::createGlyphNode()
{
    Q_D(QSGContext);

    if (d->distanceFieldDisabled) {
        return createNativeGlyphNode();
    } else {
        QSGDistanceFieldGlyphNode *node = new QSGDistanceFieldGlyphNode(this);
        node->setPreferredAntialiasingMode(d->distanceFieldAntialiasing);
        return node;
    }
}

/*!
    Factory function for the scene graph renderers.

    The renderers are used for the toplevel renderer and once for every
    QQuickShaderEffectSource used in the QML scene.
 */
QSGRenderer *QSGContext::createRenderer()
{
    return new QSGDefaultRenderer(this);
}




QSurfaceFormat QSGContext::defaultSurfaceFormat() const
{
    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    if (QQuickWindow::hasDefaultAlphaBuffer())
        format.setAlphaBufferSize(8);
    format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    return format;
}


/*!
    Factory function for texture objects.

    If \a image is a valid image, the QSGTexture::setImage function
    will be called with \a image as argument.
 */

QSGTexture *QSGContext::createTexture(const QImage &image) const
{
    QSGPlainTexture *t = new QSGPlainTexture();
    if (!image.isNull())
        t->setImage(image);
    return t;
}



/*!
    Returns the minimum supported framebuffer object size.
 */

QSize QSGContext::minimumFBOSize() const
{
#ifdef Q_OS_MAC
    return QSize(33, 33);
#else
    return QSize(1, 1);
#endif
}



/*!
    Returns a shared pointer to a depth stencil buffer that can be used with \a fbo.
  */
QSharedPointer<QSGDepthStencilBuffer> QSGContext::depthStencilBufferForFbo(QOpenGLFramebufferObject *fbo)
{
    Q_D(QSGContext);
    if (!d->gl)
        return QSharedPointer<QSGDepthStencilBuffer>();
    QSGDepthStencilBufferManager *manager = depthStencilBufferManager();
    QSGDepthStencilBuffer::Format format;
    format.size = fbo->size();
    format.samples = fbo->format().samples();
    format.attachments = QSGDepthStencilBuffer::DepthAttachment | QSGDepthStencilBuffer::StencilAttachment;
    QSharedPointer<QSGDepthStencilBuffer> buffer = manager->bufferForFormat(format);
    if (buffer.isNull()) {
        buffer = QSharedPointer<QSGDepthStencilBuffer>(new QSGDefaultDepthStencilBuffer(d->gl, format));
        manager->insertBuffer(buffer);
    }
    return buffer;
}

/*!
    Returns a pointer to the context's depth/stencil buffer manager. This is useful for custom
    implementations of \l depthStencilBufferForFbo().
  */
QSGDepthStencilBufferManager *QSGContext::depthStencilBufferManager()
{
    Q_D(QSGContext);
    if (!d->gl)
        return 0;
    if (!d->depthStencilBufferManager)
        d->depthStencilBufferManager = new QSGDepthStencilBufferManager(d->gl);
    return d->depthStencilBufferManager;
}


/*!
    Returns a material shader for the given material.
 */

QSGMaterialShader *QSGContext::prepareMaterial(QSGMaterial *material)
{
    Q_D(QSGContext);
    QSGMaterialType *type = material->type();
    QSGMaterialShader *shader = d->materials.value(type);
    if (shader)
        return shader;

#ifndef QSG_NO_RENDER_TIMING
    if (qsg_render_timing  || QQmlProfilerService::enabled)
        qsg_renderer_timer.start();
#endif

    shader = material->createShader();
    shader->compile();
    shader->initialize();
    d->materials[type] = shader;

#ifndef QSG_NO_RENDER_TIMING
    if (qsg_render_timing)
        printf("   - compiling material: %dms\n", (int) qsg_renderer_timer.elapsed());

    if (QQmlProfilerService::enabled) {
        QQmlProfilerService::sceneGraphFrame(
                    QQmlProfilerService::SceneGraphContextFrame,
                    qsg_renderer_timer.nsecsElapsed());
    }
#endif

    return shader;
}



/*!
    Sets whether the scene graph should render with flashing update rectangles or not
  */

void QSGContext::setFlashModeEnabled(bool enabled)
{
    d_func()->flashMode = enabled;
}


/*!
    Returns true if the scene graph should be rendered with flashing update rectangles
 */
bool QSGContext::isFlashModeEnabled() const
{
    return d_func()->flashMode;
}


/*!
    Sets the toplevel opacity for rendering. This value will be multiplied into all
    drawing calls where possible.

    The default value is 1. Any other value will cause artifacts and is primarily
    useful for debugging.
 */
void QSGContext::setRenderAlpha(qreal renderAlpha)
{
    d_func()->renderAlpha = renderAlpha;
}


/*!
    Returns the toplevel opacity used for rendering.

    The default value is 1.

    \sa setRenderAlpha()
 */
qreal QSGContext::renderAlpha() const
{
    return d_func()->renderAlpha;
}


/*!
    Sets whether or not the scene graph should use the distance field technique to render text
  */
void QSGContext::setDistanceFieldEnabled(bool enabled)
{
    d_func()->distanceFieldDisabled = !enabled;
}


/*!
    Returns true if the scene graph uses the distance field technique to render text
 */
bool QSGContext::isDistanceFieldEnabled() const
{
    return !d_func()->distanceFieldDisabled;
}



/*!
    Creates a new animation driver.
 */

QAnimationDriver *QSGContext::createAnimationDriver(QObject *parent)
{
    return new QAnimationDriver(parent);
}


QT_END_NAMESPACE
