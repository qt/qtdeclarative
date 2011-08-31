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

#include <QtGui/qpainter.h>

#include "private/qsgadaptationlayer_p.h"
#include "qsgcanvasitem_p.h"
#include "qsgpainteditem_p.h"
#include "qsgcontext2d_p.h"
#include "private/qsgpainternode_p.h"
#include <qdeclarativeinfo.h>
#include "qdeclarativeengine_p.h"
#include <QtCore/QBuffer>

QT_BEGIN_NAMESPACE

class QSGCanvasItemPrivate : public QSGPaintedItemPrivate
{
public:
    QSGCanvasItemPrivate();
    ~QSGCanvasItemPrivate();
    QSGContext2D* context;
    QList<QRect> dirtyRegions;
    QRect unitedDirtyRegion;
    qreal canvasX;
    qreal canvasY;
};


/*!
    \internal
*/
QSGCanvasItemPrivate::QSGCanvasItemPrivate()
    : QSGPaintedItemPrivate()
    , context(0)
    , unitedDirtyRegion()
    , canvasX(0.)
    , canvasY(0.)
{
}

QSGCanvasItemPrivate::~QSGCanvasItemPrivate()
{
}


void QSGCanvasItem::setCanvasX(qreal x)
{
    Q_D(QSGCanvasItem);
    if (d->canvasX != x) {
        d->canvasX = x;
        emit canvasXChanged();
    }
}
void QSGCanvasItem::setCanvasY(qreal y)
{
    Q_D(QSGCanvasItem);
    if (d->canvasY != y) {
        d->canvasY = y;
        emit canvasYChanged();
    }
}

qreal QSGCanvasItem::canvasX() const
{
    Q_D(const QSGCanvasItem);
    return d->canvasX;
}
qreal QSGCanvasItem::canvasY() const
{
    Q_D(const QSGCanvasItem);
    return d->canvasY;
}
QPointF QSGCanvasItem::canvasPos() const
{
    Q_D(const QSGCanvasItem);
    return QPointF(d->canvasX, d->canvasY);
}

/*!
    Constructs a QSGCanvasItem with the given \a parent item.
 */
QSGCanvasItem::QSGCanvasItem(QSGItem *parent)
    : QSGPaintedItem(*(new QSGCanvasItemPrivate), parent)
{
}

/*!
    Destroys the QSGCanvasItem.
*/
QSGCanvasItem::~QSGCanvasItem()
{
}

void QSGCanvasItem::paint(QPainter *painter)
{
    Q_D(QSGCanvasItem);
    if (d->context && d->context->isDirty()) {
        painter->setWindow(-d->canvasX, -d->canvasY, d->width, d->height);
        painter->setViewport(0, 0, d->width, d->height);
        painter->scale(d->contentsScale, d->contentsScale);

        d->context->paint(painter);
        emit painted();
    }
}

void QSGCanvasItem::componentComplete()
{
    const QMetaObject *metaObject = this->metaObject();
    int propertyCount = metaObject->propertyCount();
    int requestPaintMethod = metaObject->indexOfMethod("requestPaint(const QRect&)");
    for (int ii = QSGCanvasItem::staticMetaObject.propertyCount(); ii < propertyCount; ++ii) {
        QMetaProperty p = metaObject->property(ii);
        if (p.hasNotifySignal())
            QMetaObject::connect(this, p.notifySignalIndex(), this, requestPaintMethod, 0, 0);
    }

    createContext();

    QSGPaintedItem::componentComplete();
}

void QSGCanvasItem::createContext()
{
    Q_D(QSGCanvasItem);

    delete d->context;

    d->context = new QSGContext2D(this);

    QV8Engine *e = QDeclarativeEnginePrivate::getV8Engine(qmlEngine(this));
    d->context->setV8Engine(e);
}

QDeclarativeV8Handle QSGCanvasItem::getContext(const QString &contextId)
{
    Q_D(QSGCanvasItem);
    Q_UNUSED(contextId);

    if (d->context)
       return QDeclarativeV8Handle::fromHandle(d->context->v8value());
    return QDeclarativeV8Handle::fromHandle(v8::Undefined());
}

void QSGCanvasItem::requestPaint(const QRect& r)
{
    Q_D(QSGCanvasItem);

    QRect  region;
    if (!r.isValid())
        region = QRect(d->canvasX, d->canvasY, d->width, d->height);
    else
        region = r;

    foreach (const QRect& rect, d->dirtyRegions) {
        if (rect.contains(region))
            return;
    }

    d->unitedDirtyRegion = d->unitedDirtyRegion.unite(region);
    d->dirtyRegions.append(region);
    polish();
    update(d->unitedDirtyRegion);
}

bool QSGCanvasItem::save(const QString &filename) const
{
    Q_D(const QSGCanvasItem);
    QSGPainterNode* node = static_cast<QSGPainterNode*>(d->paintNode);
    if (node) {
        QImage image = node->toImage();
        image.save(filename);
    }
    return false;
}

void QSGCanvasItem::updatePolish()
{
    Q_D(QSGCanvasItem);
    QDeclarativeV8Handle context = QDeclarativeV8Handle::fromHandle(d->context->v8value());

    d->context->setValid(true);

   // d->context->setTileRect(QRectF(d->canvasX, d->canvasY, d->width, d->height).intersected(QRectF(0, 0, d->width, d->height)));
   // d->context->setTileRect(QRectF(d->canvasX, d->canvasY, d->width, d->height));

    foreach (const QRect& region, d->dirtyRegions) {
        emit paint(context, region);
    }
    d->dirtyRegions.clear();

    d->context->setValid(false);

    QSGPaintedItem::updatePolish();
}

QString QSGCanvasItem::toDataURL(const QString& mimeType) const
{
    Q_D(const QSGCanvasItem);

    QSGPainterNode* node = static_cast<QSGPainterNode*>(d->paintNode);
    if (node) {
        QImage image = node->toImage();
        QByteArray ba;
        QBuffer buffer(&ba);
        buffer.open(QIODevice::WriteOnly);
        QString mime = mimeType;
        QString type;
        if (mimeType == QLatin1String("image/bmp"))
            type = QLatin1String("BMP");
        else if (mimeType == QLatin1String("image/jpeg"))
            type = QLatin1String("JPEG");
        else if (mimeType == QLatin1String("image/x-portable-pixmap"))
            type = QLatin1String("PPM");
        else if (mimeType == QLatin1String("image/tiff"))
            type = QLatin1String("TIFF");
        else if (mimeType == QLatin1String("image/xbm"))
            type = QLatin1String("XBM");
        else if (mimeType == QLatin1String("image/xpm"))
            type = QLatin1String("XPM");
        else {
            type = QLatin1String("PNG");
            mime = QLatin1String("image/png");
        }
        image.save(&buffer, type.toAscii());
        buffer.close();
        QString dataUrl = QLatin1String("data:%1;base64,%2");
        return dataUrl.arg(mime).arg(QString::fromAscii(ba.toBase64()));
    }
    return QLatin1String("data:,");
}
QT_END_NAMESPACE
