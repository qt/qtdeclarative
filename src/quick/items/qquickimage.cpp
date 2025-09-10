// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickimage_p.h"
#include "qquickimage_p_p.h"

#include <QtQuick/qsgtextureprovider.h>

#include <QtQuick/private/qsgcontext_p.h>
#include <private/qsgadaptationlayer_p.h>
#include <private/qnumeric_p.h>

#include <QtCore/qmath.h>
#include <QtGui/qpainter.h>
#include <QtCore/QRunnable>

QT_BEGIN_NAMESPACE

QQuickImageTextureProvider::QQuickImageTextureProvider()
    : m_texture(nullptr)
    , m_smooth(false)
{
}

void QQuickImageTextureProvider::updateTexture(QSGTexture *texture) {
    if (m_texture == texture)
        return;

    if (m_texture)
        disconnect(m_texture, &QSGTexture::destroyed, this, nullptr);

    m_texture = texture;

    if (m_texture)
        connect(m_texture, &QSGTexture::destroyed, this, [this]() { updateTexture(nullptr); });

    emit textureChanged();
}

QSGTexture *QQuickImageTextureProvider::texture() const {
    if (m_texture) {
        m_texture->setFiltering(m_smooth ? QSGTexture::Linear : QSGTexture::Nearest);
        m_texture->setMipmapFiltering(m_mipmap ? QSGTexture::Linear : QSGTexture::None);
        m_texture->setHorizontalWrapMode(QSGTexture::ClampToEdge);
        m_texture->setVerticalWrapMode(QSGTexture::ClampToEdge);
    }
    return m_texture;
}

QQuickImagePrivate::QQuickImagePrivate()
    : pixmapChanged(false)
    , mipmap(false)
{
}

/*!
    \qmltype Image
    \nativetype QQuickImage
    \inqmlmodule QtQuick
    \ingroup qtquick-visual
    \inherits Item
    \brief Displays an image.

    The Image type displays an image.

    The source of the image is specified as a URL using the \l source property.
    Images can be supplied in any of the standard image formats supported by Qt,
    including bitmap formats such as PNG and JPEG, and vector graphics formats
    such as SVG. If you need to display animated images, use \l AnimatedSprite
    or \l AnimatedImage.

    If the \l{Item::width}{width} and \l{Item::height}{height} properties are not
    specified, the Image automatically uses the size of the loaded image.
    By default, specifying the width and height of the item causes the image
    to be scaled to that size. This behavior can be changed by setting the
    \l fillMode property, allowing the image to be stretched and tiled instead.

    It is possible to provide \l {High Resolution Versions of Images}{"@nx" high DPI syntax}.

    \section1 Example Usage

    The following example shows the simplest usage of the Image type.

    \snippet qml/image.qml document

    \beginfloatleft
    \image declarative-qtlogo.png
    \endfloat

    \clearfloat

    \section1 Compressed Texture Files

    When supported by the implementation of the underlying graphics API at run
    time, images can also be supplied in compressed texture files. The content
    must be a simple RGB(A) format 2D texture. Supported compression schemes are
    only limited by the underlying driver and GPU. The following container file
    formats are supported:

    \list
    \li \c PKM (since Qt 5.10)
    \li \c KTX (since Qt 5.11)
    \li \c ASTC (since Qt 5.13)
    \endlist

    \note The intended vertical orientation of an image in a texture file is not generally well
    defined. Different texture compression tools have different defaults and options of when to
    perform vertical flipping of the input image. If an image from a texture file appears upside
    down, flipping may need to be toggled in the asset conditioning process. Alternatively, the
    Image element itself can be flipped by either applying a suitable transformation via the
    transform property or, more conveniently, by setting the mirrorVertically property:
    \badcode
    transform: [ Translate { y: -myImage.height }, Scale { yScale: -1 } ]
    \endcode
    or
    \badcode
    mirrorVertically: true
    \endcode

    \note Semi-transparent original images require alpha pre-multiplication
    prior to texture compression in order to be correctly displayed in Qt
    Quick. This can be done with the following ImageMagick command
    line:
    \badcode
    convert foo.png \( +clone -alpha Extract \) -channel RGB -compose Multiply -composite foo_pm.png
    \endcode

    Do not confuse container formats, such as, \c KTX, and the format of the
    actual texture data stored in the container file. For example, reading a
    \c KTX file is supported on all platforms, independently of what GPU driver is
    used at run time. However, this does not guarantee that the compressed
    texture format, used by the data in the file, is supported at run time. For
    example, if the KTX file contains compressed data with the format
    \c{ETC2 RGBA8}, and the 3D graphics API implementation used at run time does not
    support \c ETC2 compressed textures, the Image item will not display
    anything.

    \note Compressed texture format support is not under Qt's control, and it
    is up to the application or device developer to ensure the compressed
    texture data is provided in the appropriate format for the target
    environment(s).

    Do not assume that compressed format support is specific to a platform. It
    may also be specific to the driver and 3D API implementation in use on that
    particular platform. In practice, implementations of different 3D graphics
    APIs (e.g., Vulkan and OpenGL) on the same platform (e.g., Windows) from
    the same vendor for the same hardware may offer a different set of
    compressed texture formats.

    When targeting desktop environments (Windows, macOS, Linux) only, a general
    recommendation is to consider using the \c{DXTn}/\c{BCn} formats since
    these tend to have the widest support amongst the implementations of Direct
    3D, Vulkan, OpenGL, and Metal on these platforms. In contrast, when
    targeting mobile or embedded devices, the \c ETC2 or \c ASTC formats are
    likely to be a better choice since these are typically the formats
    supported by the OpenGL ES implementations on such hardware.

    An application that intends to run across desktop, mobile, and embedded
    hardware should plan and design its use of compressed textures carefully.
    It is highly likely that relying on a single format is not going to be
    sufficient, and therefore the application will likely need to branch based
    on the platform to use compressed textures in a format appropriate there,
    or perhaps to skip using compressed textures in some cases.

    \section1 Automatic Detection of File Extension

    If the \l source URL indicates a non-existing local file or resource, the
    Image element attempts to auto-detect the file extension. If an existing
    file can be found by appending any of the supported image file extensions
    to the \l source URL, then that file will be loaded.

    The file search attempts to look for compressed texture container file
    extensions first. If the search is unsuccessful, it attempts to search with
    the file extensions for the
    \l{QImageReader::supportedImageFormats()}{conventional image file
    types}. For example:

    \snippet qml/image-ext.qml ext

    This functionality facilitates deploying different image asset file types
    on different target platforms. This can be useful in order to tune
    application performance and adapt to different graphics hardware.

    This functionality was introduced in Qt 5.11.

    \section1 Performance

    By default, locally available images are loaded immediately, and the user interface
    is blocked until loading is complete. If a large image is to be loaded, it may be
    preferable to load the image in a low priority thread, by enabling the \l asynchronous
    property.

    If the image is obtained from a network rather than a local resource, it is
    automatically loaded asynchronously, and the \l progress and \l status properties
    are updated as appropriate.

    Images are cached and shared internally, so if several Image items have the same \l source,
    only one copy of the image will be loaded.

    \b Note: Images are often the greatest user of memory in QML user interfaces.  It is recommended
    that images which do not form part of the user interface have their
    size bounded via the \l sourceSize property. This is especially important for content
    that is loaded from external sources or provided by the user.

    \sa {Qt Quick Examples - Image Elements}, QQuickImageProvider, QImageReader::setAutoDetectImageFormat()
*/

QQuickImage::QQuickImage(QQuickItem *parent)
    : QQuickImageBase(*(new QQuickImagePrivate), parent)
{
}

QQuickImage::QQuickImage(QQuickImagePrivate &dd, QQuickItem *parent)
    : QQuickImageBase(dd, parent)
{
}

QQuickImage::~QQuickImage()
{
    Q_D(QQuickImage);
    if (d->provider) {
        // We're guaranteed to have a window() here because the provider would have
        // been released in releaseResources() if we were gone from a window.
        QQuickWindowQObjectCleanupJob::schedule(window(), d->provider);
    }
}

void QQuickImagePrivate::setImage(const QImage &image)
{
    Q_Q(QQuickImage);
    currentPix->setImage(image);
    q->pixmapChange();
    q->update();
}

void QQuickImagePrivate::setPixmap(const QQuickPixmap &pixmap)
{
    Q_Q(QQuickImage);
    currentPix->setPixmap(pixmap);
    q->pixmapChange();
    q->update();
}

/*!
    \qmlproperty enumeration QtQuick::Image::fillMode

    Set this property to define what happens when the source image has a different size
    than the item.

    \value Image.Stretch            the image is scaled to fit
    \value Image.PreserveAspectFit  the image is scaled uniformly to fit without cropping
    \value Image.PreserveAspectCrop the image is scaled uniformly to fill, cropping if necessary
    \value Image.Tile               the image is duplicated horizontally and vertically
    \value Image.TileVertically     the image is stretched horizontally and tiled vertically
    \value Image.TileHorizontally   the image is stretched vertically and tiled horizontally
    \value Image.Pad                the image is not transformed
    \br

    \table

    \row
    \li \image declarative-qtlogo-stretch.png
    \li Stretch (default)
    \qml
    Image {
        width: 130; height: 100
        source: "qtlogo.png"
    }
    \endqml

    \row
    \li \image declarative-qtlogo-preserveaspectfit.png
    \li PreserveAspectFit
    \qml
    Image {
        width: 130; height: 100
        fillMode: Image.PreserveAspectFit
        source: "qtlogo.png"
    }
    \endqml

    \row
    \li \image declarative-qtlogo-preserveaspectcrop.png
    \li PreserveAspectCrop
    \qml
    Image {
        width: 130; height: 100
        fillMode: Image.PreserveAspectCrop
        source: "qtlogo.png"
        clip: true
    }
    \endqml

    \row
    \li \image declarative-qtlogo-tile.png
    \li Tile
    \qml
    Image {
        width: 120; height: 120
        fillMode: Image.Tile
        horizontalAlignment: Image.AlignLeft
        verticalAlignment: Image.AlignTop
        source: "qtlogo.png"
    }
    \endqml

    \row
    \li \image declarative-qtlogo-tilevertically.png
    \li TileVertically
    \qml
    Image {
        width: 120; height: 120
        fillMode: Image.TileVertically
        verticalAlignment: Image.AlignTop
        source: "qtlogo.png"
    }
    \endqml

    \row
    \li \image declarative-qtlogo-tilehorizontally.png
    \li TileHorizontally
    \qml
    Image {
        width: 120; height: 120
        fillMode: Image.TileHorizontally
        verticalAlignment: Image.AlignLeft
        source: "qtlogo.png"
    }
    \endqml

    \endtable

    Note that \c clip is \c false by default which means that the item might
    paint outside its bounding rectangle even if the fillMode is set to \c PreserveAspectCrop.

    \sa {Qt Quick Examples - Image Elements}
*/
QQuickImage::FillMode QQuickImage::fillMode() const
{
    Q_D(const QQuickImage);
    return d->fillMode;
}

void QQuickImage::setFillMode(FillMode mode)
{
    Q_D(QQuickImage);
    if (d->fillMode == mode)
        return;
    d->fillMode = mode;
    if ((mode == PreserveAspectCrop) != d->providerOptions.preserveAspectRatioCrop()) {
        d->providerOptions.setPreserveAspectRatioCrop(mode == PreserveAspectCrop);
        if (isComponentComplete())
            load();
    } else if ((mode == PreserveAspectFit) != d->providerOptions.preserveAspectRatioFit()) {
        d->providerOptions.setPreserveAspectRatioFit(mode == PreserveAspectFit);
        if (isComponentComplete())
            load();
    }
    update();
    updatePaintedGeometry();
    emit fillModeChanged();
}

/*!
    \qmlproperty real QtQuick::Image::paintedWidth
    \qmlproperty real QtQuick::Image::paintedHeight
    \readonly

    These properties hold the size of the image that is actually painted.
    In most cases it is the same as \c width and \c height, but when using an
     \l {fillMode}{Image.PreserveAspectFit} or an \l {fillMode}{Image.PreserveAspectCrop}
    \c paintedWidth or \c paintedHeight can be smaller or larger than
    \c width and \c height of the Image item.
*/
qreal QQuickImage::paintedWidth() const
{
    Q_D(const QQuickImage);
    return d->paintedWidth;
}

qreal QQuickImage::paintedHeight() const
{
    Q_D(const QQuickImage);
    return d->paintedHeight;
}

/*!
    \qmlproperty enumeration QtQuick::Image::status
    \readonly

    This property holds the status of image loading.  It can be one of:

    \value Image.Null       No image has been set
    \value Image.Ready      The image has been loaded
    \value Image.Loading    The image is currently being loaded
    \value Image.Error      An error occurred while loading the image

    Use this status to provide an update or respond to the status change in some way.
    For example, you could:

    \list
    \li Trigger a state change:
    \qml
        State { name: 'loaded'; when: image.status == Image.Ready }
    \endqml

    \li Implement an \c onStatusChanged signal handler:
    \qml
        Image {
            id: image
            onStatusChanged: if (image.status == Image.Ready) console.log('Loaded')
        }
    \endqml

    \li Bind to the status value:
    \qml
        Text { text: image.status == Image.Ready ? 'Loaded' : 'Not loaded' }
    \endqml
    \endlist

    \sa progress
*/

/*!
    \qmlproperty real QtQuick::Image::progress
    \readonly

    This property holds the progress of image loading, from 0.0 (nothing loaded)
    to 1.0 (finished).

    \sa status
*/

/*!
    \qmlproperty bool QtQuick::Image::smooth

    This property holds whether the image is smoothly filtered when scaled or
    transformed.  Smooth filtering gives better visual quality, but it may be slower
    on some hardware.  If the image is displayed at its natural size, this property has
    no visual or performance effect.

    By default, this property is set to true.

    \sa mipmap
*/

/*!
    \qmlproperty size QtQuick::Image::sourceSize

    This property holds the scaled width and height of the full-frame image.

    Unlike the \l {Item::}{width} and \l {Item::}{height} properties, which scale
    the painting of the image, this property sets the maximum number of pixels
    stored for the loaded image so that large images do not use more
    memory than necessary. For example, this ensures the image in memory is no
    larger than 1024x1024 pixels, regardless of the Image's \l {Item::}{width} and
    \l {Item::}{height} values:

    \code
    Rectangle {
        width: ...
        height: ...

        Image {
           anchors.fill: parent
           source: "reallyBigImage.jpg"
           sourceSize.width: 1024
           sourceSize.height: 1024
        }
    }
    \endcode

    If the image's actual size is larger than the sourceSize, the image is scaled down.
    If only one dimension of the size is set to greater than 0, the
    other dimension is set in proportion to preserve the source image's aspect ratio.
    (The \l fillMode is independent of this.)

    If both the sourceSize.width and sourceSize.height are set, the image will be scaled
    down to fit within the specified size (unless PreserveAspectCrop or PreserveAspectFit
    are used, then it will be scaled to match the optimal size for cropping/fitting),
    maintaining the image's aspect ratio.  The actual
    size of the image after scaling is available via \l Item::implicitWidth and \l Item::implicitHeight.

    If the source is an intrinsically scalable image (eg. SVG), this property
    determines the size of the loaded image regardless of intrinsic size.
    Avoid changing this property dynamically; rendering an SVG is \e slow compared
    to an image.

    If the source is a non-scalable image (eg. JPEG), the loaded image will
    be no greater than this property specifies. For some formats (currently only JPEG),
    the whole image will never actually be loaded into memory.

    If the \l sourceClipRect property is also set, \c sourceSize determines the scale,
    but it will be clipped to the size of the clip rectangle.

    sourceSize can be cleared to the natural size of the image
    by setting sourceSize to \c undefined.

    \note \e {Changing this property dynamically causes the image source to be reloaded,
    potentially even from the network, if it is not in the disk cache.}

    \sa {Qt Quick Examples - Pointer Handlers}
*/

/*!
    \qmlproperty rect QtQuick::Image::sourceClipRect
    \since 5.15

    This property, if set, holds the rectangular region of the source image to
    be loaded.

    The \c sourceClipRect works together with the \l sourceSize property to
    conserve system resources when only a portion of an image needs to be
    loaded.

    \code
    Rectangle {
        width: ...
        height: ...

        Image {
           anchors.fill: parent
           source: "reallyBigImage.svg"
           sourceSize.width: 1024
           sourceSize.height: 1024
           sourceClipRect: Qt.rect(100, 100, 512, 512)
        }
    }
    \endcode

    In the above example, we conceptually scale the SVG graphic to 1024x1024
    first, and then cut out a region of interest that is 512x512 pixels from a
    location 100 pixels from the top and left edges. Thus \c sourceSize
    determines the scale, but the actual output image is 512x512 pixels.

    Some image formats are able to conserve CPU time by rendering only the
    specified region. Others will need to load the entire image first and then
    clip it to the specified region.

    This property can be cleared to reload the entire image by setting
    \c sourceClipRect to \c undefined.

    \note \e {Changing this property dynamically causes the image source to be reloaded,
    potentially even from the network, if it is not in the disk cache.}

    \note Sub-pixel clipping is not supported: the given rectangle will be
    passed to \l QImageReader::setScaledClipRect().
*/

/*!
    \qmlproperty url QtQuick::Image::source

    Image can handle any image format supported by Qt, loaded from any URL scheme supported by Qt.

    The URL may be absolute, or relative to the URL of the component.

    \sa QQuickImageProvider, {Compressed Texture Files}, {Automatic Detection of File Extension}
*/

/*!
    \qmlproperty bool QtQuick::Image::asynchronous

    Specifies that images on the local filesystem should be loaded
    asynchronously in a separate thread.  The default value is
    false, causing the user interface thread to block while the
    image is loaded.  Setting \a asynchronous to true is useful where
    maintaining a responsive user interface is more desirable
    than having images immediately visible.

    Note that this property is only valid for images read from the
    local filesystem.  Images loaded via a network resource (e.g. HTTP)
    are always loaded asynchronously.
*/

/*!
    \qmlproperty bool QtQuick::Image::cache

    Specifies whether the image should be cached. The default value is
    true. Setting \a cache to false is useful when dealing with large images,
    to make sure that they aren't cached at the expense of small 'ui element' images.
*/

/*!
    \qmlproperty bool QtQuick::Image::mirror

    This property holds whether the image should be horizontally inverted
    (effectively displaying a mirrored image).

    The default value is false.
*/

/*!
    \qmlproperty bool QtQuick::Image::mirrorVertically

    This property holds whether the image should be vertically inverted
    (effectively displaying a mirrored image).

    The default value is false.

    \since 6.2
*/

/*!
    \qmlproperty enumeration QtQuick::Image::horizontalAlignment
    \qmlproperty enumeration QtQuick::Image::verticalAlignment

    Sets the horizontal and vertical alignment of the image. By default, the image is center aligned.

    The valid values for \c horizontalAlignment are \c Image.AlignLeft, \c Image.AlignRight and \c Image.AlignHCenter.
    The valid values for \c verticalAlignment are \c Image.AlignTop, \c Image.AlignBottom
    and \c Image.AlignVCenter.
*/
void QQuickImage::updatePaintedGeometry()
{
    Q_D(QQuickImage);

    if (d->fillMode == PreserveAspectFit) {
        if (!d->currentPix->width() || !d->currentPix->height()) {
            setImplicitSize(0, 0);
            return;
        }
        const qreal pixWidth = d->currentPix->width() / d->devicePixelRatio;
        const qreal pixHeight = d->currentPix->height() / d->devicePixelRatio;
        const qreal w = widthValid() ? width() : pixWidth;
        const qreal widthScale = w / pixWidth;
        const qreal h = heightValid() ? height() : pixHeight;
        const qreal heightScale = h / pixHeight;
        if (widthScale <= heightScale) {
            d->paintedWidth = w;
            d->paintedHeight = widthScale * pixHeight;
        } else if (heightScale < widthScale) {
            d->paintedWidth = heightScale * pixWidth;
            d->paintedHeight = h;
        }
        const qreal iHeight = (widthValid() && !heightValid()) ? d->paintedHeight : pixHeight;
        const qreal iWidth = (heightValid() && !widthValid()) ? d->paintedWidth : pixWidth;
        setImplicitSize(iWidth, iHeight);

    } else if (d->fillMode == PreserveAspectCrop) {
        if (!d->currentPix->width() || !d->currentPix->height())
            return;
        const qreal pixWidth = d->currentPix->width() / d->devicePixelRatio;
        const qreal pixHeight = d->currentPix->height() / d->devicePixelRatio;
        qreal widthScale = width() / pixWidth;
        qreal heightScale = height() / pixHeight;
        if (widthScale < heightScale) {
            widthScale = heightScale;
        } else if (heightScale < widthScale) {
            heightScale = widthScale;
        }

        d->paintedHeight = heightScale * pixHeight;
        d->paintedWidth = widthScale * pixWidth;
    } else if (d->fillMode == Pad) {
        d->paintedWidth = d->currentPix->width() / d->devicePixelRatio;
        d->paintedHeight = d->currentPix->height() / d->devicePixelRatio;
    } else {
        d->paintedWidth = width();
        d->paintedHeight = height();
    }
    emit paintedGeometryChanged();
}

void QQuickImage::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickImageBase::geometryChange(newGeometry, oldGeometry);
    if (newGeometry.size() != oldGeometry.size())
        updatePaintedGeometry();
}

QRectF QQuickImage::boundingRect() const
{
    Q_D(const QQuickImage);
    return QRectF(0, 0, qMax(width(), d->paintedWidth), qMax(height(), d->paintedHeight));
}

QSGTextureProvider *QQuickImage::textureProvider() const
{
    Q_D(const QQuickImage);

    // When Item::layer::enabled == true, QQuickItem will be a texture
    // provider. In this case we should prefer to return the layer rather
    // than the image itself. The layer will include any children and any
    // the image's wrap and fill mode.
    if (QQuickItem::isTextureProvider())
        return QQuickItem::textureProvider();

    if (!d->window || !d->sceneGraphRenderContext() || QThread::currentThread() != d->sceneGraphRenderContext()->thread()) {
        qWarning("QQuickImage::textureProvider: can only be queried on the rendering thread of an exposed window");
        return nullptr;
    }

    if (!d->provider) {
        QQuickImagePrivate *dd = const_cast<QQuickImagePrivate *>(d);
        dd->provider = new QQuickImageTextureProvider;
        dd->provider->m_smooth = d->smooth;
        dd->provider->m_mipmap = d->mipmap;
        dd->provider->updateTexture(d->sceneGraphRenderContext()->textureForFactory(d->currentPix->textureFactory(), window()));
    }

    return d->provider;
}

void QQuickImage::invalidateSceneGraph()
{
    Q_D(QQuickImage);
    delete d->provider;
    d->provider = nullptr;
}

void QQuickImage::releaseResources()
{
    Q_D(QQuickImage);
    if (d->provider) {
        QQuickWindowQObjectCleanupJob::schedule(window(), d->provider);
        d->provider = nullptr;
    }
}

QSGNode *QQuickImage::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    Q_D(QQuickImage);

    QSGTexture *texture = d->sceneGraphRenderContext()->textureForFactory(d->currentPix->textureFactory(), window());

    // Copy over the current texture state into the texture provider...
    if (d->provider) {
        d->provider->m_smooth = d->smooth;
        d->provider->m_mipmap = d->mipmap;
        d->provider->updateTexture(texture);
    }

    if (!texture || width() <= 0 || height() <= 0) {
        delete oldNode;
        return nullptr;
    }

    QSGInternalImageNode *node = static_cast<QSGInternalImageNode *>(oldNode);
    if (!node) {
        d->pixmapChanged = true;
        node = d->sceneGraphContext()->createInternalImageNode(d->sceneGraphRenderContext());
    }

    QRectF targetRect;
    QRectF sourceRect;
    QSGTexture::WrapMode hWrap = QSGTexture::ClampToEdge;
    QSGTexture::WrapMode vWrap = QSGTexture::ClampToEdge;

    qreal pixWidth = (d->fillMode == PreserveAspectFit) ? d->paintedWidth : d->currentPix->width() / d->devicePixelRatio;
    qreal pixHeight = (d->fillMode == PreserveAspectFit) ? d->paintedHeight :  d->currentPix->height() / d->devicePixelRatio;

    int xOffset = 0;
    if (d->hAlign == QQuickImage::AlignHCenter)
        xOffset = (width() - pixWidth) / 2;
    else if (d->hAlign == QQuickImage::AlignRight)
        xOffset = qCeil(width() - pixWidth);

    int yOffset = 0;
    if (d->vAlign == QQuickImage::AlignVCenter)
        yOffset = (height() - pixHeight) / 2;
    else if (d->vAlign == QQuickImage::AlignBottom)
        yOffset = qCeil(height() - pixHeight);

    switch (d->fillMode) {
    case Stretch:
        targetRect = QRectF(0, 0, width(), height());
        sourceRect = d->currentPix->rect();
        break;

    case PreserveAspectFit:
        targetRect = QRectF(xOffset, yOffset, d->paintedWidth, d->paintedHeight);
        sourceRect = d->currentPix->rect();
        break;

    case PreserveAspectCrop: {
        targetRect = QRectF(0, 0, width(), height());
        qreal wscale = width() / qreal(d->currentPix->width());
        qreal hscale = height() / qreal(d->currentPix->height());

        if (wscale > hscale) {
            int src = (hscale / wscale) * qreal(d->currentPix->height());
            int y = 0;
            if (d->vAlign == QQuickImage::AlignVCenter)
                y = qCeil((d->currentPix->height() - src) / 2.);
            else if (d->vAlign == QQuickImage::AlignBottom)
                y = qCeil(d->currentPix->height() - src);
            sourceRect = QRectF(0, y, d->currentPix->width(), src);

        } else {
            int src = (wscale / hscale) * qreal(d->currentPix->width());
            int x = 0;
            if (d->hAlign == QQuickImage::AlignHCenter)
                x = qCeil((d->currentPix->width() - src) / 2.);
            else if (d->hAlign == QQuickImage::AlignRight)
                x = qCeil(d->currentPix->width() - src);
            sourceRect = QRectF(x, 0, src, d->currentPix->height());
        }
        }
        break;

    case Tile:
        targetRect = QRectF(0, 0, width(), height());
        sourceRect = QRectF(-xOffset, -yOffset, width(), height());
        hWrap = QSGTexture::Repeat;
        vWrap = QSGTexture::Repeat;
        break;

    case TileHorizontally:
        targetRect = QRectF(0, 0, width(), height());
        sourceRect = QRectF(-xOffset, 0, width(), d->currentPix->height());
        hWrap = QSGTexture::Repeat;
        break;

    case TileVertically:
        targetRect = QRectF(0, 0, width(), height());
        sourceRect = QRectF(0, -yOffset, d->currentPix->width(), height());
        vWrap = QSGTexture::Repeat;
        break;

    case Pad:
        qreal w = qMin(qreal(pixWidth), width());
        qreal h = qMin(qreal(pixHeight), height());
        qreal x = (pixWidth > width()) ? -xOffset : 0;
        qreal y = (pixHeight > height()) ? -yOffset : 0;
        targetRect = QRectF(x + xOffset, y + yOffset, w, h);
        sourceRect = QRectF(x, y, w, h);
        break;
    }

    qreal nsWidth = (hWrap == QSGTexture::Repeat || d->fillMode == Pad) ? d->currentPix->width() / d->devicePixelRatio : d->currentPix->width();
    qreal nsHeight = (vWrap == QSGTexture::Repeat || d->fillMode == Pad) ? d->currentPix->height() / d->devicePixelRatio : d->currentPix->height();
    QRectF nsrect(sourceRect.x() / nsWidth,
                  sourceRect.y() / nsHeight,
                  sourceRect.width() / nsWidth,
                  sourceRect.height() / nsHeight);

    if (targetRect.isEmpty()
        || !qt_is_finite(targetRect.width()) || !qt_is_finite(targetRect.height())
        || nsrect.isEmpty()
        || !qt_is_finite(nsrect.width()) || !qt_is_finite(nsrect.height())) {
        delete node;
        return nullptr;
    }

    if (d->pixmapChanged) {
        // force update the texture in the node to trigger reconstruction of
        // geometry and the likes when a atlas segment has changed.
        if (texture->isAtlasTexture() && (hWrap == QSGTexture::Repeat || vWrap == QSGTexture::Repeat || d->mipmap))
            node->setTexture(texture->removedFromAtlas());
        else
            node->setTexture(texture);
        d->pixmapChanged = false;
    }

    node->setMipmapFiltering(d->mipmap ? QSGTexture::Linear : QSGTexture::None);
    node->setHorizontalWrapMode(hWrap);
    node->setVerticalWrapMode(vWrap);
    node->setFiltering(d->smooth ? QSGTexture::Linear : QSGTexture::Nearest);

    node->setTargetRect(targetRect);
    node->setInnerTargetRect(targetRect);
    node->setSubSourceRect(nsrect);
    node->setMirror(d->mirrorHorizontally, d->mirrorVertically);
    node->setAntialiasing(d->antialiasing);
    node->update();

    return node;
}

void QQuickImage::pixmapChange()
{
    Q_D(QQuickImage);
    // PreserveAspectFit calculates the implicit size differently so we
    // don't call our superclass pixmapChange(), since that would
    // result in the implicit size being set incorrectly, then updated
    // in updatePaintedGeometry()
    if (d->fillMode != PreserveAspectFit)
        QQuickImageBase::pixmapChange();
    updatePaintedGeometry();
    d->pixmapChanged = true;

    // When the pixmap changes, such as being deleted, we need to update the textures
    update();
}

QQuickImage::VAlignment QQuickImage::verticalAlignment() const
{
    Q_D(const QQuickImage);
    return d->vAlign;
}

void QQuickImage::setVerticalAlignment(VAlignment align)
{
    Q_D(QQuickImage);
    if (d->vAlign == align)
        return;

    d->vAlign = align;
    update();
    updatePaintedGeometry();
    emit verticalAlignmentChanged(align);
}

QQuickImage::HAlignment QQuickImage::horizontalAlignment() const
{
    Q_D(const QQuickImage);
    return d->hAlign;
}

void QQuickImage::setHorizontalAlignment(HAlignment align)
{
    Q_D(QQuickImage);
    if (d->hAlign == align)
        return;

    d->hAlign = align;
    update();
    updatePaintedGeometry();
    emit horizontalAlignmentChanged(align);
}

/*!
    \qmlproperty bool QtQuick::Image::mipmap
    \since 5.3

    This property holds whether the image uses mipmap filtering when scaled or
    transformed.

    Mipmap filtering gives better visual quality when scaling down
    compared to smooth, but it may come at a performance cost (both when
    initializing the image and during rendering).

    By default, this property is set to false.

    \sa smooth
 */

bool QQuickImage::mipmap() const
{
    Q_D(const QQuickImage);
    return d->mipmap;
}

void QQuickImage::setMipmap(bool use)
{
    Q_D(QQuickImage);
    if (d->mipmap == use)
        return;
    d->mipmap = use;
    emit mipmapChanged(d->mipmap);

    d->pixmapChanged = true;
    if (isComponentComplete())
        load();
    update();
}

/*!
    \qmlproperty bool QtQuick::Image::autoTransform
    \since 5.5

    This property holds whether the image should automatically apply
    image transformation metadata such as EXIF orientation.

    By default, this property is set to false.
 */

/*!
    \qmlproperty int QtQuick::Image::currentFrame
    \qmlproperty int QtQuick::Image::frameCount
    \since 5.14

    currentFrame is the frame that is currently visible. The default is \c 0.
    You can set it to a number between \c 0 and \c {frameCount - 1} to display a
    different frame, if the image contains multiple frames.

    frameCount is the number of frames in the image. Most images have only one frame.
*/

/*!
    \qmlproperty bool QtQuick::Image::retainWhileLoading
    \since 6.8

//! [qml-image-retainwhileloading]
    This property defines the behavior when the \l source property is changed and loading happens
    asynchronously. This is the case when the \l asynchronous property is set to \c true, or if the
    image is not on the local file system.

    If \c retainWhileLoading is \c false (the default), the old image is discarded immediately, and
    the component is cleared while the new image is being loaded. If set to \c true, the old image
    is retained and remains visible until the new one is ready.

    Enabling this property can avoid flickering in cases where loading the new image takes a long
    time. It comes at the cost of some extra memory use for double buffering while the new image is
    being loaded.
//! [qml-image-retainwhileloading]
 */

QT_END_NAMESPACE

#include "moc_qquickimage_p_p.cpp"

#include "moc_qquickimage_p.cpp"
