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

#include <private/qsgcontext_p.h>
#include <private/qsgadaptationlayer_p.h>

#include <QtGui/qpainter.h>
#include <qmath.h>

QT_BEGIN_NAMESPACE

QSGImagePrivate::QSGImagePrivate()
    : fillMode(QSGImage::Stretch)
    , paintedWidth(0)
    , paintedHeight(0)
    , pixmapChanged(false)
    , hAlign(QSGImage::AlignHCenter)
    , vAlign(QSGImage::AlignVCenter)
{
}

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
}

void QSGImagePrivate::setPixmap(const QPixmap &pixmap)
{
    Q_Q(QSGImage);
    pix.setPixmap(pixmap);

    q->pixmapChange();
    status = pix.isNull() ? QSGImageBase::Null : QSGImageBase::Ready;

    q->update();
}

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

QSGTexture *QSGImage::texture() const
{
    Q_D(const QSGImage);
    QSGTexture *t = d->pix.texture(d->sceneGraphContext());
    if (t) {
        t->setFiltering(QSGItemPrivate::get(this)->smooth ? QSGTexture::Linear : QSGTexture::Nearest);
        t->setMipmapFiltering(QSGTexture::None);
        t->setHorizontalWrapMode(QSGTexture::ClampToEdge);
        t->setVerticalWrapMode(QSGTexture::ClampToEdge);
    }
    return t;
}

QSGNode *QSGImage::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    Q_D(QSGImage);

    QSGTexture *texture = d->pix.texture(d->sceneGraphContext());

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
