// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsgtexture_p.h"
#include "qsgtexture_platform.h"
#include <private/qqmlglobal_p.h>
#include <private/qsgmaterialshader_p.h>
#include <private/qsgrenderer_p.h>
#include <private/qquickitem_p.h> // qquickwindow_p.h cannot be included on its own due to template nonsense
#include <private/qquickwindow_p.h>
#include <QtCore/private/qnativeinterface_p.h>
#include <rhi/qrhi.h>

#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID) && defined(__GLIBC__)
#define CAN_BACKTRACE_EXECINFO
#endif

#if defined(Q_OS_APPLE)
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
Q_GLOBAL_STATIC(QSet<QSGTexture *>, qsg_valid_texture_set)
Q_GLOBAL_STATIC(QMutex, qsg_valid_texture_mutex)
#endif

QT_BEGIN_NAMESPACE

bool operator==(const QSGSamplerDescription &a, const QSGSamplerDescription &b) noexcept
{
    return a.filtering == b.filtering
            && a.mipmapFiltering == b.mipmapFiltering
            && a.horizontalWrap == b.horizontalWrap
            && a.verticalWrap == b.verticalWrap
            && a.anisotropylevel == b.anisotropylevel;
}

bool operator!=(const QSGSamplerDescription &a, const QSGSamplerDescription &b) noexcept
{
    return !(a == b);
}

size_t qHash(const QSGSamplerDescription &s, size_t seed) noexcept
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

QSGTexturePrivate::QSGTexturePrivate(QSGTexture *t)
    : wrapChanged(false)
    , filteringChanged(false)
    , anisotropyChanged(false)
    , horizontalWrap(QSGTexture::ClampToEdge)
    , verticalWrap(QSGTexture::ClampToEdge)
    , mipmapMode(QSGTexture::None)
    , filterMode(QSGTexture::Nearest)
    , anisotropyLevel(QSGTexture::AnisotropyNone)
#if QT_CONFIG(opengl)
    , m_openglTextureAccessor(t)
#endif
#ifdef Q_OS_WIN
    , m_d3d11TextureAccessor(t)
    , m_d3d12TextureAccessor(t)
#endif
#if QT_CONFIG(metal)
    , m_metalTextureAccessor(t)
#endif
#if QT_CONFIG(vulkan)
    , m_vulkanTextureAccessor(t)
#endif
{
#if !QT_CONFIG(opengl)
    Q_UNUSED(t);
#endif
}

#ifndef QT_NO_DEBUG

static int qt_debug_texture_count = 0;

#if (defined(Q_OS_LINUX) || defined (Q_OS_APPLE)) && !defined(Q_OS_ANDROID)
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
    qCDebug(lcQsgLeak, "Number of leaked textures: %i", qt_debug_texture_count);
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
    Q_UNUSED(texture);
#endif

    --qt_debug_texture_count;

    if (qt_debug_texture_count < 0)
        qDebug("Texture destroyed after qt_debug_print_texture_count() was called.");
}

#endif // QT_NO_DEBUG

/*!
    \class QSGTexture

    \inmodule QtQuick

    \brief The QSGTexture class is the base class for textures used in
    the scene graph.

    Users can freely implement their own texture classes to support arbitrary
    input textures, such as YUV video frames or 8 bit alpha masks. The scene
    graph provides a default implementation for RGBA textures.The default
    implementation is not instantiated directly, rather they are constructed
    via factory functions, such as QQuickWindow::createTextureFromImage().

    With the default implementation, each QSGTexture is backed by a
    QRhiTexture, which in turn contains a native texture object, such as an
    OpenGL texture or a Vulkan image.

    The size in pixels is given by textureSize(). hasAlphaChannel() reports if
    the texture contains opacity values and hasMipmaps() reports if the texture
    contains mipmap levels.

    \l{QSGMaterial}{Materials} that work with textures reimplement
    \l{QSGMaterialShader::updateSampledImage()}{updateSampledImage()} to
    provide logic that decides which QSGTexture's underlying native texture
    should be exposed at a given shader resource binding point.

    QSGTexture does not separate image (texture) and sampler objects. The
    parameters for filtering and wrapping can be specified with
    setMipmapFiltering(), setFiltering(), setHorizontalWrapMode() and
    setVerticalWrapMode(). The scene graph and Qt's graphics abstraction takes
    care of creating separate sampler objects, when applicable.

    \section1 Texture Atlases

    Some scene graph backends use texture atlasses, grouping multiple small
    textures into one large texture. If this is the case, the function
    isAtlasTexture() will return true. Atlases are used to aid the rendering
    algorithm to do better sorting which increases performance. Atlases are
    also essential for batching (merging together geometry to reduce the number
    of draw calls), because two instances of the same material using two
    different QSGTextures are not batchable, whereas if both QSGTextures refer
    to the same atlas, batching can happen, assuming the materials are
    otherwise compatible.

    The location of the texture inside the atlas is given with the
    normalizedTextureSubRect() function.

    If the texture is used in such a way that atlas is not preferable, the
    function removedFromAtlas() can be used to extract a non-atlased copy.

    \note All classes with QSG prefix should be used solely on the scene graph's
    rendering thread. See \l {Scene Graph and Rendering} for more information.
 */

/*!
    \enum QSGTexture::WrapMode

    Specifies how the sampler should treat texture coordinates.

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
    Constructs the QSGTexture base class.
 */
QSGTexture::QSGTexture()
    : QObject(*(new QSGTexturePrivate(this)))
{
#ifndef QT_NO_DEBUG
    if (lcQsgLeak().isDebugEnabled())
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
    if (lcQsgLeak().isDebugEnabled())
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
    if (lcQsgLeak().isDebugEnabled())
        qt_debug_remove_texture(this);

    QMutexLocker locker(qsg_valid_texture_mutex());
    qsg_valid_texture_set()->remove(this);
#endif
}

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

    \a resourceUpdates is an optional resource update batch, on which texture
    operations, if any, are enqueued. Materials can retrieve an instance from
    QSGMaterialShader::RenderState. When null, the removedFromAtlas()
    implementation creates its own batch and submit it right away. However,
    when a valid instance is specified, this function will not submit the
    update batch.

    \warning This function can only be called from the rendering thread.
 */

QSGTexture *QSGTexture::removedFromAtlas(QRhiResourceUpdateBatch *resourceUpdates) const
{
    Q_UNUSED(resourceUpdates);
    Q_ASSERT_X(!isAtlasTexture(), "QSGTexture::removedFromAtlas()", "Called on a non-atlas texture");
    return nullptr;
}

/*!
    Returns whether this texture is part of an atlas or not.

    The default implementation returns false.
 */
bool QSGTexture::isAtlasTexture() const
{
    return false;
}

/*!
    \fn qint64 QSGTexture::comparisonKey() const

    Returns a key suitable for comparing textures. Typically used in
    QSGMaterial::compare() implementations.

    Just comparing QSGTexture pointers is not always sufficient because two
    QSGTexture instances that refer to the same native texture object
    underneath should also be considered equal. Hence the need for this function.

    Implementations of this function are not expected to, and should not create
    any graphics resources (native texture objects) in case there are none yet.

    A QSGTexture that does not have a native texture object underneath is
    typically \b not equal to any other QSGTexture, so the return value has to
    be crafted accordingly. There are exceptions to this, in particular when
    atlasing is used (where multiple textures share the same atlas texture
    under the hood), that is then up to the subclass implementations to deal
    with as appropriate.

    \warning This function can only be called from the rendering thread.

    \since 5.14
 */

/*!
    \fn QSize QSGTexture::textureSize() const

    Returns the size of the texture in pixels.
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
    Sets the mipmap sampling mode to \a filter.

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
    Sets the sampling mode to \a filter.
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
    Sets the level of anisotropic filtering to \a level. The default value is
    QSGTexture::AnisotropyNone, which means no anisotropic filtering is
    enabled.

    \note The request may be ignored depending on the graphics API in use.
    There is no guarantee anisotropic filtering is supported at run time.

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
    Sets the horizontal wrap mode to \a hwrap
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
    Sets the vertical wrap mode to \a vwrap
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
    \return the QRhiTexture for this QSGTexture or null if there is none (either
    because a valid texture has not been created internally yet, or because the
    concept is not applicable to the scenegraph backend in use).

    This function is not expected to create a new QRhiTexture in case there is
    none. It should return null in that case. The expectation towards the
    renderer is that a null texture leads to using a transparent, dummy texture
    instead.

    \warning This function can only be called from the rendering thread.

    \since 6.0
 */
QRhiTexture *QSGTexture::rhiTexture() const
{
    return nullptr;
}

/*!
    Call this function to enqueue image upload operations to \a
    resourceUpdates, in case there are any pending ones. When there is no new
    data (for example, because there was no setImage() since the last call to
    this function), the function does nothing.

    Materials involving \a rhi textures are expected to call this function from
    their \l{QSGMaterialShader::updateSampledImage()}{updateSampledImage()}
    implementation, typically without any conditions, passing \c{state.rhi()}
    and \c{state.resourceUpdateBatch()} from the QSGMaterialShader::RenderState.

    \warning This function can only be called from the rendering thread.

    \since 6.0
 */
void QSGTexture::commitTextureOperations(QRhi *rhi, QRhiResourceUpdateBatch *resourceUpdates)
{
    Q_UNUSED(rhi);
    Q_UNUSED(resourceUpdates);
}

bool QSGTexturePrivate::hasDirtySamplerOptions() const
{
    return wrapChanged || filteringChanged || anisotropyChanged;
}

void QSGTexturePrivate::resetDirtySamplerOptions()
{
    wrapChanged = filteringChanged = anisotropyChanged = false;
}

/*!
    \class QSGDynamicTexture
    \brief The QSGDynamicTexture class serves as a baseclass for dynamically changing textures,
    such as content that is rendered to FBO's.
    \inmodule QtQuick

    To update the content of the texture, call updateTexture() explicitly.

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

/*!
    \internal
 */
QSGDynamicTexture::~QSGDynamicTexture()
    = default;

/*!
    \fn template <typename QNativeInterface> NativeInterface *QSGTexture::nativeInterface() const

    Returns a native interface of the given type for the texture.

    This function provides access to platform specific functionality of
    QSGTexture, as declared in the QNativeInterface namespace:

    \annotatedlist native-interfaces-qsgtexture

    This allows accessing the underlying native texture object, such as, the \c GLuint
    texture ID with OpenGL, or the \c VkImage handle with Vulkan.

    If the requested interface is not available a \nullptr is returned.
 */


#if QT_CONFIG(opengl) || defined(Q_QDOC)
namespace QNativeInterface {
/*!
    \class QNativeInterface::QSGOpenGLTexture
    \inmodule QtQuick
    \ingroup native-interfaces
    \ingroup native-interfaces-qsgtexture
    \inheaderfile QSGTexture
    \brief Provides access to and enables adopting OpenGL texture objects.
    \since 6.0
*/

/*!
    \fn VkImage QNativeInterface::QSGOpenGLTexture::nativeTexture() const
    \return the OpenGL texture ID.
 */

QT_DEFINE_NATIVE_INTERFACE(QSGOpenGLTexture);

/*!
    Creates a new QSGTexture wrapping an existing OpenGL texture object for
    \a window.

    The native object specified in \a textureId is wrapped, but not owned, by
    the resulting QSGTexture. The caller of the function is responsible for
    deleting the returned QSGTexture, but that will not destroy the underlying
    native object.

    This function is currently suitable for 2D RGBA textures only.

    \warning This function will return null if the scenegraph has not yet been
    initialized.

    Use \a options to customize the texture attributes. Only the
    TextureHasAlphaChannel and TextureHasMipmaps are taken into account here.

    \a size specifies the size in pixels.

    \note This function must be called on the scenegraph rendering thread.

    \sa QQuickWindow::sceneGraphInitialized(), QSGTexture,
    {Scene Graph - Metal Texture Import}, {Scene Graph - Vulkan Texture Import}

    \since 6.0
 */
QSGTexture *QSGOpenGLTexture::fromNative(GLuint textureId,
                                         QQuickWindow *window,
                                         const QSize &size,
                                         QQuickWindow::CreateTextureOptions options)
{
    return QQuickWindowPrivate::get(window)->createTextureFromNativeTexture(quint64(textureId), 0, size, options);
}

/*!
    Creates a new QSGTexture wrapping an existing OpenGL ES texture object for
    \a window.

    The native object specified in \a textureId is wrapped, but not owned, by
    the resulting QSGTexture. The caller of the function is responsible for
    deleting the returned QSGTexture, but that will not destroy the underlying
    native object.

    This function is suitable only for textures that are meant to be
    used with the \c{GL_TEXTURE_EXTERNAL_OES} target: usually textures
    to which another device (such as a camera) writes data.

    \warning This function will return null if the scenegraph has not yet been
    initialized.

    Use \a options to customize the texture attributes. Only the
    TextureHasAlphaChannel and TextureHasMipmaps are taken into account here.

    \a size specifies the size in pixels.

    \note This function must be called on the scenegraph rendering thread.

    \since 6.1

    \sa fromNative()
 */
QSGTexture *QSGOpenGLTexture::fromNativeExternalOES(GLuint textureId,
                                                    QQuickWindow *window,
                                                    const QSize &size,
                                                    QQuickWindow::CreateTextureOptions options)
{
    return QQuickWindowPrivate::get(window)->createTextureFromNativeTexture(quint64(textureId),
                                                                            0,
                                                                            size,
                                                                            options,
                                                                            QQuickWindowPrivate::NativeTextureIsExternalOES);
}
} // QNativeInterface

GLuint QSGTexturePlatformOpenGL::nativeTexture() const
{
    if (auto *tex = m_texture->rhiTexture())
        return GLuint(tex->nativeTexture().object);
    return 0;
}
#endif // opengl

#if defined(Q_OS_WIN) || defined(Q_QDOC)
namespace QNativeInterface {
/*!
    \class QNativeInterface::QSGD3D11Texture
    \inmodule QtQuick
    \ingroup native-interfaces
    \ingroup native-interfaces-qsgtexture
    \inheaderfile QSGTexture
    \brief Provides access to and enables adopting Direct3D 11 texture objects.
    \since 6.0
*/

/*!
    \fn void *QNativeInterface::QSGD3D11Texture::nativeTexture() const
    \return the ID3D11Texture2D object.
 */

QT_DEFINE_NATIVE_INTERFACE(QSGD3D11Texture);

/*!
    Creates a new QSGTexture wrapping an existing Direct 3D 11 \a texture object
    for \a window.

    The native object is wrapped, but not owned, by the resulting QSGTexture.
    The caller of the function is responsible for deleting the returned
    QSGTexture, but that will not destroy the underlying native object.

    This function is currently suitable for 2D RGBA textures only.

    \warning This function will return null if the scene graph has not yet been
    initialized.

    Use \a options to customize the texture attributes. Only the
    TextureHasAlphaChannel and TextureHasMipmaps are taken into account here.

    \a size specifies the size in pixels.

    \note This function must be called on the scene graph rendering thread.

    \sa QQuickWindow::sceneGraphInitialized(), QSGTexture,
    {Scene Graph - Metal Texture Import}, {Scene Graph - Vulkan Texture Import}

    \since 6.0
 */
QSGTexture *QSGD3D11Texture::fromNative(void *texture,
                                        QQuickWindow *window,
                                        const QSize &size,
                                        QQuickWindow::CreateTextureOptions options)
{
    return QQuickWindowPrivate::get(window)->createTextureFromNativeTexture(quint64(texture), 0, size, options);
}
} // QNativeInterface

void *QSGTexturePlatformD3D11::nativeTexture() const
{
    if (auto *tex = m_texture->rhiTexture())
        return reinterpret_cast<void *>(quintptr(tex->nativeTexture().object));
    return 0;
}

namespace QNativeInterface {
/*!
    \class QNativeInterface::QSGD3D12Texture
    \inmodule QtQuick
    \ingroup native-interfaces
    \ingroup native-interfaces-qsgtexture
    \inheaderfile QSGTexture
    \brief Provides access to and enables adopting Direct3D 12 texture objects.
    \since 6.6
*/

/*!
    \fn void *QNativeInterface::QSGD3D12Texture::nativeTexture() const
    \return the ID3D12Texture object.
 */

QT_DEFINE_NATIVE_INTERFACE(QSGD3D12Texture);

/*!
    Creates a new QSGTexture wrapping an existing Direct 3D 12 \a texture object
    for \a window.

    The native object is wrapped, but not owned, by the resulting QSGTexture.
    The caller of the function is responsible for deleting the returned
    QSGTexture, but that will not destroy the underlying native object.

    This function is currently suitable for 2D RGBA textures only.

    \warning This function will return null if the scene graph has not yet been
    initialized.

    Use \a options to customize the texture attributes. Only the
    TextureHasAlphaChannel and TextureHasMipmaps are taken into account here.

    \a size specifies the size in pixels.

    \a resourceState must specify the
    \l{https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_resource_states}{current state}
    of the texture resource.

    \note This function must be called on the scene graph rendering thread.

    \sa QQuickWindow::sceneGraphInitialized(), QSGTexture,
    {Scene Graph - Metal Texture Import}, {Scene Graph - Vulkan Texture Import}

    \since 6.6
 */
QSGTexture *QSGD3D12Texture::fromNative(void *texture,
                                        int resourceState,
                                        QQuickWindow *window,
                                        const QSize &size,
                                        QQuickWindow::CreateTextureOptions options)
{
    return QQuickWindowPrivate::get(window)->createTextureFromNativeTexture(quint64(texture), resourceState, size, options);
}
} // QNativeInterface

int QSGTexturePlatformD3D12::nativeResourceState() const
{
    if (auto *tex = m_texture->rhiTexture())
        return tex->nativeTexture().layout;
    return 0;
}

void *QSGTexturePlatformD3D12::nativeTexture() const
{
    if (auto *tex = m_texture->rhiTexture())
        return reinterpret_cast<void *>(quintptr(tex->nativeTexture().object));
    return 0;
}

#endif // win

#if defined(__OBJC__) || defined(Q_QDOC)
namespace QNativeInterface {
/*!
    \class QNativeInterface::QSGMetalTexture
    \inmodule QtQuick
    \ingroup native-interfaces
    \ingroup native-interfaces-qsgtexture
    \inheaderfile QSGTexture
    \brief Provides access to and enables adopting Metal texture objects.
    \since 6.0
*/

/*!
    \fn id<MTLTexture> QNativeInterface::QSGMetalTexture::nativeTexture() const
    \return the Metal texture object.
 */

/*!
    \fn QSGTexture *QNativeInterface::QSGMetalTexture::fromNative(id<MTLTexture> texture, QQuickWindow *window, const QSize &size, QQuickWindow::CreateTextureOptions options)

    Creates a new QSGTexture wrapping an existing Metal \a texture object for
    \a window.

    The native object is wrapped, but not owned, by the resulting QSGTexture.
    The caller of the function is responsible for deleting the returned
    QSGTexture, but that will not destroy the underlying native object.

    This function is currently suitable for 2D RGBA textures only.

    \warning This function will return null if the scene graph has not yet been
    initialized.

    Use \a options to customize the texture attributes. Only the
    TextureHasAlphaChannel and TextureHasMipmaps are taken into account here.

    \a size specifies the size in pixels.

    \note This function must be called on the scene graph rendering thread.

    \sa QQuickWindow::sceneGraphInitialized(), QSGTexture,
    {Scene Graph - Metal Texture Import}, {Scene Graph - Vulkan Texture Import}

    \since 6.0
 */

} // QNativeInterface
#endif // OBJC

#if QT_CONFIG(vulkan) || defined(Q_QDOC)
namespace QNativeInterface {
/*!
    \class QNativeInterface::QSGVulkanTexture
    \inmodule QtQuick
    \ingroup native-interfaces
    \ingroup native-interfaces-qsgtexture
    \inheaderfile QSGTexture
    \brief Provides access to and enables adopting Vulkan image objects.
    \since 6.0
*/

/*!
    \fn VkImage QNativeInterface::QSGVulkanTexture::nativeImage() const
    \return the VkImage handle.
 */

/*!
    \fn VkImageLayout QNativeInterface::QSGVulkanTexture::nativeImageLayout() const
    \return the image layout.
 */

QT_DEFINE_NATIVE_INTERFACE(QSGVulkanTexture);

/*!
    Creates a new QSGTexture wrapping an existing Vulkan \a image object for
    \a window.

    The native object is wrapped, but not owned, by the resulting QSGTexture.
    The caller of the function is responsible for deleting the returned
    QSGTexture, but that will not destroy the underlying native object.

    This function is currently suitable for 2D RGBA textures only.

    \warning This function will return null if the scene graph has not yet been
    initialized.

    \a layout must specify the current layout of the image.

    Use \a options to customize the texture attributes. Only the
    TextureHasAlphaChannel and TextureHasMipmaps are taken into account here.

    \a size specifies the size in pixels.

    \note This function must be called on the scene graph rendering thread.

    \sa QQuickWindow::sceneGraphInitialized(), QSGTexture,
    {Scene Graph - Metal Texture Import}, {Scene Graph - Vulkan Texture Import}

    \since 6.0
 */
QSGTexture *QSGVulkanTexture::fromNative(VkImage image,
                                         VkImageLayout layout,
                                         QQuickWindow *window,
                                         const QSize &size,
                                         QQuickWindow::CreateTextureOptions options)
{
    return QQuickWindowPrivate::get(window)->createTextureFromNativeTexture(quint64(image), layout, size, options);
}
} // QNativeInterface

VkImage QSGTexturePlatformVulkan::nativeImage() const
{
    if (auto *tex = m_texture->rhiTexture())
        return VkImage(tex->nativeTexture().object);
    return VK_NULL_HANDLE;
}

VkImageLayout QSGTexturePlatformVulkan::nativeImageLayout() const
{
    if (auto *tex = m_texture->rhiTexture())
        return VkImageLayout(tex->nativeTexture().layout);
    return VK_IMAGE_LAYOUT_UNDEFINED;
}
#endif // vulkan

void *QSGTexture::resolveInterface(const char *name, int revision) const
{
    using namespace QNativeInterface;
    Q_UNUSED(name);
    Q_UNUSED(revision);

    Q_D(const QSGTexture);
    auto *dd = const_cast<QSGTexturePrivate *>(d);
    Q_UNUSED(dd);

#if QT_CONFIG(vulkan)
    QT_NATIVE_INTERFACE_RETURN_IF(QSGVulkanTexture, &dd->m_vulkanTextureAccessor);
#endif
#if QT_CONFIG(metal)
    QT_NATIVE_INTERFACE_RETURN_IF(QSGMetalTexture, &dd->m_metalTextureAccessor);
#endif
#if defined(Q_OS_WIN)
    QT_NATIVE_INTERFACE_RETURN_IF(QSGD3D11Texture, &dd->m_d3d11TextureAccessor);
    QT_NATIVE_INTERFACE_RETURN_IF(QSGD3D12Texture, &dd->m_d3d12TextureAccessor);
#endif
#if QT_CONFIG(opengl)
    QT_NATIVE_INTERFACE_RETURN_IF(QSGOpenGLTexture, &dd->m_openglTextureAccessor);
#endif

    return nullptr;
}

QT_END_NAMESPACE

#include "moc_qsgtexture.cpp"
