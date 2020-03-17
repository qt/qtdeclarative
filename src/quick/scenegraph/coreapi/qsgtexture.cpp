/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qsgtexture_p.h"
#if QT_CONFIG(opengl)
# include <QtGui/qopenglcontext.h>
# include <QtGui/qopenglfunctions.h>
#endif
#include <private/qqmlglobal_p.h>
#include <private/qsgmaterialshader_p.h>
#include <QtGui/private/qrhi_p.h>

#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID) && defined(__GLIBC__)
#define CAN_BACKTRACE_EXECINFO
#endif

#if defined(Q_OS_MAC)
#define CAN_BACKTRACE_EXECINFO
#endif

#if defined(QT_NO_DEBUG)
#undef CAN_BACKTRACE_EXECINFO
#endif

#if defined(CAN_BACKTRACE_EXECINFO)
#include <execinfo.h>
#include <QHash>
#endif

#ifndef QT_NO_DEBUG
static const bool qsg_leak_check = !qEnvironmentVariableIsEmpty("QML_LEAK_CHECK");
#endif

QT_BEGIN_NAMESPACE

bool operator==(const QSGSamplerDescription &a, const QSGSamplerDescription &b) Q_DECL_NOTHROW
{
    return a.filtering == b.filtering
            && a.mipmapFiltering == b.mipmapFiltering
            && a.horizontalWrap == b.horizontalWrap
            && a.verticalWrap == b.verticalWrap
            && a.anisotropylevel == b.anisotropylevel;
}

bool operator!=(const QSGSamplerDescription &a, const QSGSamplerDescription &b) Q_DECL_NOTHROW
{
    return !(a == b);
}

uint qHash(const QSGSamplerDescription &s, uint seed) Q_DECL_NOTHROW
{
    const int f = s.filtering;
    const int m = s.mipmapFiltering;
    const int w = s.horizontalWrap;
    const int a = s.anisotropylevel;
    return (((f & 7) << 24) | ((m & 7) << 16) | ((w & 7) << 8) | (a & 7)) ^ seed;
}

QSGSamplerDescription QSGSamplerDescription::fromTexture(QSGTexture *t)
{
    QSGSamplerDescription s;
    s.filtering = t->filtering();
    s.mipmapFiltering = t->mipmapFiltering();
    s.horizontalWrap = t->horizontalWrapMode();
    s.verticalWrap = t->verticalWrapMode();
    s.anisotropylevel = t->anisotropyLevel();
    return s;
}

#if QT_CONFIG(opengl)
#ifndef QT_NO_DEBUG
inline static bool isPowerOfTwo(int x)
{
    // Assumption: x >= 1
    return x == (x & -x);
}
#endif
#endif

QSGTexturePrivate::QSGTexturePrivate()
    : wrapChanged(false)
    , filteringChanged(false)
    , anisotropyChanged(false)
    , horizontalWrap(QSGTexture::ClampToEdge)
    , verticalWrap(QSGTexture::ClampToEdge)
    , mipmapMode(QSGTexture::None)
    , filterMode(QSGTexture::Nearest)
    , anisotropyLevel(QSGTexture::AnisotropyNone)
{
}

#ifndef QT_NO_DEBUG

static int qt_debug_texture_count = 0;

#if (defined(Q_OS_LINUX) || defined (Q_OS_MAC)) && !defined(Q_OS_ANDROID)
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

#if defined(CAN_BACKTRACE_EXECINFO)
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
#if defined(CAN_BACKTRACE_EXECINFO)
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
#if defined(CAN_BACKTRACE_EXECINFO)
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
        qDebug("Texture destroyed after qt_debug_print_texture_count() was called.");
}

#endif // QT_NO_DEBUG

/*!
    \class QSGTexture

    \inmodule QtQuick

    \brief The QSGTexture class is a baseclass for textures used in
    the scene graph.


    Users can freely implement their own texture classes to support
    arbitrary input textures, such as YUV video frames or 8 bit alpha
    masks. The scene graph backend provides a default implementation
    of normal color textures. As the implementation of these may be
    hardware specific, they are constructed via the factory
    function QQuickWindow::createTextureFromImage().

    The texture is a wrapper around an OpenGL texture, which texture
    id is given by textureId() and which size in pixels is given by
    textureSize(). hasAlphaChannel() reports if the texture contains
    opacity values and hasMipmaps() reports if the texture contains
    mipmap levels.

    To use a texture, call the bind() function. The texture parameters
    specifying how the texture is bound, can be specified with
    setMipmapFiltering(), setFiltering(), setHorizontalWrapMode() and
    setVerticalWrapMode(). The texture will internally try to store
    these values to minimize the OpenGL state changes when the texture
    is bound.

    \section1 Texture Atlasses

    Some scene graph backends use texture atlasses, grouping multiple
    small textures into one large texture. If this is the case, the
    function isAtlasTexture() will return true. Atlasses are used to
    aid the rendering algorithm to do better sorting which increases
    performance. The location of the texture inside the atlas is
    given with the normalizedTextureSubRect() function.

    If the texture is used in such a way that atlas is not preferable,
    the function removedFromAtlas() can be used to extract a
    non-atlassed copy.

    \note All classes with QSG prefix should be used solely on the scene graph's
    rendering thread. See \l {Scene Graph and Rendering} for more information.

    \sa {Scene Graph - Rendering FBOs}, {Scene Graph - Rendering FBOs in a thread}
 */

/*!
    \enum QSGTexture::WrapMode

    Specifies how the texture should treat texture coordinates.

    \value Repeat Only the fractional part of the texture coordinate is
    used, causing values above 1 and below 0 to repeat.

    \value ClampToEdge Values above 1 are clamped to 1 and values
    below 0 are clamped to 0.

    \value MirroredRepeat When the texture coordinate is even, only the
    fractional part is used. When odd, the texture coordinate is set to
    \c{1 - fractional part}. This value has been introduced in Qt 5.10.
 */

/*!
    \enum QSGTexture::Filtering

    Specifies how sampling of texels should filter when texture
    coordinates are not pixel aligned.

    \value None No filtering should occur. This value is only used
    together with setMipmapFiltering().

    \value Nearest Sampling returns the nearest texel.

    \value Linear Sampling returns a linear interpolation of the
    neighboring texels.
*/

/*!
    \enum QSGTexture::AnisotropyLevel

    Specifies the anisotropic filtering level to be used when
    the texture is not screen aligned.

    \value AnisotropyNone No anisotropic filtering.

    \value Anisotropy2x 2x anisotropic filtering.

    \value Anisotropy4x 4x anisotropic filtering.

    \value Anisotropy8x 8x anisotropic filtering.

    \value Anisotropy16x 16x anisotropic filtering.

    \since 5.9
*/

/*!
    \class QSGTexture::NativeTexture
    \brief Contains information about the underlying native resources of a texture.
    \since 5.15
 */

/*!
    \variable QSGTexture::NativeTexture::object
    \brief a pointer to the native object handle.

    With OpenGL, the native handle is a GLuint value, so \c object is then a
    pointer to a GLuint. With Vulkan, the native handle is a VkImage, so \c
    object is a pointer to a VkImage. With Direct3D 11 and Metal \c
    object is a pointer to a ID3D11Texture2D or MTLTexture pointer, respectively.

    \note Pay attention to the fact that \a object is always a pointer
    to the native texture handle type, even if the native type itself is a
    pointer.
 */

/*!
    \variable QSGTexture::NativeTexture::layout
    \brief Specifies the current image layout for APIs like Vulkan.

    For Vulkan, \c layout contains a \c VkImageLayout value.
 */


#ifndef QT_NO_DEBUG
Q_QUICK_PRIVATE_EXPORT void qsg_set_material_failure();
#endif

#ifndef QT_NO_DEBUG
Q_GLOBAL_STATIC(QSet<QSGTexture *>, qsg_valid_texture_set)
Q_GLOBAL_STATIC(QMutex, qsg_valid_texture_mutex)

bool qsg_safeguard_texture(QSGTexture *texture)
{
#if QT_CONFIG(opengl)
    QMutexLocker locker(qsg_valid_texture_mutex());
    if (!qsg_valid_texture_set()->contains(texture)) {
        qWarning() << "Invalid texture accessed:" << (void *) texture;
        qsg_set_material_failure();
        QOpenGLContext::currentContext()->functions()->glBindTexture(GL_TEXTURE_2D, 0);
        return false;
    }
#else
    Q_UNUSED(texture)
#endif
    return true;
}
#endif

/*!
    Constructs the QSGTexture base class.
 */
QSGTexture::QSGTexture()
    : QObject(*(new QSGTexturePrivate))
{
#ifndef QT_NO_DEBUG
    if (qsg_leak_check)
        qt_debug_add_texture(this);

    QMutexLocker locker(qsg_valid_texture_mutex());
    qsg_valid_texture_set()->insert(this);
#endif
}

/*!
    \internal
 */
QSGTexture::QSGTexture(QSGTexturePrivate &dd)
    : QObject(dd)
{
#ifndef QT_NO_DEBUG
    if (qsg_leak_check)
        qt_debug_add_texture(this);

    QMutexLocker locker(qsg_valid_texture_mutex());
    qsg_valid_texture_set()->insert(this);
#endif
}

/*!
    Destroys the QSGTexture.
 */
QSGTexture::~QSGTexture()
{
#ifndef QT_NO_DEBUG
    if (qsg_leak_check)
        qt_debug_remove_texture(this);

    QMutexLocker locker(qsg_valid_texture_mutex());
    qsg_valid_texture_set()->remove(this);
#endif
}

/*!
    \fn void QSGTexture::bind()

    Call this function to bind this texture to the current texture
    target.

    Binding a texture may also include uploading the texture data from
    a previously set QImage.

    \warning This function should only be called when running with the
    direct OpenGL rendering path.

    \warning This function can only be called from the rendering thread.
 */

/*!
    \fn QRectF QSGTexture::convertToNormalizedSourceRect(const QRectF &rect) const

    Returns \a rect converted to normalized coordinates.

    \sa normalizedTextureSubRect()
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
    return nullptr;
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
    Returns a key suitable for comparing textures. Typically used in
    QSGMaterial::compare() implementations.

    Just comparing QSGTexture pointers is not always sufficient because two
    QSGTexture instances that refer to the same native texture object
    underneath should also be considered equal. Hence this function.

    \note Unlike textureId(), implementations of this function are not expected
    to and should not create any graphics resources (so texture objects) in
    case there is none yet.

    A QSGTexture that does not have a native texture object underneath is
    typically not equal to any other QSGTexture. There are exceptions to this,
    in particular when atlasing is used (where multiple textures share the same
    atlas texture under the hood), that is then up to the subclass
    implementations to deal with as appropriate.

    \warning This function can only be called from the rendering thread.

    \since 5.14
 */
int QSGTexture::comparisonKey() const
{
    Q_D(const QSGTexture);
    return d->comparisonKey();
}

/*!
    \fn QSize QSGTexture::textureSize() const

    Returns the size of the texture.
 */

/*!
    Returns the rectangle inside textureSize() that this texture
    represents in normalized coordinates.

    The default implementation returns a rect at position (0, 0) with
    width and height of 1.
 */
QRectF QSGTexture::normalizedTextureSubRect() const
{
    return QRectF(0, 0, 1, 1);
}

/*!
    \fn bool QSGTexture::hasAlphaChannel() const

    Returns true if the texture data contains an alpha channel.
 */

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

/*!
    Returns the sampling mode to be used for this texture.
 */
QSGTexture::Filtering QSGTexture::filtering() const
{
    return (QSGTexture::Filtering) d_func()->filterMode;
}

/*!
    Sets the level of anisotropic filtering to be used for the upcoming bind() call to \a level.
    The default value is QSGTexture::AnisotropyNone, which means no anisotropic filtering is enabled.

    \since 5.9
 */
void QSGTexture::setAnisotropyLevel(AnisotropyLevel level)
{
    Q_D(QSGTexture);
    if (d->anisotropyLevel != (uint) level) {
        d->anisotropyLevel = level;
        d->anisotropyChanged = true;
    }
}

/*!
    Returns the anisotropy level in use for filtering this texture.

    \since 5.9
 */
QSGTexture::AnisotropyLevel QSGTexture::anisotropyLevel() const
{
    return (QSGTexture::AnisotropyLevel) d_func()->anisotropyLevel;
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

/*!
    Returns the horizontal wrap mode to be used for this texture.
 */
QSGTexture::WrapMode QSGTexture::horizontalWrapMode() const
{
    return (QSGTexture::WrapMode) d_func()->horizontalWrap;
}



/*!
    Sets the vertical wrap mode to be used for the upcoming bind() call to \a vwrap
 */
void QSGTexture::setVerticalWrapMode(WrapMode vwrap)
{
    Q_D(QSGTexture);
    if ((uint) vwrap != d->verticalWrap) {
        d->verticalWrap = vwrap;
        d->wrapChanged = true;
    }
}

/*!
    Returns the vertical wrap mode to be used for this texture.
 */
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
void QSGTexture::updateBindOptions(bool force) // legacy (GL-only)
{
#if QT_CONFIG(opengl)
    Q_D(QSGTexture);
    QOpenGLFunctions *funcs = QOpenGLContext::currentContext()->functions();
    force |= isAtlasTexture();

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
        funcs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
        funcs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
        d->filteringChanged = false;
    }

    if (force || d->anisotropyChanged) {
        d->anisotropyChanged = false;
        if (QOpenGLContext::currentContext()->hasExtension(QByteArrayLiteral("GL_EXT_texture_filter_anisotropic")))
            funcs->glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, float(1 << (d->anisotropyLevel)));
    }

    if (force || d->wrapChanged) {
#ifndef QT_NO_DEBUG
        if (d->horizontalWrap == Repeat || d->verticalWrap == Repeat
            || d->horizontalWrap == MirroredRepeat || d->verticalWrap == MirroredRepeat)
        {
            bool npotSupported = QOpenGLFunctions(QOpenGLContext::currentContext()).hasOpenGLFeature(QOpenGLFunctions::NPOTTextures);
            QSize size = textureSize();
            bool isNpot = !isPowerOfTwo(size.width()) || !isPowerOfTwo(size.height());
            if (!npotSupported && isNpot)
                qWarning("Scene Graph: This system does not support the REPEAT wrap mode for non-power-of-two textures.");
        }
#endif
        GLenum wrapS = GL_CLAMP_TO_EDGE;
        if (d->horizontalWrap == Repeat)
            wrapS = GL_REPEAT;
        else if (d->horizontalWrap == MirroredRepeat)
            wrapS = GL_MIRRORED_REPEAT;
        GLenum wrapT = GL_CLAMP_TO_EDGE;
        if (d->verticalWrap == Repeat)
            wrapT = GL_REPEAT;
        else if (d->verticalWrap == MirroredRepeat)
            wrapT = GL_MIRRORED_REPEAT;
        funcs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapS);
        funcs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapT);
        d->wrapChanged = false;
    }
#else
    Q_UNUSED(force)
#endif
}

/*!
    Call this function to enqueue image upload operations to \a
    resourceUpdates, in case there are any pending ones. When there is no new
    data (for example, because there was no setImage() since the last call to
    this function), the function does nothing.

    Materials involving \a rhi textures are expected to call this function from
    their updateSampledImage() implementation, typically without any conditions.

    \note This function is only used when running the graphics API independent
    rendering path of the scene graph.

    \warning This function can only be called from the rendering thread.

    \since 5.14
 */
void QSGTexture::updateRhiTexture(QRhi *rhi, QRhiResourceUpdateBatch *resourceUpdates)
{
    Q_D(QSGTexture);
    d->updateRhiTexture(rhi, resourceUpdates);
}

/*!
    \return the platform-specific texture data for this texture.

    \note This is only available when running the graphics API independent
    rendering path of the scene graph. Use textureId() otherwise.

    Returns an empty result (\c object is null) if there is no available
    underlying native texture.

    \since 5.15
    \sa QQuickWindow::createTextureFromNativeObject()
 */
QSGTexture::NativeTexture QSGTexture::nativeTexture() const
{
    Q_D(const QSGTexture);
    if (auto *tex = d->rhiTexture()) {
        auto nativeTexture = tex->nativeTexture();
        return {nativeTexture.object, nativeTexture.layout};
    }
    return {};
}

/*!
    \internal
 */
void QSGTexture::setWorkResourceUpdateBatch(QRhiResourceUpdateBatch *resourceUpdates)
{
    Q_D(QSGTexture);
    d->workResourceUpdateBatch = resourceUpdates;
}

bool QSGTexturePrivate::hasDirtySamplerOptions() const
{
    return wrapChanged || filteringChanged || anisotropyChanged;
}

void QSGTexturePrivate::resetDirtySamplerOptions()
{
    wrapChanged = filteringChanged = anisotropyChanged = false;
}

int QSGTexturePrivate::comparisonKey() const
{
    // Must be overridden in subclasses but we cannot make this pure virtual
    // before Qt 6 because the simple QSGTexture ctor must be kept working.
    Q_Q(const QSGTexture);
    return q->textureId(); // this is semantically wrong but at least compatible with existing, non-RHI-aware subclasses
}

/*!
    \internal

    \return the QRhiTexture for this QSGTexture or null if there is none.

    Unlike textureId(), this function is not expected to create a new
    QRhiTexture in case there is none. Just return null in that case. The
    expectation towards the renderer is that a null texture leads to using a
    transparent, dummy texture instead.

    \note This function is only used when running the graphics API independent
    rendering path of the scene graph.

    \warning This function can only be called from the rendering thread.

    \since 5.14
 */
QRhiTexture *QSGTexturePrivate::rhiTexture() const
{
    return nullptr;
}

void QSGTexturePrivate::updateRhiTexture(QRhi *rhi, QRhiResourceUpdateBatch *resourceUpdates)
{
    Q_UNUSED(rhi);
    Q_UNUSED(resourceUpdates);
}

/*!
    \class QSGDynamicTexture
    \brief The QSGDynamicTexture class serves as a baseclass for dynamically changing textures,
    such as content that is rendered to FBO's.
    \inmodule QtQuick

    To update the content of the texture, call updateTexture() explicitly. Simply calling bind()
    will not update the texture.

    \note All classes with QSG prefix should be used solely on the scene graph's
    rendering thread. See \l {Scene Graph and Rendering} for more information.
 */


/*!
    \fn bool QSGDynamicTexture::updateTexture()

    Call this function to explicitly update the dynamic texture.

    The function returns true if the texture was changed as a resul of the update; otherwise
    returns false.

    \note This function is typically called from QQuickItem::updatePaintNode()
    or QSGNode::preprocess(), meaning during the \c{synchronization} or the
    \c{node preprocessing} phases of the scenegraph. Calling it at other times
    is discouraged and can lead to unexpected behavior.
 */

/*!
    \internal
 */
QSGDynamicTexture::QSGDynamicTexture(QSGTexturePrivate &dd)
    : QSGTexture(dd)
{
}

QT_END_NAMESPACE

#include "moc_qsgtexture.cpp"
