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

#define GL_GLEXT_PROTOTYPES

#include "qsgtexture_p.h"
#include <qopenglfunctions.h>
#include <QtQuick/private/qsgcontext_p.h>
#include <qthread.h>
#include <private/qdeclarativedebugtrace_p.h>

#if !defined(QT_NO_DEBUG) && (defined(Q_OS_LINUX) || defined(Q_OS_MAC))
#include <execinfo.h>
#include <QHash>
#endif

QT_BEGIN_NAMESPACE

inline static bool isPowerOfTwo(int x)
{
    // Assumption: x >= 1
    return x == (x & -x);
}

QSGTexturePrivate::QSGTexturePrivate()
    : wrapChanged(false)
    , filteringChanged(false)
    , horizontalWrap(QSGTexture::ClampToEdge)
    , verticalWrap(QSGTexture::ClampToEdge)
    , mipmapMode(QSGTexture::None)
    , filterMode(QSGTexture::Nearest)
{
}

#ifndef QT_NO_DEBUG

static int qt_debug_texture_count = 0;

#if defined(Q_OS_LINUX) || defined (Q_OS_MAC)
DEFINE_BOOL_CONFIG_OPTION(qmlDebugLeakBacktrace, QML_DEBUG_LEAK_BACKTRACE)

#define BACKTRACE_SIZE 20
class SGTextureTraceItem
{
public:
    void *backTrace[BACKTRACE_SIZE];
    size_t backTraceSize;
};

static QHash<QSGTexture*, SGTextureTraceItem*> qt_debug_allocated_textures;
#endif

inline static void qt_debug_print_texture_count()
{
    qDebug("Number of leaked textures: %i", qt_debug_texture_count);
    qt_debug_texture_count = -1;

#if defined(Q_OS_LINUX) || defined (Q_OS_MAC)
    if (qmlDebugLeakBacktrace()) {
        while (!qt_debug_allocated_textures.isEmpty()) {
            QHash<QSGTexture*, SGTextureTraceItem*>::Iterator it = qt_debug_allocated_textures.begin();
            QSGTexture* texture = it.key();
            SGTextureTraceItem* item = it.value();

            qt_debug_allocated_textures.erase(it);

            qDebug() << "------";
            qDebug() << "Leaked" << texture << "backtrace:";

            char** symbols = backtrace_symbols(item->backTrace, item->backTraceSize);

            if (symbols) {
                for (int i=0; i<(int) item->backTraceSize; i++)
                    qDebug("Backtrace <%02d>: %s", i, symbols[i]);
                free(symbols);
            }

            qDebug() << "------";

            delete item;
        }
    }
#endif
}

inline static void qt_debug_add_texture(QSGTexture* texture)
{
#if defined(Q_OS_LINUX) || defined (Q_OS_MAC)
    if (qmlDebugLeakBacktrace()) {
        SGTextureTraceItem* item = new SGTextureTraceItem;
        item->backTraceSize = backtrace(item->backTrace, BACKTRACE_SIZE);
        qt_debug_allocated_textures.insert(texture, item);
    }
#else
    Q_UNUSED(texture);
#endif // Q_OS_LINUX

    ++qt_debug_texture_count;

    static bool atexit_registered = false;
    if (!atexit_registered) {
        atexit(qt_debug_print_texture_count);
        atexit_registered = true;
    }
}

static void qt_debug_remove_texture(QSGTexture* texture)
{
#if defined(Q_OS_LINUX) || defined (Q_OS_MAC)
    if (qmlDebugLeakBacktrace()) {
        SGTextureTraceItem* item = qt_debug_allocated_textures.value(texture, 0);
        if (item) {
            qt_debug_allocated_textures.remove(texture);
            delete item;
        }
    }
#else
    Q_UNUSED(texture)
#endif

    --qt_debug_texture_count;

    if (qt_debug_texture_count < 0)
        qDebug("Material destroyed after qt_debug_print_texture_count() was called.");
}

#endif // QT_NO_DEBUG


QSGTexture::QSGTexture()
    : QObject(*(new QSGTexturePrivate))
{
#ifndef QT_NO_DEBUG
    qt_debug_add_texture(this);
#endif
}

QSGTexture::~QSGTexture()
{
#ifndef QT_NO_DEBUG
    qt_debug_remove_texture(this);
#endif
}


/*!
    \fn void QSGTexture::bind()

    Call this function to bind this texture to the current texture
    target.

    Binding a texture may also include uploading the texture data from
    a previously set QImage.

    \warning This function can only be called from the rendering thread.
 */

/*!
    This function returns a copy of the current texture which is removed
    from its atlas.

    The current texture remains unchanged, so texture coordinates do not
    need to be updated.

    Removing a texture from an atlas is primarily useful when passing
    it to a shader that operates on the texture coordinates 0-1 instead
    of the texture subrect inside the atlas.

    If the texture is not part of a texture atlas, this function returns 0.

    Implementations of this function are recommended to return the same instance
    for multiple calls to limit memory usage.

    \warning This function can only be called from the rendering thread.
 */

QSGTexture *QSGTexture::removedFromAtlas() const
{
    Q_ASSERT_X(!isAtlasTexture(), "QSGTexture::removedFromAtlas()", "Called on a non-atlas texture");
    return 0;
}

/*!
    Returns weither this texture is part of an atlas or not.

    The default implementation returns false.
 */
bool QSGTexture::isAtlasTexture() const
{
    return false;
}

/*!
    \fn int QSGTexture::textureId() const

    Returns the OpenGL texture id for this texture.

    The default value is 0, indicating that it is an invalid texture id.

    The function should at all times return the correct texture id.

    \warning This function can only be called from the rendering thread.
 */



/*!
    Returns the rectangle inside textureSize() that this texture
    represents in normalized coordinates.

    The default implementation returns a rect at position (0, 0) with
    width and height of 1.
 */
QRectF QSGTexture::textureSubRect() const
{
    return QRectF(0, 0, 1, 1);
}

/*!
    \fn bool QSGTexture::hasMipmaps() const

    Returns true if the texture data contains mipmap levels.
 */


/*!
    Sets the mipmap sampling mode to be used for the upcoming bind() call to \a filter.

    Setting the mipmap filtering has no effect it the texture does not have mipmaps.

    \sa hasMipmaps()
 */
void QSGTexture::setMipmapFiltering(Filtering filter)
{
    Q_D(QSGTexture);
    if (d->mipmapMode != (uint) filter) {
        d->mipmapMode = filter;
        d->filteringChanged = true;
    }
}

/*!
    Returns whether mipmapping should be used when sampling from this texture.
 */
QSGTexture::Filtering QSGTexture::mipmapFiltering() const
{
    return (QSGTexture::Filtering) d_func()->mipmapMode;
}


/*!
    Sets the sampling mode to be used for the upcoming bind() call to \a filter.
 */
void QSGTexture::setFiltering(QSGTexture::Filtering filter)
{
    Q_D(QSGTexture);
    if (d->filterMode != (uint) filter) {
        d->filterMode = filter;
        d->filteringChanged = true;
    }
}

QSGTexture::Filtering QSGTexture::filtering() const
{
    return (QSGTexture::Filtering) d_func()->filterMode;
}



/*!
    Sets the horizontal wrap mode to be used for the upcoming bind() call to \a hwrap
 */

void QSGTexture::setHorizontalWrapMode(WrapMode hwrap)
{
    Q_D(QSGTexture);
    if ((uint) hwrap != d->horizontalWrap) {
        d->horizontalWrap = hwrap;
        d->wrapChanged = true;
    }
}

QSGTexture::WrapMode QSGTexture::horizontalWrapMode() const
{
    return (QSGTexture::WrapMode) d_func()->horizontalWrap;
}



void QSGTexture::setVerticalWrapMode(WrapMode vwrap)
{
    Q_D(QSGTexture);
    if ((uint) vwrap != d->verticalWrap) {
        d->verticalWrap = vwrap;
        d->wrapChanged = true;
    }
}

QSGTexture::WrapMode QSGTexture::verticalWrapMode() const
{
    return (QSGTexture::WrapMode) d_func()->verticalWrap;
}


/*!
    Update the texture state to match the filtering, mipmap and wrap options
    currently set.

    If \a force is true, all properties will be updated regardless of weither
    they have changed or not.
 */
void QSGTexture::updateBindOptions(bool force)
{
    Q_D(QSGTexture);
    if (force || d->filteringChanged) {
        bool linear = d->filterMode == Linear;
        GLint minFilter = linear ? GL_LINEAR : GL_NEAREST;
        GLint magFilter = linear ? GL_LINEAR : GL_NEAREST;

        if (hasMipmaps()) {
            if (d->mipmapMode == Nearest)
                minFilter = linear ? GL_LINEAR_MIPMAP_NEAREST : GL_NEAREST_MIPMAP_NEAREST;
            else if (d->mipmapMode == Linear)
                minFilter = linear ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST_MIPMAP_LINEAR;
        }
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
        d->filteringChanged = false;
    }

    if (force || d->wrapChanged) {
#if !defined(QT_NO_DEBUG) && defined(QT_OPENGL_ES_2)
        if (d->horizontalWrap == Repeat || d->verticalWrap == Repeat) {
            bool npotSupported = QOpenGLFunctions(QOpenGLContext::currentContext()).hasOpenGLFeature(QOpenGLFunctions::NPOTTextures);
            QSize size = textureSize();
            bool isNpot = !isPowerOfTwo(size.width()) || !isPowerOfTwo(size.height());
            if (!npotSupported && isNpot)
                qWarning("Scene Graph: This system does not support the REPEAT wrap mode for non-power-of-two textures.");
        }
#endif
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, d->horizontalWrap == Repeat ? GL_REPEAT : GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, d->verticalWrap == Repeat ? GL_REPEAT : GL_CLAMP_TO_EDGE);
        d->wrapChanged = false;
    }
}

QSGPlainTexture::QSGPlainTexture()
    : QSGTexture()
    , m_texture_id(0)
    , m_has_alpha(false)
    , m_has_mipmaps(false)
    , m_dirty_bind_options(false)
    , m_owns_texture(true)
    , m_mipmaps_generated(false)
{
}


QSGPlainTexture::~QSGPlainTexture()
{
    if (m_texture_id && m_owns_texture)
        glDeleteTextures(1, &m_texture_id);
}

#ifdef QT_OPENGL_ES
static void swizzleBGRAToRGBA(QImage *image)
{
    const int width = image->width();
    const int height = image->height();
    for (int i = 0; i < height; ++i) {
        uint *p = (uint *) image->scanLine(i);
        for (int x = 0; x < width; ++x)
            p[x] = ((p[x] << 16) & 0xff0000) | ((p[x] >> 16) & 0xff) | (p[x] & 0xff00ff00);
    }
}
#endif

void QSGPlainTexture::setImage(const QImage &image)
{
    m_image = image;
    m_texture_size = image.size();
    m_has_alpha = image.hasAlphaChannel();
    m_dirty_texture = true;
    m_dirty_bind_options = true;
 }

int QSGPlainTexture::textureId() const
{
    if (m_dirty_texture) {
        if (m_image.isNull()) {
            // The actual texture and id will be updated/deleted in a later bind()
            // or ~QSGPlainTexture so just keep it minimal here.
            return 0;
        } else {
            // Generate a texture id for use later and return it.
            glGenTextures(1, &const_cast<QSGPlainTexture *>(this)->m_texture_id);
            return m_texture_id;
        }
    }
    return m_texture_id;
}

void QSGPlainTexture::setTextureId(int id)
{
    if (m_texture_id && m_owns_texture)
        glDeleteTextures(1, &m_texture_id);

    m_texture_id = id;
    m_dirty_texture = false;
    m_dirty_bind_options = true;
    m_image = QImage();
    m_mipmaps_generated = false;
}

void QSGPlainTexture::setHasMipmaps(bool mm)
{
    m_has_mipmaps = mm;
    m_mipmaps_generated = false;
}


void QSGPlainTexture::bind()
{
    if (!m_dirty_texture) {
        glBindTexture(GL_TEXTURE_2D, m_texture_id);
        if (m_has_mipmaps && !m_mipmaps_generated) {
            QOpenGLContext *ctx = QOpenGLContext::currentContext();
            ctx->functions()->glGenerateMipmap(GL_TEXTURE_2D);
            m_mipmaps_generated = true;
        }
        updateBindOptions(m_dirty_bind_options);
        m_dirty_bind_options = false;
        return;
    }

    m_dirty_texture = false;


    if (m_image.isNull()) {
        if (m_texture_id && m_owns_texture)
            glDeleteTextures(1, &m_texture_id);
        m_texture_id = 0;
        m_texture_size = QSize();
        m_has_mipmaps = false;
        m_has_alpha = false;
        return;
    }

    if (m_texture_id == 0)
        glGenTextures(1, &m_texture_id);
    glBindTexture(GL_TEXTURE_2D, m_texture_id);

    // ### TODO: check for out-of-memory situations...
    int w = m_image.width();
    int h = m_image.height();

    QImage tmp = (m_image.format() == QImage::Format_RGB32 || m_image.format() == QImage::Format_ARGB32_Premultiplied)
                 ? m_image
                 : m_image.convertToFormat(QImage::Format_ARGB32_Premultiplied);

#ifdef QT_OPENGL_ES
        swizzleBGRAToRGBA(&tmp);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, tmp.constBits());
#else
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_BGRA, GL_UNSIGNED_BYTE, tmp.constBits());
#endif

    if (m_has_mipmaps) {
        QOpenGLContext *ctx = QOpenGLContext::currentContext();
        ctx->functions()->glGenerateMipmap(GL_TEXTURE_2D);
        m_mipmaps_generated = true;
    }

    m_texture_size = QSize(w, h);
    m_texture_rect = QRectF(0, 0, 1, 1);

    updateBindOptions(m_dirty_bind_options);
    m_dirty_bind_options = false;
}


/*!
    \class QSGDynamicTexture
    \brief The QSGDynamicTexture class serves as a baseclass for dynamically changing textures,
    such as content that is rendered to FBO's.

    To update the content of the texture, call updateTexture() explicitly. Simply calling bind()
    will not update the texture.
 */


/*!
    \fn bool QSGDynamicTexture::updateTexture()

    Call this function to explicitely update the dynamic texture. Calling bind() will bind
    the content that was previously updated.

    The function returns true if the texture was changed as a resul of the update; otherwise
    returns false.
 */



QT_END_NAMESPACE
