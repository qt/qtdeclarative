/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: http://www.qt-project.org/
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
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
#include <QtQuick/private/qsgtexture_p.h>
#include <QtQuick/private/qdeclarativepixmapcache_p.h>

#include <QGuiApplication>
#include <QOpenGLContext>

#include <QDeclarativeImageProvider>
#include <private/qdeclarativeglobal_p.h>

#include <private/qobject_p.h>
#include <qmutex.h>

DEFINE_BOOL_CONFIG_OPTION(qmlFlashMode, QML_FLASH_MODE)
DEFINE_BOOL_CONFIG_OPTION(qmlTranslucentMode, QML_TRANSLUCENT_MODE)
DEFINE_BOOL_CONFIG_OPTION(qmlDisableDistanceField, QML_DISABLE_DISTANCEFIELD)

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
        , distanceFieldCacheManager(0)
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
    QHash<QDeclarativeTextureFactory *, QSGTexture *> textures;

    QSGDistanceFieldGlyphCacheManager *distanceFieldCacheManager;

    bool flashMode;
    float renderAlpha;
    bool distanceFieldDisabled;
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
}


QSGContext::~QSGContext()
{
    invalidate();
}



void QSGContext::invalidate()
{
    Q_D(QSGContext);
    qDeleteAll(d->textures.values());
    d->textures.clear();
    qDeleteAll(d->materials.values());
    d->materials.clear();
    delete d->distanceFieldCacheManager;
    d->distanceFieldCacheManager = 0;

    d->gl = 0;

    emit invalidated();
}


QSGTexture *QSGContext::textureForFactory(QDeclarativeTextureFactory *factory)
{
    Q_D(QSGContext);
    if (!factory)
        return 0;

    QSGTexture *texture = d->textures.value(factory);
    if (!texture) {
        if (QDeclarativeDefaultTextureFactory *dtf = qobject_cast<QDeclarativeDefaultTextureFactory *>(factory))
            texture = createTexture(dtf->image());
        else
            texture = factory->createTexture();
        d->textures.insert(factory, texture);
        connect(factory, SIGNAL(destroyed(QObject *)), this, SLOT(textureFactoryDestroyed(QObject *)));
    }
    return texture;
}


void QSGContext::textureFactoryDestroyed(QObject *o)
{
    Q_D(QSGContext);
    QDeclarativeTextureFactory *f = static_cast<QDeclarativeTextureFactory *>(o);

    // This function will only be called on the scene graph thread, so it is
    // safe to directly delete the texture here.
    delete d->textures.take(f);
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

    Q_ASSERT(!d->gl);
    d->gl = context;

    emit initialized();
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


void QSGContext::renderNextFrame(QSGRenderer *renderer, QOpenGLFramebufferObject *fbo)
{
    if (fbo) {
        QSGBindableFbo bindable(fbo);
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
    return new QSGDefaultRectangleNode(this);
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
QSGDistanceFieldGlyphCache *QSGContext::createDistanceFieldGlyphCache(const QRawFont &font)
{
    Q_D(QSGContext);
    return new QSGDefaultDistanceFieldGlyphCache(d->distanceFieldCacheManager, glContext(), font);
}

/*!
    Factory function for scene graph backends of the Text elements;
 */
QSGGlyphNode *QSGContext::createGlyphNode()
{
    Q_D(QSGContext);

    // ### Do something with these before final release...
    static bool doSubpixel = qApp->arguments().contains(QLatin1String("--text-subpixel-antialiasing"));
    static bool doLowQualSubpixel = qApp->arguments().contains(QLatin1String("--text-subpixel-antialiasing-lowq"));
    static bool doGray = qApp->arguments().contains(QLatin1String("--text-gray-antialiasing"));

    if (d->distanceFieldDisabled) {
        return new QSGDefaultGlyphNode;
    } else {
        if (!d->distanceFieldCacheManager) {
            d->distanceFieldCacheManager = new QSGDistanceFieldGlyphCacheManager(this);
            if (doSubpixel)
                d->distanceFieldCacheManager->setDefaultAntialiasingMode(QSGGlyphNode::HighQualitySubPixelAntialiasing);
            else if (doLowQualSubpixel)
                d->distanceFieldCacheManager->setDefaultAntialiasingMode(QSGGlyphNode::LowQualitySubPixelAntialiasing);
            else if (doGray)
               d->distanceFieldCacheManager->setDefaultAntialiasingMode(QSGGlyphNode::GrayAntialiasing);
        }

        QSGGlyphNode *node = new QSGDistanceFieldGlyphNode(d->distanceFieldCacheManager);
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
    // ### Do something with this before release...
    static bool doFrontToBack = qApp->arguments().contains(QLatin1String("--opaque-front-to-back"));
    QSGDefaultRenderer *renderer = new QSGDefaultRenderer(this);
    if (doFrontToBack) {
        printf("QSGContext: Sorting opaque nodes front to back...\n");
        renderer->setSortFrontToBackEnabled(true);
    }
    return renderer;
}




QSurfaceFormat QSGContext::defaultSurfaceFormat() const
{
    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setSamples(16);
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
    Returns a material shader for the given material.
 */

QSGMaterialShader *QSGContext::prepareMaterial(QSGMaterial *material)
{
    Q_D(QSGContext);
    QSGMaterialType *type = material->type();
    QSGMaterialShader *shader = d->materials.value(type);
    if (shader)
        return shader;

    shader = material->createShader();
    shader->compile();
    shader->initialize();
    d->materials[type] = shader;

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
