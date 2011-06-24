/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
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
};


/*!
    \internal
*/
QSGCanvasItemPrivate::QSGCanvasItemPrivate()
    : QSGPaintedItemPrivate()
    , context(0)
{
}

QSGCanvasItemPrivate::~QSGCanvasItemPrivate()
{
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

    if (d->context) {
        emit drawRegion(getContext(), QRect(0, 0, width(), height()));
        d->context->paint(painter);
        emit canvasUpdated();
    }
}

void QSGCanvasItem::componentComplete()
{
    const QMetaObject *metaObject = this->metaObject();
    int propertyCount = metaObject->propertyCount();
    int requestPaintMethod = metaObject->indexOfMethod("requestPaint()");
    for (int ii = QSGCanvasItem::staticMetaObject.propertyCount(); ii < propertyCount; ++ii) {
        QMetaProperty p = metaObject->property(ii);
        if (p.hasNotifySignal())
            QMetaObject::connect(this, p.notifySignalIndex(), this, requestPaintMethod, 0, 0);
    }
    QSGPaintedItem::componentComplete();
}


QDeclarativeV8Handle QSGCanvasItem::getContext(const QString &contextId)
{
    Q_D(QSGCanvasItem);

    if (contextId == QLatin1String("2d")) {
        if (!d->context) {
            QV8Engine *e = QDeclarativeEnginePrivate::getV8Engine(qmlEngine(this));
            d->context = new QSGContext2D(this);
            d->context->setV8Engine(e);
            connect(d->context, SIGNAL(changed()), this, SLOT(requestPaint()));
        }
        return QDeclarativeV8Handle::fromHandle(d->context->v8value());
    }
    return QDeclarativeV8Handle::fromHandle(v8::Undefined());
}

void QSGCanvasItem::requestPaint()
{
    //TODO:update(d->context->dirtyRect());
    update();
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
        return dataUrl.arg(mime).arg(ba.toBase64().constData());
    }
    return QLatin1String("data:,");
}

QT_END_NAMESPACE
