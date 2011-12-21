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

#include <private/qsgadaptationlayer_p.h>
#include "qquickcanvasitem_p.h"
#include <private/qquickitem_p.h>
#include "qquickcontext2d_p.h"
#include "qquickcontext2dnode_p.h"
#include "qquickcontext2dtexture_p.h"
#include <QtQuick/private/qdeclarativepixmapcache_p.h>

#include <qdeclarativeinfo.h>
#include <private/qdeclarativeengine_p.h>
#include <QtCore/QBuffer>

QT_BEGIN_NAMESPACE

class QQuickCanvasItemPrivate : public QQuickItemPrivate
{
public:
    QQuickCanvasItemPrivate();
    ~QQuickCanvasItemPrivate();
    QQuickContext2D* context;
    QQuickContext2DTexture* texture;
    QSizeF canvasSize;
    QSize tileSize;
    QRectF canvasWindow;
    QRectF dirtyRect;
    uint renderInThread : 1;
    uint hasCanvasSize :1;
    uint hasTileSize :1;
    uint hasCanvasWindow :1;
    uint componentCompleted :1;
    QQuickCanvasItem::RenderTarget renderTarget;
    QHash<QUrl, QDeclarativePixmap*> images;
    QUrl baseUrl;
};

QQuickCanvasItemPrivate::QQuickCanvasItemPrivate()
    : QQuickItemPrivate()
    , context(0)
    , texture(0)
    , canvasSize(1, 1)
    , tileSize(1, 1)
    , renderInThread(false)
    , hasCanvasSize(false)
    , hasTileSize(false)
    , hasCanvasWindow(false)
    , componentCompleted(false)
    , renderTarget(QQuickCanvasItem::FramebufferObject)
{
}

QQuickCanvasItemPrivate::~QQuickCanvasItemPrivate()
{
    qDeleteAll(images);
}

/*!
    \qmlclass Canvas QQuickCanvasItem
    \inqmlmodule QtQuick 2
    \since QtQuick 2.0
    \brief The Canvas item provides a 2D canvas element which enables drawing via Javascript.
    \inherits Item
    \ingroup qml-basic-visual-elements

    The Canvas item allows drawing of straight and curved lines, simple and
    complex shapes, graphs, and referenced graphic images.  It can also add text, colors,
    shadows, gradients, and patterns, and do low level pixel operations. The Canvas
    output may be saved as an image file or serialized to a url.

    To define a drawing area in the Canvas item set the \c width and \c height properties.
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

    \section1 Threaded Rendering and Render Target

    The Canvas item supports two render targets: \c Canvas.Image and \c Canvas.FramebufferObject.

    The \c Canvas.Image render target is a \a QImage object.  This render target supports background
    thread rendering, allowing complex or long running painting to be executed without blocking the UI.

    The Canvas.FramebufferObject render target utilizes OpenGL hardware accelaration rather than rendering into
    system memory, which in many cases results in faster rendering.

    The default render target is Canvas.Image and the default renderInThread property is
    false.

    \section1 Tiled Canvas
    The Canvas item supports tiled rendering by setting \l canvasSize, \l tileSize
    and \l canvasWindow properties.

    Tiling allows efficient display of a very large virtual via a smaller canvas
    window. The actual memory consumption is in relatation to the canvas window size.  The painting
    code can draw within the virtual canvas without handling coordinate system transformations.

    The tiles overlapping with the canvas window may be cached eliminating the need to redraw,
    which can lead to significantly improved performance in some situations.

    \section1 Pixel Operations
    All HTML5 2D context pixel operations are supported. In order to ensure improved
    pixel reading/writing performance the \a Canvas.Image render target should be chosen. The
    \a Canvas.FramebufferObject render target requires the pixel data to be exchanged between
    the system memory and the graphic card, which is significantly more expensive.  Rendering
    may also be synchronized with the V-sync signal (to avoid {en.wikipedia.org/wiki/Screen_tearing}{screen tearing})
    which will futher impact pixel operations with \c Canvas.FrambufferObject render target.

    \section1 Tips for Porting Existing HTML5 Canvas applications

    Although the Canvas item is provides a HTML5 like API, HTML5 canvas applications
    need to be modified to run in the Canvas item:
    \list
    \o Replace all DOM API calls with QML property bindings or Canvas item methods.
    \o Replace all HTML event handlers with the \a MouseArea item.
    \o Change setInterval/setTimeout function calls with the \a Timer item.
    \o Place painting code into the \a QtQuick2::Canvas::onPaint handler and trigger
       painting by calling the \c markDirty or \c requestPaint methods.
    \o To draw images, load them by calling the Canvas's loadImage method and then request to paint
       them in the onImageLoaded handler.
    \endlist

    \sa QtQuick2::Context2D
*/

QQuickCanvasItem::QQuickCanvasItem(QQuickItem *parent)
    : QQuickItem(*(new QQuickCanvasItemPrivate), parent)
{
    setFlag(ItemHasContents);
}

QQuickCanvasItem::~QQuickCanvasItem()
{
    Q_D(QQuickCanvasItem);
    delete d->context;
}

/*!
    \qmlproperty size QtQuick2::Canvas::canvasSize
     Holds the logical canvas size that the context paints on.

     By default, the canvas size is the same size as the current canvas item size.
     By setting the canvasSize, tileSize and canvasWindow, the Canvas
     item can act as a large virtual canvas with many seperately rendered tile rectangles
     Only those tiles within the current canvas window are painted by
     the Canvas render engine.

    \sa QtQuick2::Canvas::tileSize QtQuick2::Canvas::canvasWindow
*/
QSizeF QQuickCanvasItem::canvasSize() const
{
    Q_D(const QQuickCanvasItem);
    return d->canvasSize;
}

void QQuickCanvasItem::setCanvasSize(const QSizeF & size)
{
    Q_D(QQuickCanvasItem);
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

     The Canvas item enters tiled mode by setting canvasSize, tileSize and
     the canvasWindow. This can improve rendering performance
     by rendering and caching tiles instead of rendering the whole canvas every time.

     Memory will be consumed only by those tiles within the current visible region.

     By default the tileSize is the same as the canvasSize.

     \sa QtQuick2::Canvas::canvaasSize QtQuick2::Canvas::canvasWindow
*/
QSize QQuickCanvasItem::tileSize() const
{
    Q_D(const QQuickCanvasItem);
    return d->tileSize;
}

void QQuickCanvasItem::setTileSize(const QSize & size)
{
    Q_D(QQuickCanvasItem);
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

     By default the canvasWindow size is the same as the Canvas item
     size with the topleft point as (0, 0).

     If the canvasSize is different to the Canvas item size, the Canvas
     item can display different visible areas by changing the canvas windowSize
     and/or position.

    \sa QtQuick2::Canvas::canvasSize QtQuick2::Canvas::tileSize
*/
QRectF QQuickCanvasItem::canvasWindow() const
{
    Q_D(const QQuickCanvasItem);
    return d->canvasWindow;
}

void QQuickCanvasItem::setCanvasWindow(const QRectF& rect)
{
    Q_D(QQuickCanvasItem);
    if (d->canvasWindow != rect) {
        d->canvasWindow = rect;

        d->hasCanvasWindow = true;
        emit canvasWindowChanged();
        polish();
        update();
    }
}


QQuickContext2D* QQuickCanvasItem::context() const
{
    Q_D(const QQuickCanvasItem);
    return d->context;
}
/*!
    \qmlproperty bool QtQuick2::Canvas::renderInThread
     Holds the current canvas rendering mode.

     Set renderInThread to true to render complex and long
     running painting in a dedicated background
     thread, avoiding blocking the main UI.

     \note: Not all renderTargets support background rendering.  If background rendering
     is not supported by the current renderTarget, the renderInThread
     property is ignored.

     The default value is false.
    \sa QtQuick2::Canvas::renderTarget
*/
bool QQuickCanvasItem::renderInThread() const
{
    Q_D(const QQuickCanvasItem);
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
                                   or in the main GUI thread depending upon the platform.
     \endlist

     The default render target is \c Canvas.Image.
    \sa QtQuick2::Canvas::renderInThread
*/
QQuickCanvasItem::RenderTarget QQuickCanvasItem::renderTarget() const
{
    Q_D(const QQuickCanvasItem);
    return d->renderTarget;
}

void QQuickCanvasItem::setRenderTarget(RenderTarget target)
{
    Q_D(QQuickCanvasItem);
    if (d->renderTarget != target) {
        d->renderTarget = target;

        if (d->componentCompleted)
            createTexture();
        emit renderTargetChanged();
    }
}

void QQuickCanvasItem::_doPainting(const QRectF& region)
{
    Q_D(QQuickCanvasItem);
    emit paint(QDeclarativeV8Handle::fromHandle(d->context->v8value())
             , QQuickContext2DTexture::tiledRect(region, d->tileSize));
    if (d->texture)
        d->texture->wake();
}

void QQuickCanvasItem::setRenderInThread(bool renderInThread)
{
    Q_D(QQuickCanvasItem);
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

void QQuickCanvasItem::geometryChanged(const QRectF &newGeometry,
                             const QRectF &oldGeometry)
{
    Q_D(QQuickCanvasItem);
    QQuickItem::geometryChanged(newGeometry, oldGeometry);

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

void QQuickCanvasItem::componentComplete()
{
    Q_D(QQuickCanvasItem);
    QQuickItem::componentComplete();

    if (!d->context)
        createContext();
    createTexture();

    d->baseUrl = qmlEngine(this)->contextForObject(this)->baseUrl();
    requestPaint();
    updatePolish(); //force update the canvas sizes to texture for the first time
    update();
    d->componentCompleted = true;
}

void QQuickCanvasItem::updatePolish()
{
    Q_D(QQuickCanvasItem);
    QQuickItem::updatePolish();
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

QSGNode *QQuickCanvasItem::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    Q_D(QQuickCanvasItem);
    QQuickContext2DNode *node = static_cast<QQuickContext2DNode *>(oldNode);
    if (!node)
        node = new QQuickContext2DNode(this);

    node->setTexture(d->texture);
    node->setSize(d->canvasWindow.size());
    node->update();
    return node;
}

void QQuickCanvasItem::createTexture()
{
    Q_D(QQuickCanvasItem);

    if (!d->texture
      || d->texture->threadRendering() != d->renderInThread
      || d->texture->renderTarget() != d->renderTarget) {
        if (d->texture) {
            d->texture->deleteLater();
            d->texture = 0;
        }

        if (d->renderTarget == QQuickCanvasItem::Image) {
            d->texture = new QQuickContext2DImageTexture(d->renderInThread);
        } else if (d->renderTarget == QQuickCanvasItem::FramebufferObject) {
            d->texture = new QQuickContext2DFBOTexture();
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

void QQuickCanvasItem::createContext()
{
    Q_D(QQuickCanvasItem);

    delete d->context;

    d->context = new QQuickContext2D(this);

    QV8Engine *e = QDeclarativeEnginePrivate::getV8Engine(qmlEngine(this));
    d->context->setV8Engine(e);
}

/*!
  \qmlmethod object QtQuick2::Canvas::getContext(string contextId)

  Currently, the canvas item only supports the 2D context. If the \a contextId
  parameter isn't provided or is "2d", then the QtQuick2::Context2D object is
  returned, otherwise returns an invalid value.
  */
QDeclarativeV8Handle QQuickCanvasItem::getContext(const QString &contextId)
{
    Q_D(QQuickCanvasItem);

    if (contextId.toLower() != QLatin1String("2d"))
        return QDeclarativeV8Handle::fromHandle(v8::Undefined());

    if (!d->context)
        createContext();
    return QDeclarativeV8Handle::fromHandle(d->context->v8value());
}

/*!
  \qmlmethod void QtQuick2::Canvas::markDirty(rect region)

    Mark the given \a region as dirty, so that when this region is visible
    the canvas renderer will redraw it. This will trigger the "onPaint" signal
    handler function.

    \sa QtQuick2::Canvas::paint QtQuick2::Canvas::requestPaint
  */
void QQuickCanvasItem::markDirty(const QRectF& region)
{
    Q_D(QQuickCanvasItem);
    d->dirtyRect |= region;
    if (d->componentCompleted)
        polish();
    update();
}


/*!
  \qmlmethod bool QtQuick2::Canvas::save(string filename)

   Save the current canvas content into an image file \a filename.
   The saved image format is automatically decided by the \a filename's
   suffix.

   Note: calling this method will force painting the whole canvas, not just the
   current canvas visible window.

   \sa canvasWindow canvasSize toDataURL
  */
bool QQuickCanvasItem::save(const QString &filename) const
{
    Q_D(const QQuickCanvasItem);
    QUrl url = d->baseUrl.resolved(QUrl::fromLocalFile(filename));
    return toImage().save(url.toLocalFile());
}

QImage QQuickCanvasItem::loadedImage(const QUrl& url)
{
    Q_D(QQuickCanvasItem);
    QUrl fullPathUrl = d->baseUrl.resolved(url);
    if (!d->images.contains(fullPathUrl)) {
        loadImage(url);
    }
    QDeclarativePixmap* pix = d->images.value(fullPathUrl);
    if (pix->isLoading() || pix->isError()) {
        return QImage();
    }
    return pix->image();
}

/*!
  \qmlmethod void QtQuick2::Canvas::loadImage(url image)
    Loads the given \c image asynchronously.

    When the image is ready, onImageLoaded will be emitted.
    The loaded image can be unloaded by the \a QtQuick2::Canvas::unloadImage method.

    Note: Only loaded images can be painted on the Canvas item.
  \sa QtQuick2::Canvas::unloadImage QtQuick2::Canvas::imageLoaded QtQuick2::Canvas::isImageLoaded
  \sa QtQuick2::Context2D::createImageData QtQuick2::Context2D::drawImage
  */
void QQuickCanvasItem::loadImage(const QUrl& url)
{
    Q_D(QQuickCanvasItem);
    QUrl fullPathUrl = d->baseUrl.resolved(url);
    if (!d->images.contains(fullPathUrl)) {
        QDeclarativePixmap* pix = new QDeclarativePixmap();
        d->images.insert(fullPathUrl, pix);

        pix->load(qmlEngine(this)
                , fullPathUrl
                , QDeclarativePixmap::Cache | QDeclarativePixmap::Asynchronous);
        if (pix->isLoading())
            pix->connectFinished(this, SIGNAL(imageLoaded()));
    }
}
/*!
  \qmlmethod void QtQuick2::Canvas::unloadImage(url image)
  Unloads the \c image.

  Once an image is unloaded it cannot be painted by the canvas context
  unless it is loaded again.

  \sa QtQuick2::Canvas::loadImage QtQuick2::Canvas::imageLoaded QtQuick2::Canvas::isImageLoaded
  \sa QtQuick2::Context2D::createImageData QtQuick2::Context2D::drawImage
  */
void QQuickCanvasItem::unloadImage(const QUrl& url)
{
    Q_D(QQuickCanvasItem);
    QUrl removeThis = d->baseUrl.resolved(url);
    if (d->images.contains(removeThis)) {
        delete d->images.value(removeThis);
        d->images.remove(removeThis);
    }
}

/*!
  \qmlmethod void QtQuick2::Canvas::isImageError(url image)
  Returns true if the \a image failed to load.

  \sa QtQuick2::Canvas::loadImage
  */
bool QQuickCanvasItem::isImageError(const QUrl& url) const
{
    Q_D(const QQuickCanvasItem);
    QUrl fullPathUrl = d->baseUrl.resolved(url);
    return d->images.contains(fullPathUrl)
        && d->images.value(fullPathUrl)->isError();
}

/*!
  \qmlmethod void QtQuick2::Canvas::isImageLoading(url image)
  Returns true if the \a image is currently loading.

  \sa QtQuick2::Canvas::loadImage
  */
bool QQuickCanvasItem::isImageLoading(const QUrl& url) const
{
    Q_D(const QQuickCanvasItem);
    QUrl fullPathUrl = d->baseUrl.resolved(url);
    return d->images.contains(fullPathUrl)
        && d->images.value(fullPathUrl)->isLoading();
}
/*!
  \qmlmethod void QtQuick2::Canvas::isImageLoaded(url image)
  Returns true if the \a image is sucessfully loaded and ready to use.

  \sa QtQuick2::Canvas::loadImage
  */
bool QQuickCanvasItem::isImageLoaded(const QUrl& url) const
{
    Q_D(const QQuickCanvasItem);
    QUrl fullPathUrl = d->baseUrl.resolved(url);
    return d->images.contains(fullPathUrl)
        && d->images.value(fullPathUrl)->isReady();
}

QImage QQuickCanvasItem::toImage(const QRectF& region) const
{
    Q_D(const QQuickCanvasItem);
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

   Returns a data URL for the image in the canvas.

   The default \a mimeType is "image/png".

   \sa QtQuick2::Canvas::save
  */
QString QQuickCanvasItem::toDataURL(const QString& mimeType) const
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

    This handler is called to render the \a region.

    This signal can be triggered by QtQuick2::Canvas::markdirty, QtQuick2::Canvas::requestPaint
    or by changing the current canvas window.
*/

/*!
    \qmlsignal QtQuick2::Canvas::onPainted()

    This handler is called after all context painting commands are executed and
    the Canvas has been rendered.
*/

QT_END_NAMESPACE
