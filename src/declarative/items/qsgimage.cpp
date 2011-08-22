/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
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

#include "qsgimage_p.h"
#include "qsgimage_p_p.h"

#include <private/qsgtextureprovider_p.h>

#include <private/qsgcontext_p.h>
#include <private/qsgadaptationlayer_p.h>

#include <QtGui/qpainter.h>
#include <qmath.h>

QT_BEGIN_NAMESPACE

class QSGImageTextureProvider : public QSGTextureProvider
{
    Q_OBJECT
public:
    QSGImageTextureProvider(const QSGImage *imageItem)
        : d((QSGImagePrivate *) QSGItemPrivate::get(imageItem))
        , m_texture(0)
        , m_smooth(false)
    {
    }

    QSGTexture *texture() const {
        if (m_texture) {
            m_texture->setFiltering(m_smooth ? QSGTexture::Linear : QSGTexture::Nearest);
            m_texture->setMipmapFiltering(QSGTexture::Nearest);
            m_texture->setHorizontalWrapMode(QSGTexture::ClampToEdge);
            m_texture->setVerticalWrapMode(QSGTexture::ClampToEdge);
        }
        return m_texture;
    }

    friend class QSGImage;

    QSGImagePrivate *d;
    QSGTexture *m_texture;
    bool m_smooth;
};

#include "qsgimage.moc"

QSGImagePrivate::QSGImagePrivate()
    : fillMode(QSGImage::Stretch)
    , paintedWidth(0)
    , paintedHeight(0)
    , pixmapChanged(false)
    , hAlign(QSGImage::AlignHCenter)
    , vAlign(QSGImage::AlignVCenter)
    , provider(0)
{
}

/*!
    \qmlclass Image QSGImage
    \since QtQuick 1.0
    \inqmlmodule QtQuick 2
    \ingroup qml-basic-visual-elements
    \brief The Image element displays an image in a declarative user interface
    \inherits Item

    The Image element is used to display images in a declarative user interface.

    The source of the image is specified as a URL using the \l source property.
    Images can be supplied in any of the standard image formats supported by Qt,
    including bitmap formats such as PNG and JPEG, and vector graphics formats
    such as SVG. If you need to display animated images, use the \l AnimatedImage
    element.

    If the \l{Item::width}{width} and \l{Item::height}{height} properties are not
    specified, the Image element automatically uses the size of the loaded image.
    By default, specifying the width and height of the element causes the image
    to be scaled to that size. This behavior can be changed by setting the
    \l fillMode property, allowing the image to be stretched and tiled instead.

    \section1 Example Usage

    The following example shows the simplest usage of the Image element.

    \snippet doc/src/snippets/declarative/image.qml document

    \beginfloatleft
    \image declarative-qtlogo.png
    \endfloat

    \clearfloat

    \section1 Performance

    By default, locally available images are loaded immediately, and the user interface
    is blocked until loading is complete. If a large image is to be loaded, it may be
    preferable to load the image in a low priority thread, by enabling the \l asynchronous
    property.

    If the image is obtained from a network rather than a local resource, it is
    automatically loaded asynchronously, and the \l progress and \l status properties
    are updated as appropriate.

    Images are cached and shared internally, so if several Image elements have the same \l source,
    only one copy of the image will be loaded.

    \bold Note: Images are often the greatest user of memory in QML user interfaces.  It is recommended
    that images which do not form part of the user interface have their
    size bounded via the \l sourceSize property. This is especially important for content
    that is loaded from external sources or provided by the user.

    \sa {declarative/imageelements/image}{Image example}, QDeclarativeImageProvider
*/

QSGImage::QSGImage(QSGItem *parent)
    : QSGImageBase(*(new QSGImagePrivate), parent)
{
}

QSGImage::QSGImage(QSGImagePrivate &dd, QSGItem *parent)
    : QSGImageBase(dd, parent)
{
}

QSGImage::~QSGImage()
{
    Q_D(QSGImage);
    if (d->provider)
        d->provider->deleteLater();
}

void QSGImagePrivate::setPixmap(const QPixmap &pixmap)
{
    Q_Q(QSGImage);
    pix.setPixmap(pixmap);

    q->pixmapChange();
    status = pix.isNull() ? QSGImageBase::Null : QSGImageBase::Ready;

    q->update();
}

/*!
    \qmlproperty enumeration QtQuick2::Image::fillMode

    Set this property to define what happens when the source image has a different size
    than the item.

    \list
    \o Image.Stretch - the image is scaled to fit
    \o Image.PreserveAspectFit - the image is scaled uniformly to fit without cropping
    \o Image.PreserveAspectCrop - the image is scaled uniformly to fill, cropping if necessary
    \o Image.Tile - the image is duplicated horizontally and vertically
    \o Image.TileVertically - the image is stretched horizontally and tiled vertically
    \o Image.TileHorizontally - the image is stretched vertically and tiled horizontally
    \o Image.Pad - the image is not transformed
    \endlist

    \table

    \row
    \o \image declarative-qtlogo-stretch.png
    \o Stretch (default)
    \qml
    Image {
        width: 130; height: 100
        smooth: true
        source: "qtlogo.png"
    }
    \endqml

    \row
    \o \image declarative-qtlogo-preserveaspectfit.png
    \o PreserveAspectFit
    \qml
    Image {
        width: 130; height: 100
        fillMode: Image.PreserveAspectFit
        smooth: true
        source: "qtlogo.png"
    }
    \endqml

    \row
    \o \image declarative-qtlogo-preserveaspectcrop.png
    \o PreserveAspectCrop
    \qml
    Image {
        width: 130; height: 100
        fillMode: Image.PreserveAspectCrop
        smooth: true
        source: "qtlogo.png"
        clip: true
    }
    \endqml

    \row
    \o \image declarative-qtlogo-tile.png
    \o Tile
    \qml
    Image {
        width: 120; height: 120
        fillMode: Image.Tile
        source: "qtlogo.png"
    }
    \endqml

    \row
    \o \image declarative-qtlogo-tilevertically.png
    \o TileVertically
    \qml
    Image {
        width: 120; height: 120
        fillMode: Image.TileVertically
        smooth: true
        source: "qtlogo.png"
    }
    \endqml

    \row
    \o \image declarative-qtlogo-tilehorizontally.png
    \o TileHorizontally
    \qml
    Image {
        width: 120; height: 120
        fillMode: Image.TileHorizontally
        smooth: true
        source: "qtlogo.png"
    }
    \endqml

    \endtable

    Note that \c clip is \c false by default which means that the element might
    paint outside its bounding rectangle even if the fillMode is set to \c PreserveAspectCrop.

    \sa {declarative/imageelements/image}{Image example}
*/
QSGImage::FillMode QSGImage::fillMode() const
{
    Q_D(const QSGImage);
    return d->fillMode;
}

void QSGImage::setFillMode(FillMode mode)
{
    Q_D(QSGImage);
    if (d->fillMode == mode)
        return;
    d->fillMode = mode;
    update();
    updatePaintedGeometry();
    emit fillModeChanged();
}

/*!

    \qmlproperty real QtQuick2::Image::paintedWidth
    \qmlproperty real QtQuick2::Image::paintedHeight

    These properties hold the size of the image that is actually painted.
    In most cases it is the same as \c width and \c height, but when using a
    \c fillMode \c PreserveAspectFit or \c fillMode \c PreserveAspectCrop
    \c paintedWidth or \c paintedHeight can be smaller or larger than
    \c width and \c height of the Image element.
*/
qreal QSGImage::paintedWidth() const
{
    Q_D(const QSGImage);
    return d->paintedWidth;
}

qreal QSGImage::paintedHeight() const
{
    Q_D(const QSGImage);
    return d->paintedHeight;
}

/*!
    \qmlproperty enumeration QtQuick2::Image::status

    This property holds the status of image loading.  It can be one of:
    \list
    \o Image.Null - no image has been set
    \o Image.Ready - the image has been loaded
    \o Image.Loading - the image is currently being loaded
    \o Image.Error - an error occurred while loading the image
    \endlist

    Use this status to provide an update or respond to the status change in some way.
    For example, you could:

    \list
    \o Trigger a state change:
    \qml
        State { name: 'loaded'; when: image.status == Image.Ready }
    \endqml

    \o Implement an \c onStatusChanged signal handler:
    \qml
        Image {
            id: image
            onStatusChanged: if (image.status == Image.Ready) console.log('Loaded')
        }
    \endqml

    \o Bind to the status value:
    \qml
        Text { text: image.status == Image.Ready ? 'Loaded' : 'Not loaded' }
    \endqml
    \endlist

    \sa progress
*/

/*!
    \qmlproperty real QtQuick2::Image::progress

    This property holds the progress of image loading, from 0.0 (nothing loaded)
    to 1.0 (finished).

    \sa status
*/

/*!
    \qmlproperty bool QtQuick2::Image::smooth

    Set this property if you want the image to be smoothly filtered when scaled or
    transformed.  Smooth filtering gives better visual quality, but is slower.  If
    the image is displayed at its natural size, this property has no visual or
    performance effect.

    \note Generally scaling artifacts are only visible if the image is stationary on
    the screen.  A common pattern when animating an image is to disable smooth
    filtering at the beginning of the animation and reenable it at the conclusion.
*/

/*!
    \qmlproperty QSize QtQuick2::Image::sourceSize

    This property holds the actual width and height of the loaded image.

    Unlike the \l {Item::}{width} and \l {Item::}{height} properties, which scale
    the painting of the image, this property sets the actual number of pixels
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

    If the source is an instrinsically scalable image (eg. SVG), this property
    determines the size of the loaded image regardless of intrinsic size.
    Avoid changing this property dynamically; rendering an SVG is \e slow compared
    to an image.

    If the source is a non-scalable image (eg. JPEG), the loaded image will
    be no greater than this property specifies. For some formats (currently only JPEG),
    the whole image will never actually be loaded into memory.

    Since QtQuick 1.1 the sourceSize can be cleared to the natural size of the image
    by setting sourceSize to \c undefined.

    \note \e {Changing this property dynamically causes the image source to be reloaded,
    potentially even from the network, if it is not in the disk cache.}
*/

/*!
    \qmlproperty url QtQuick2::Image::source

    Image can handle any image format supported by Qt, loaded from any URL scheme supported by Qt.

    The URL may be absolute, or relative to the URL of the component.

    \sa QDeclarativeImageProvider
*/

/*!
    \qmlproperty bool QtQuick2::Image::asynchronous

    Specifies that images on the local filesystem should be loaded
    asynchronously in a separate thread.  The default value is
    false, causing the user interface thread to block while the
    image is loaded.  Setting \a asynchronous to true is useful where
    maintaining a responsive user interface is more desirable
    than having images immediately visible.

    Note that this property is only valid for images read from the
    local filesystem.  Images loaded via a network resource (e.g. HTTP)
    are always loaded asynchonously.
*/

/*!
    \qmlproperty bool QtQuick2::Image::cache
    \since Quick 1.1

    Specifies whether the image should be cached. The default value is
    true. Setting \a cache to false is useful when dealing with large images,
    to make sure that they aren't cached at the expense of small 'ui element' images.
*/

/*!
    \qmlproperty bool QtQuick2::Image::mirror
    \since Quick 1.1

    This property holds whether the image should be horizontally inverted
    (effectively displaying a mirrored image).

    The default value is false.
*/

/*!
    \qmlproperty enumeration QtQuick2::Image::horizontalAlignment
    \qmlproperty enumeration QtQuick2::Image::verticalAlignment
    \since Quick 2.0

    Sets the horizontal and vertical alignment of the image. By default, the image is top-left aligned.

    The valid values for \c horizontalAlignment are \c Image.AlignLeft, \c Image.AlignRight and \c Image.AlignHCenter.
    The valid values for \c verticalAlignment are \c Image.AlignTop, \c Image.AlignBottom
    and \c Image.AlignVCenter.
*/
void QSGImage::updatePaintedGeometry()
{
    Q_D(QSGImage);

    if (d->fillMode == PreserveAspectFit) {
        if (!d->pix.width() || !d->pix.height()) {
            setImplicitWidth(0);
            setImplicitHeight(0);
            return;
        }
        qreal w = widthValid() ? width() : d->pix.width();
        qreal widthScale = w / qreal(d->pix.width());
        qreal h = heightValid() ? height() : d->pix.height();
        qreal heightScale = h / qreal(d->pix.height());
        if (widthScale <= heightScale) {
            d->paintedWidth = w;
            d->paintedHeight = widthScale * qreal(d->pix.height());
        } else if(heightScale < widthScale) {
            d->paintedWidth = heightScale * qreal(d->pix.width());
            d->paintedHeight = h;
        }
        if (widthValid() && !heightValid()) {
            setImplicitHeight(d->paintedHeight);
        } else {
            setImplicitHeight(d->pix.height());
        }
        if (heightValid() && !widthValid()) {
            setImplicitWidth(d->paintedWidth);
        } else {
            setImplicitWidth(d->pix.width());
        }
    } else if (d->fillMode == PreserveAspectCrop) {
        if (!d->pix.width() || !d->pix.height())
            return;
        qreal widthScale = width() / qreal(d->pix.width());
        qreal heightScale = height() / qreal(d->pix.height());
        if (widthScale < heightScale) {
            widthScale = heightScale;
        } else if(heightScale < widthScale) {
            heightScale = widthScale;
        }

        d->paintedHeight = heightScale * qreal(d->pix.height());
        d->paintedWidth = widthScale * qreal(d->pix.width());
    } else if (d->fillMode == Pad) {
        d->paintedWidth = d->pix.width();
        d->paintedHeight = d->pix.height();
    } else {
        d->paintedWidth = width();
        d->paintedHeight = height();
    }
    emit paintedGeometryChanged();
}

void QSGImage::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QSGImageBase::geometryChanged(newGeometry, oldGeometry);
    updatePaintedGeometry();
}

QRectF QSGImage::boundingRect() const
{
    Q_D(const QSGImage);
    return QRectF(0, 0, qMax(width(), d->paintedWidth), qMax(height(), d->paintedHeight));
}

QSGTextureProvider *QSGImage::textureProvider() const
{
    Q_D(const QSGImage);
    if (!d->provider) {
        // Make sure it gets thread affinity on the rendering thread so deletion works properly..
        Q_ASSERT_X(d->canvas
                   && d->sceneGraphContext()
                   && QThread::currentThread() == d->sceneGraphContext()->thread(),
                   "QSGImage::textureProvider",
                   "Cannot be used outside the GUI thread");
        const_cast<QSGImagePrivate *>(d)->provider = new QSGImageTextureProvider(this);
    }

    return d->provider;
}

QSGNode *QSGImage::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    Q_D(QSGImage);

    QSGTexture *texture = d->pix.texture(d->sceneGraphContext());

    // Copy over the current texture state into the texture provider...
    if (d->provider) {
        d->provider->m_smooth = d->smooth;
        d->provider->m_texture = texture;
    }

    if (!texture || width() <= 0 || height() <= 0) {
        delete oldNode;
        return 0;
    }

    QSGImageNode *node = static_cast<QSGImageNode *>(oldNode);
    if (!node) { 
        d->pixmapChanged = true;
        node = d->sceneGraphContext()->createImageNode();
        node->setTexture(texture);
    }

    if (d->pixmapChanged) {
        // force update the texture in the node to trigger reconstruction of
        // geometry and the likes when a atlas segment has changed.
        node->setTexture(0);
        node->setTexture(texture);
        d->pixmapChanged = false;
    }

    QRectF targetRect;
    QRectF sourceRect;
    QSGTexture::WrapMode hWrap = QSGTexture::ClampToEdge;
    QSGTexture::WrapMode vWrap = QSGTexture::ClampToEdge;

    qreal pixWidth = (d->fillMode == PreserveAspectFit) ? d->paintedWidth : d->pix.width();
    qreal pixHeight = (d->fillMode == PreserveAspectFit) ? d->paintedHeight : d->pix.height();

    int xOffset = 0;
    if (d->hAlign == QSGImage::AlignHCenter)
        xOffset = qCeil((width() - pixWidth) / 2.);
    else if (d->hAlign == QSGImage::AlignRight)
        xOffset = qCeil(width() - pixWidth);

    int yOffset = 0;
    if (d->vAlign == QSGImage::AlignVCenter)
        yOffset = qCeil((height() - pixHeight) / 2.);
    else if (d->vAlign == QSGImage::AlignBottom)
        yOffset = qCeil(height() - pixHeight);

    switch (d->fillMode) {
    default:
    case Stretch:
        targetRect = QRectF(0, 0, width(), height());
        sourceRect = d->pix.rect();
        break;

    case PreserveAspectFit:
        targetRect = QRectF(xOffset, yOffset, d->paintedWidth, d->paintedHeight);
        sourceRect = d->pix.rect();
        break;

    case PreserveAspectCrop: {
        targetRect = QRect(0, 0, width(), height());
        qreal wscale = width() / qreal(d->pix.width());
        qreal hscale = height() / qreal(d->pix.height());

        if (wscale > hscale) {
            int src = (hscale / wscale) * qreal(d->pix.height());
            int y = 0;
            if (d->vAlign == QSGImage::AlignVCenter)
                y = qCeil((d->pix.height() - src) / 2.);
            else if (d->vAlign == QSGImage::AlignBottom)
                y = qCeil(d->pix.height() - src);
            sourceRect = QRectF(0, y, d->pix.width(), src);

        } else {
            int src = (wscale / hscale) * qreal(d->pix.width());
            int x = 0;
            if (d->hAlign == QSGImage::AlignHCenter)
                x = qCeil((d->pix.width() - src) / 2.);
            else if (d->hAlign == QSGImage::AlignRight)
                x = qCeil(d->pix.width() - src);
            sourceRect = QRectF(x, 0, src, d->pix.height());
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
        sourceRect = QRectF(-xOffset, 0, width(), d->pix.height());
        hWrap = QSGTexture::Repeat;
        break;

    case TileVertically:
        targetRect = QRectF(0, 0, width(), height());
        sourceRect = QRectF(0, -yOffset, d->pix.width(), height());
        vWrap = QSGTexture::Repeat;
        break;

    case Pad:
        qreal w = qMin(qreal(d->pix.width()), width());
        qreal h = qMin(qreal(d->pix.height()), height());
        qreal x = (d->pix.width() > width()) ? -xOffset : 0;
        qreal y = (d->pix.height() > height()) ? -yOffset : 0;
        targetRect = QRectF(x + xOffset, y + yOffset, w, h);
        sourceRect = QRectF(x, y, w, h);
        break;
    };

    QRectF nsrect(sourceRect.x() / d->pix.width(),
                  sourceRect.y() / d->pix.height(),
                  sourceRect.width() / d->pix.width(),
                  sourceRect.height() / d->pix.height());

    if (d->mirror) {
        qreal oldLeft = nsrect.left();
        nsrect.setLeft(nsrect.right());
        nsrect.setRight(oldLeft);
    }

    node->setHorizontalWrapMode(hWrap);
    node->setVerticalWrapMode(vWrap);
    node->setFiltering(d->smooth ? QSGTexture::Linear : QSGTexture::Nearest);

    node->setTargetRect(targetRect);
    node->setSourceRect(nsrect);
    node->update();

    return node;
}

void QSGImage::pixmapChange()
{
    Q_D(QSGImage);
    // PreserveAspectFit calculates the implicit size differently so we
    // don't call our superclass pixmapChange(), since that would
    // result in the implicit size being set incorrectly, then updated
    // in updatePaintedGeometry()
    if (d->fillMode != PreserveAspectFit)
        QSGImageBase::pixmapChange();
    updatePaintedGeometry();
    d->pixmapChanged = true;
}

QSGImage::VAlignment QSGImage::verticalAlignment() const
{
    Q_D(const QSGImage);
    return d->vAlign;
}

void QSGImage::setVerticalAlignment(VAlignment align)
{
    Q_D(QSGImage);
    if (d->vAlign == align)
        return;

    d->vAlign = align;
    update();
    updatePaintedGeometry();
    emit verticalAlignmentChanged(align);
}

QSGImage::HAlignment QSGImage::horizontalAlignment() const
{
    Q_D(const QSGImage);
    return d->hAlign;
}

void QSGImage::setHorizontalAlignment(HAlignment align)
{
    Q_D(QSGImage);
    if (d->hAlign == align)
        return;

    d->hAlign = align;
    update();
    updatePaintedGeometry();
    emit horizontalAlignmentChanged(align);
}

QT_END_NAMESPACE
