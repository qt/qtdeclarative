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
    uint threadRendering : 1;
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
    , threadRendering(true)
    , hasCanvasSize(false)
    , hasTileSize(false)
    , hasCanvasWindow(false)
    , componentCompleted(false)
    , renderTarget(QSGCanvasItem::Image)
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
    \brief The Canvas item provides HTML5 canvas element compatible scripts with a resolution-dependent bitmap canvas.
    \inherits Item
    \ingroup qml-basic-visual-elements

    The canvas is used to render graphs, game graphics, or other visual images on the fly.

    \section1 Example Usage

    \section1 Thread Rendering Mode

    \section1 Tiled Canvas

    \section1 Quality and Performance

    By default, all of the drawing commands are rendered by a dedicated thread for better
    performance and avoid blocking the main GUI thread. Setting the \l threadRendering property
    to false can make the canvas rendering stay in the main GUI thread.

    \sa Context2D
*/

/*!
    Constructs a QSGCanvasItem with the given \a parent item.
 */
QSGCanvasItem::QSGCanvasItem(QSGItem *parent)
    : QSGItem(*(new QSGCanvasItemPrivate), parent)
{
    setFlag(ItemHasContents);
}

/*!
    Destroys the QSGCanvasItem.
*/
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

     By default, the canvas size is the same with the current canvas item size.
    \sa tileSize canvasWindow
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
    }
}

/*!
    \qmlproperty size QtQuick2::Canvas::tileSize
     Holds the canvas rendering tile size.

     The canvas render can improve the rendering performance
     by rendering and caching each tiles instead of rendering
     the whole canvas everytime.

     Additionally, the canvas size could be infinitely large
     because only those tiles within the current visible region
     are actually rendered.

     By default, the tile size is the same with the canvas size.
    \sa canvasSize
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
    }
}

/*!
    \qmlproperty rect QtQuick2::Canvas::canvasWindow
     Holds the current canvas visible window.

     This property is read only, a canvas window can
     be changed by changing the canvas item width, height
     or the canvas viewport properties.

     When painting on a canvas item, even the item is visible
     and focused, only the current canvas window area is actually
     rendered even the painting commands may paint shaps out of
     the canvas window.

     The canvas window size is already synchronized with the canvas item size.
    \sa width height canvasSize
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
    }
}


QSGContext2D* QSGCanvasItem::context() const
{
    Q_D(const QSGCanvasItem);
    return d->context;
}
bool QSGCanvasItem::threadRendering() const
{
    Q_D(const QSGCanvasItem);
    return d->threadRendering;
}

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
    \qmlproperty bool QtQuick2::Canvas::threadRendering
     Holds the current canvas rendering mode.

     When this property is true, all canvas painting commands
     are rendered in a background rendering thread, otherwise
     the rendering happens in the main GUI thread.

     The default threadRendering value is true.
*/
void QSGCanvasItem::setThreadRendering(bool threadRendering)
{
    Q_D(QSGCanvasItem);
    if (d->threadRendering != threadRendering) {
        d->threadRendering = threadRendering;

        if (d->componentCompleted)
            createTexture();

        if (d->threadRendering)
            connect(this, SIGNAL(painted()), SLOT(update()));
        else
            disconnect(this, SIGNAL(painted()), this, SLOT(update()));
        emit threadRenderingChanged();
        polish();
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
}

void QSGCanvasItem::componentComplete()
{
    Q_D(QSGCanvasItem);

    createContext();
    createTexture();

    markDirty(d->canvasWindow);
    QSGItem::componentComplete();

    d->baseUrl = qmlEngine(this)->contextForObject(this)->baseUrl();
    d->componentCompleted = true;
}

void QSGCanvasItem::updatePolish()
{
    Q_D(QSGCanvasItem);

    QSGItem::updatePolish();
    if (d->texture) {
        if (!d->threadRendering && d->dirtyRect.isValid())
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
      || d->texture->threadRendering() != d->threadRendering
      || d->texture->renderTarget() != d->renderTarget) {
        if (d->texture) {
            d->texture->deleteLater();
            d->texture = 0;
        }

        if (d->renderTarget == QSGCanvasItem::Image) {
            d->texture = new QSGContext2DImageTexture(d->threadRendering);
        } else if (d->renderTarget == QSGCanvasItem::FramebufferObject) {
            d->texture = new QSGContext2DFBOTexture();
        }

        if (d->threadRendering && !d->texture->supportThreadRendering()) {
            qWarning("Canvas: render target does not support thread rendering, force to non-thread rendering mode.");
            d->threadRendering = false;
            emit threadRenderingChanged();
        }

        if (d->threadRendering)
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
    Q_UNUSED(contextId);

    if (d->context)
       return QDeclarativeV8Handle::fromHandle(d->context->v8value());
    return QDeclarativeV8Handle::fromHandle(v8::Undefined());
}

/*!
  \qmlmethod void QtQuick2::Canvas::markDirty(rect region)

  Mark the given \a region as dirty, so that when this region is visible
  the canvas render will redraw it. During the rendering stage, the
  canvas renderer may emit the canvas' "paint" signal so the actual painting
  scripts can be putted into the canvas's "onPaint" function.

  \sa QtQuick2::Canvas::paint
  */
void QSGCanvasItem::markDirty(const QRectF& region)
{
    Q_D(QSGCanvasItem);
    d->dirtyRect |= region;
    polish();
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
    return toImage().save(filename);
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

void QSGCanvasItem::unloadImage(const QUrl& url)
{
    Q_D(QSGCanvasItem);
    QUrl removeThis = d->baseUrl.resolved(url);
    if (d->images.contains(removeThis)) {
        delete d->images.value(removeThis);
        d->images.remove(removeThis);
    }
}

bool QSGCanvasItem::isImageError(const QUrl& url) const
{
    Q_D(const QSGCanvasItem);
    QUrl fullPathUrl = d->baseUrl.resolved(url);
    return d->images.contains(fullPathUrl)
        && d->images.value(fullPathUrl)->isError();
}

bool QSGCanvasItem::isImageLoading(const QUrl& url) const
{
    Q_D(const QSGCanvasItem);
    QUrl fullPathUrl = d->baseUrl.resolved(url);
    return d->images.contains(fullPathUrl)
        && d->images.value(fullPathUrl)->isLoading();
}

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
   Note: calling this method will force painting the whole canvas, not the
   current canvas visible window.

   \sa canvasWindow canvasSize save
  */
QString QSGCanvasItem::toDataURL(const QString& mimeType) const
{
    QImage image = toImage();

    if (!image.isNull()) {
        QByteArray ba;
        QBuffer buffer(&ba);
        buffer.open(QIODevice::WriteOnly);
        QString mime = mimeType;
        QString type;
        if (mimeType == QLatin1Literal("image/bmp"))
            type = QLatin1Literal("BMP");
        else if (mimeType == QLatin1Literal("image/jpeg"))
            type = QLatin1Literal("JPEG");
        else if (mimeType == QLatin1Literal("image/x-portable-pixmap"))
            type = QLatin1Literal("PPM");
        else if (mimeType == QLatin1Literal("image/tiff"))
            type = QLatin1Literal("TIFF");
        else if (mimeType == QLatin1Literal("image/xbm"))
            type = QLatin1Literal("XBM");
        else if (mimeType == QLatin1Literal("image/xpm"))
            type = QLatin1Literal("XPM");
        else {
            type = QLatin1Literal("PNG");
            mime = QLatin1Literal("image/png");
        }
        image.save(&buffer, type.toAscii());
        buffer.close();
        QString dataUrl = QLatin1Literal("data:%1;base64,%2");
        return dataUrl.arg(mime).arg(QLatin1String(ba.toBase64().constData()));
    }
    return QLatin1Literal("data:,");
}
QT_END_NAMESPACE
