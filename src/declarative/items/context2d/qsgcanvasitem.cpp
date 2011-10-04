/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "private/qsgadaptationlayer_p.h"
#include "qsgcanvasitem_p.h"
#include "qsgitem_p.h"
#include "qsgcontext2d_p.h"
#include "qsgcontext2dnode_p.h"
#include "qsgcontext2dtexture_p.h"
#include "qdeclarativepixmapcache_p.h"

#include <qdeclarativeinfo.h>
#include "qdeclarativeengine_p.h"
#include <QtCore/QBuffer>

QT_BEGIN_NAMESPACE

class QSGCanvasItemPrivate : public QSGItemPrivate
{
public:
    QSGCanvasItemPrivate();
    ~QSGCanvasItemPrivate();
    QSGContext2D* context;
    QSGContext2DTexture* texture;
    QSizeF canvasSize;
    QSize tileSize;
    QRectF canvasWindow;
    QRectF dirtyRect;
    uint renderInThread : 1;
    uint hasCanvasSize :1;
    uint hasTileSize :1;
    uint hasCanvasWindow :1;
    uint componentCompleted :1;
    QSGCanvasItem::RenderTarget renderTarget;
    QHash<QUrl, QDeclarativePixmap*> images;
    QUrl baseUrl;
};

QSGCanvasItemPrivate::QSGCanvasItemPrivate()
    : QSGItemPrivate()
    , context(0)
    , texture(0)
    , canvasSize(1, 1)
    , tileSize(1, 1)
    , renderInThread(false)
    , hasCanvasSize(false)
    , hasTileSize(false)
    , hasCanvasWindow(false)
    , componentCompleted(false)
    , renderTarget(QSGCanvasItem::FramebufferObject)
{
}

QSGCanvasItemPrivate::~QSGCanvasItemPrivate()
{
    qDeleteAll(images);
}

/*!
    \qmlclass Canvas QSGCanvasItem
    \inqmlmodule QtQuick 2
    \since QtQuick 2.0
    \brief The Canvas item provides HTML5 like canvas element which enables you to
    draw within the item area by using Javascript.
    \inherits Item
    \ingroup qml-basic-visual-elements

    With the Canvas item, users can draw straight and curved lines, simple and
    complex shapes, graphs, and referenced graphic images. can also add texts, colors,
    shadows, gradients, and patterns, and do low level pixel operations, etc. The Canvas item
    also enables you to save or export the canvas as a image file or serialize the image data
    to data url string.

    To define a drawing area in the Canvas item, just set the \c width and \c height properties.
    For example, the following code creates a Canvas item which has a drawing area with a height of 100
    pixels and width of 200 pixels:
    \qml
    import QtQuick 2.0
    Canvas {
      id:mycanvas
      width:100
      height:200
    }
    \endqml

    Currently the Canvas item only supports the two-dimensional rendering context.

    \section1 Thread Rendering and Render Target
    The Canvas item supports two render targets:Canvas.Image and Canvas.FramebufferObject.
    The Canvas.Image render target is a \a QImage object which is actually a block of system
    memory. This render target support background thread rendering. So if some complex or long
    running painting need to be done, the Canvas.Image with thread rendering mode should be
    chosen to avoid blocking the UI. Otherwise the Canvas.FramebufferObject render target should
    be chosen as it could be much faster with good OpenGL hardware accelaration than rendering into
    system memory, especially when the CPU is already very busy.

    The default render target is Canvas.Image and the default renderInThread property is
    false.

    \section1 Tiled Canvas
    The Canvas item also supports tiled rendering mode by setting the proper canvasSize, tileSize
    and the canvasWindow properties.

    With tiled canvas, a virtually very large canvas can be provided by a relatively small canvas
    window. The actual memory consumption only relates to the canvas window size. So the canvas size
    can be chosen freely as needed. The painting code then doesn't need to worry about the coordinate
    system and complex matrix transformations at all.

    As a side effect, by setting a good tile size, the tiles overlapped with the canvas window could be
    cached and don't need to redraw, which can improve the performance significantly in some situations.

    \section1 Pixel Operations
    The Canvas item support all HTML5 2d context pixel operations. In order to get better
    pixel reading/writing performance, the Canvas.Image render target should be chosen. As
    for Canvas.FramebufferObject render target, the pixel data need to be exchanged between
    the system memory and the graphic card, which can't be benefit from the hardware acceleration
    at all. And the OpenGL rendering may synchronise with the V-Sync signal to avoid the
    {en.wikipedia.org/wiki/Screen_tearing}{screen tearing} which makes the pixel operations
    even slower with the Canvas.FrambufferObject render target.

    \section1 Tips for Porting Existing HTML5 Canvas applications

    Although the Canvas item is provided as a HTML5 like API, and
    the canvas context API is as compatible with HTML5 2d context standard
    as possible, the working HTML5 canvas applications are still need to
    be modified to run in the Canvas item:
    \list
    \o Removes and replaces all DOM API calls with QML property bindings or Canvas item methods.
    \o Removes and replaces all HTML envent handlers with the \a MouseArea item.
    \o Changes the setInterval/setTimeout function calls with the \a Timer item.
    \o Puts the actual painting code into the \a QtQuick2::Canvas::onPaint handler and triggers the
       painting by calling the Canvas's \c markDirty or \c requestPaint methods.
    \o For drawing images, loads them by calling the Canvas's loadImage method and then request to paint
       them in the onImageLoaded handler.
    \endlist

    \sa QtQuick2::Context2D
*/

QSGCanvasItem::QSGCanvasItem(QSGItem *parent)
    : QSGItem(*(new QSGCanvasItemPrivate), parent)
{
    setFlag(ItemHasContents);
}

QSGCanvasItem::~QSGCanvasItem()
{
    Q_D(QSGCanvasItem);
    if (d->texture) {
        d->texture->setItem(0);
        d->texture->deleteLater();
    }
    delete d->context;
}

/*!
    \qmlproperty size QtQuick2::Canvas::canvasSize
     Holds the logical canvas size that the context paints on.

     By default, the canvas size is the same size as the current canvas item size.
     By setting the canvas size, tile size and canvas window, the Canvas
     item can act as a virtual large canvas with many seperately rendered tile rectangle
     areas. Only those tiles within the current canvas window would be painted by
     the Canvas render engine.
    \sa QtQuick2::Canvas::tileSize QtQuick2::Canvas::canvasWindow
*/
QSizeF QSGCanvasItem::canvasSize() const
{
    Q_D(const QSGCanvasItem);
    return d->canvasSize;
}

void QSGCanvasItem::setCanvasSize(const QSizeF & size)
{
    Q_D(QSGCanvasItem);
    if (d->canvasSize != size) {
        d->hasCanvasSize = true;
        d->canvasSize = size;
        emit canvasSizeChanged();
        polish();
        update();
    }
}

/*!
    \qmlproperty size QtQuick2::Canvas::tileSize
     Holds the canvas rendering tile size.

     When the Canvas item in tiled mode by setting the canvas size, tile size and
     the canvas window. The canvas render can improve the rendering performance
     by rendering and caching tiles instead of rendering the whole canvas everytime.

     Additionally, the canvas size could be infinitely large without allocating more
     memories because only those tiles within the current visible region
     are actually rendered.

     By default, the tile size is the same with the canvas size.
     \sa QtQuick2::Canvas::canvaasSize QtQuick2::Canvas::canvasWindow
*/
QSize QSGCanvasItem::tileSize() const
{
    Q_D(const QSGCanvasItem);
    return d->tileSize;
}

void QSGCanvasItem::setTileSize(const QSize & size)
{
    Q_D(QSGCanvasItem);
    if (d->tileSize != size) {
        d->hasTileSize = true;
        d->tileSize = size;

        emit tileSizeChanged();
        polish();
        update();
    }
}

/*!
    \qmlproperty rect QtQuick2::Canvas::canvasWindow
     Holds the current canvas visible window.

     By default, the canvas window size is the same as the Canvas item
     size with the topleft point as (0, 0).

     If the canvas size is different with the Canvas item size, the Canvas
     item can display different visible areas by changing the canvas window's size
     and/or position.
    \sa QtQuick2::Canvas::canvasSize QtQuick2::Canvas::tileSize
*/
QRectF QSGCanvasItem::canvasWindow() const
{
    Q_D(const QSGCanvasItem);
    return d->canvasWindow;
}

void QSGCanvasItem::setCanvasWindow(const QRectF& rect)
{
    Q_D(QSGCanvasItem);
    if (d->canvasWindow != rect) {
        d->canvasWindow = rect;

        d->hasCanvasWindow = true;
        emit canvasWindowChanged();
        polish();
        update();
    }
}


QSGContext2D* QSGCanvasItem::context() const
{
    Q_D(const QSGCanvasItem);
    return d->context;
}
/*!
    \qmlproperty bool QtQuick2::Canvas::renderInThread
     Holds the current canvas rendering mode.

     By setting the renderInThread to true, complex and long
     running painting can be rendered in a dedicated background
     rendering thread to avoid blocking the main GUI.

     Note: Different renderTarget may or may not support the
     background rendering thread, if not, the renderInThread
     property will be ignored.

     The default value is false.
    \sa QtQuick2::Canvas::renderTarget
*/
bool QSGCanvasItem::renderInThread() const
{
    Q_D(const QSGCanvasItem);
    return d->renderInThread;
}
/*!
    \qmlproperty bool QtQuick2::Canvas::renderTarget
     Holds the current canvas render target.

     \list
     \o Canvas.Image  - render to an in memory image buffer, the render
                        target supports background rendering.
     \o Canvas.FramebufferObject - render to an OpenGL frame buffer,
                                   this render target will ignore the
                                   renderInThread property. The actual
                                   rendering happens in the main QML rendering
                                   process, which may be in a seperate render thread
                                   or in the main GUI thread depends on the platforms.
     \endlist

     The default render target is \c Canvas.Image.
    \sa QtQuick2::Canvas::renderInThread
*/
QSGCanvasItem::RenderTarget QSGCanvasItem::renderTarget() const
{
    Q_D(const QSGCanvasItem);
    return d->renderTarget;
}

void QSGCanvasItem::setRenderTarget(RenderTarget target)
{
    Q_D(QSGCanvasItem);
    if (d->renderTarget != target) {
        d->renderTarget = target;

        if (d->componentCompleted)
            createTexture();
        emit renderTargetChanged();
    }
}

void QSGCanvasItem::_doPainting(const QRectF& region)
{
    Q_D(QSGCanvasItem);
    emit paint(QDeclarativeV8Handle::fromHandle(d->context->v8value())
             , QSGContext2DTexture::tiledRect(region, d->tileSize));
    if (d->texture)
        d->texture->wake();
}

/*!
    \qmlproperty bool QtQuick2::Canvas::renderInThread
     Holds the current canvas rendering mode.

     When this property is true, all canvas painting commands
     are rendered in a background rendering thread, otherwise
     the rendering happens in the main GUI thread.

     The default renderInThread value is false.
*/
void QSGCanvasItem::setRenderInThread(bool renderInThread)
{
    Q_D(QSGCanvasItem);
    if (d->renderInThread != renderInThread) {
        d->renderInThread = renderInThread;

        if (d->componentCompleted)
            createTexture();

        if (d->renderInThread)
            connect(this, SIGNAL(painted()), SLOT(update()));
        else
            disconnect(this, SIGNAL(painted()), this, SLOT(update()));
        emit renderInThreadChanged();
        polish();
        update();
    }
}

void QSGCanvasItem::geometryChanged(const QRectF &newGeometry,
                             const QRectF &oldGeometry)
{
    Q_D(QSGCanvasItem);
    QSGItem::geometryChanged(newGeometry, oldGeometry);

    const qreal w = newGeometry.width();
    const qreal h = newGeometry.height();

    if (!d->hasCanvasSize) {
        d->canvasSize = QSizeF(w, h);
        emit canvasSizeChanged();
    }

    if (!d->hasTileSize) {
        d->tileSize = d->canvasSize.toSize();
        emit tileSizeChanged();
    }

    if (!d->hasCanvasWindow) {
        d->canvasWindow = newGeometry;
        emit canvasWindowChanged();
    }

    polish();
    update();
}

void QSGCanvasItem::componentComplete()
{
    Q_D(QSGCanvasItem);

    if (!d->context)
        createContext();
    createTexture();


    _doPainting(canvasWindow());
    QSGItem::componentComplete();

    d->baseUrl = qmlEngine(this)->contextForObject(this)->baseUrl();
    d->componentCompleted = true;
    update();
}

void QSGCanvasItem::updatePolish()
{
    Q_D(QSGCanvasItem);
    QSGItem::updatePolish();
    if (d->texture) {
        if (!d->renderInThread && d->dirtyRect.isValid())
            _doPainting(d->dirtyRect);

        d->texture->canvasChanged(d->canvasSize.toSize()
                                , d->tileSize
                                , d->canvasWindow.toAlignedRect()
                                , d->dirtyRect.toAlignedRect()
                                , d->smooth);
        d->dirtyRect = QRectF();
    }
}

QSGNode *QSGCanvasItem::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    Q_D(QSGCanvasItem);
    QSGContext2DNode *node = static_cast<QSGContext2DNode *>(oldNode);
    if (!node)
        node = new QSGContext2DNode(this);

    node->setTexture(d->texture);
    node->update();
    return node;
}

void QSGCanvasItem::createTexture()
{
    Q_D(QSGCanvasItem);

    if (!d->texture
      || d->texture->threadRendering() != d->renderInThread
      || d->texture->renderTarget() != d->renderTarget) {
        if (d->texture) {
            d->texture->deleteLater();
            d->texture = 0;
        }

        if (d->renderTarget == QSGCanvasItem::Image) {
            d->texture = new QSGContext2DImageTexture(d->renderInThread);
        } else if (d->renderTarget == QSGCanvasItem::FramebufferObject) {
            d->texture = new QSGContext2DFBOTexture();
        }

        if (d->renderInThread && !d->texture->supportThreadRendering()) {
            qWarning("Canvas: render target does not support thread rendering, force to non-thread rendering mode.");
            d->renderInThread = false;
            emit renderInThreadChanged();
        }

        if (d->renderInThread)
            connect(d->texture, SIGNAL(textureChanged()), this, SLOT(update()));

        d->texture->setItem(this);
    }
}

void QSGCanvasItem::createContext()
{
    Q_D(QSGCanvasItem);

    delete d->context;

    d->context = new QSGContext2D(this);

    QV8Engine *e = QDeclarativeEnginePrivate::getV8Engine(qmlEngine(this));
    d->context->setV8Engine(e);
}

/*!
  \qmlmethod QtQuick2::Context2D QtQuick2::Canvas::getContext(string contextId)

  Currently, the canvas item only support the 2D context. If the \a contextId
  parameter isn't provided or is "2d", then the QtQuick2::Context2D object is
  returned, otherwise returns an invalid value.
  */
QDeclarativeV8Handle QSGCanvasItem::getContext(const QString &contextId)
{
    Q_D(QSGCanvasItem);

    if (contextId.toLower() != QLatin1String("2d"))
        return QDeclarativeV8Handle::fromHandle(v8::Undefined());

    if (!d->context)
        createContext();
    return QDeclarativeV8Handle::fromHandle(d->context->v8value());
}

/*!
  \qmlmethod void QtQuick2::Canvas::markDirty(rect region)

  Mark the given \a region as dirty, so that when this region is visible
  the canvas render will redraw it. During the rendering process, the
  canvas renderer may emit the canvas' "paint" signal so the actual painting
  scripts can be putted into the canvas's "onPaint" signal handler function.

  \sa QtQuick2::Canvas::paint QtQuick2::Canvas::requestPaint
  */
void QSGCanvasItem::markDirty(const QRectF& region)
{
    Q_D(QSGCanvasItem);
    d->dirtyRect |= region;
    polish();
    update();
}


/*!
  \qmlmethod bool QtQuick2::Canvas::save(string filename)

   Save the current canvas content into an image file \a filename.
   The saved image format is automatically decided by the \a filename's
   suffix.

   Note: calling this method will force painting the whole canvas, not the
   current canvas visible window.

   \sa canvasWindow canvasSize toDataURL
  */
bool QSGCanvasItem::save(const QString &filename) const
{
    Q_D(const QSGCanvasItem);
    QUrl url = d->baseUrl.resolved(QUrl::fromLocalFile(filename));
    return toImage().save(url.toLocalFile());
}

QImage QSGCanvasItem::loadedImage(const QUrl& url)
{
    Q_D(QSGCanvasItem);
    QUrl fullPathUrl = d->baseUrl.resolved(url);
    if (!d->images.contains(fullPathUrl)) {
        loadImage(url);
    }
    QDeclarativePixmap* pix = d->images.value(fullPathUrl);
    if (pix->isLoading() || pix->isError()) {
        return QImage();
    }
    return pix->pixmap().toImage();
}

/*!
  \qmlmethod void QtQuick2::Canvas::loadImage(url image)
    Loads the given \c image asynchronously, when the image is
    ready, an imageLoaded signal will be emitted.
    The loaded image can be unloaded by the \a QtQuick2::Canvas::unloadImage method.

    Note: Only loaded images can be painted on the Canvas item.
  \sa QtQuick2::Canvas::unloadImage QtQuick2::Canvas::imageLoaded QtQuick2::Canvas::isImageLoaded
  \sa QtQuick2::Context2D::createImageData QtQuick2::Context2D::drawImage
  */
void QSGCanvasItem::loadImage(const QUrl& url)
{
    Q_D(QSGCanvasItem);
    QUrl fullPathUrl = d->baseUrl.resolved(url);
    if (!d->images.contains(fullPathUrl)) {
        QDeclarativePixmap* pix = new QDeclarativePixmap();
        d->images.insert(fullPathUrl, pix);

        pix->load(qmlEngine(this)
                , fullPathUrl
                , QDeclarativePixmap::Cache | QDeclarativePixmap::Asynchronous);
        pix->connectFinished(this, SIGNAL(imageLoaded()));
    }
}
/*!
  \qmlmethod void QtQuick2::Canvas::loadImage(url image)
  Unloads the \c image.

  If the image is unloaded from the Canvas item, it can't be painted by the canvas context
  until it's loaded again.

  \sa QtQuick2::Canvas::loadImage QtQuick2::Canvas::imageLoaded QtQuick2::Canvas::isImageLoaded
  \sa QtQuick2::Context2D::createImageData QtQuick2::Context2D::drawImage
  */
void QSGCanvasItem::unloadImage(const QUrl& url)
{
    Q_D(QSGCanvasItem);
    QUrl removeThis = d->baseUrl.resolved(url);
    if (d->images.contains(removeThis)) {
        delete d->images.value(removeThis);
        d->images.remove(removeThis);
    }
}

/*!
  \qmlmethod void QtQuick2::Canvas::isImageError(url image)
  Returns true if the image can't be loaded because of error happens.

  \sa QtQuick2::Canvas::loadImage
  */
bool QSGCanvasItem::isImageError(const QUrl& url) const
{
    Q_D(const QSGCanvasItem);
    QUrl fullPathUrl = d->baseUrl.resolved(url);
    return d->images.contains(fullPathUrl)
        && d->images.value(fullPathUrl)->isError();
}

/*!
  \qmlmethod void QtQuick2::Canvas::isImageLoading(url image)
  Returns true if the Canvas item still is loading the \c image.

  \sa QtQuick2::Canvas::loadImage
  */
bool QSGCanvasItem::isImageLoading(const QUrl& url) const
{
    Q_D(const QSGCanvasItem);
    QUrl fullPathUrl = d->baseUrl.resolved(url);
    return d->images.contains(fullPathUrl)
        && d->images.value(fullPathUrl)->isLoading();
}
/*!
  \qmlmethod void QtQuick2::Canvas::isImageLoaded(url image)
  Returns true if the \c image is sucessfully loaded and ready to use.

  \sa QtQuick2::Canvas::loadImage
  */
bool QSGCanvasItem::isImageLoaded(const QUrl& url) const
{
    Q_D(const QSGCanvasItem);
    QUrl fullPathUrl = d->baseUrl.resolved(url);
    return d->images.contains(fullPathUrl)
        && d->images.value(fullPathUrl)->isReady();
}

QImage QSGCanvasItem::toImage(const QRectF& region) const
{
    Q_D(const QSGCanvasItem);
    if (d->texture) {
        if (region.isEmpty())
            return d->texture->toImage(canvasWindow());
        else
            return d->texture->toImage(region);
    }
    return QImage();
}

/*!
  \qmlmethod string QtQuick2::Canvas::toDataURL(string mimeType)

   Returns a data: URL for the image in the canvas.

   The default \a mimeType is "image/png".

   \sa QtQuick2::Canvas::save
  */
QString QSGCanvasItem::toDataURL(const QString& mimeType) const
{
    QImage image = toImage();

    if (!image.isNull()) {
        QByteArray ba;
        QBuffer buffer(&ba);
        buffer.open(QIODevice::WriteOnly);
        QString mime = mimeType.toLower();
        QString type;
        if (mime == QLatin1Literal("image/png")) {
            type = QLatin1Literal("PNG");
        } else if (mime == QLatin1Literal("image/bmp"))
            type = QLatin1Literal("BMP");
        else if (mime == QLatin1Literal("image/jpeg"))
            type = QLatin1Literal("JPEG");
        else if (mime == QLatin1Literal("image/x-portable-pixmap"))
            type = QLatin1Literal("PPM");
        else if (mime == QLatin1Literal("image/tiff"))
            type = QLatin1Literal("TIFF");
        else if (mime == QLatin1Literal("image/xbm"))
            type = QLatin1Literal("XBM");
        else if (mime == QLatin1Literal("image/xpm"))
            type = QLatin1Literal("XPM");
        else
            return QLatin1Literal("data:,");

        image.save(&buffer, type.toAscii());
        buffer.close();
        QString dataUrl = QLatin1Literal("data:%1;base64,%2");
        return dataUrl.arg(mime).arg(QLatin1String(ba.toBase64().constData()));
    }
    return QLatin1Literal("data:,");
}

/*!
    \qmlsignal QtQuick2::Canvas::onPaint(QtQuick2::Context2D context, rect region)

    This handler is called before the given \c region needs to be rendered.

    This signal can be triggered by QtQuick2::Canvas::markdirty, QtQuick2::Canvas::requestPaint
    or by changing the current canvas window.
*/

/*!
    \qmlsignal QtQuick2::Canvas::onPainted()

    This handler is called after all context painting commands are executed and
    the Canvas is actually rendered.
*/

QT_END_NAMESPACE
