// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickuniversalfocusrectangle_p.h"

#include <QtGui/qpixmap.h>
#include <QtGui/qpainter.h>
#include <QtGui/qpixmapcache.h>
#include <QtQuick/private/qquickitem_p.h>

QT_BEGIN_NAMESPACE

QQuickUniversalFocusRectangle::QQuickUniversalFocusRectangle(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
    QQuickItemPrivate::get(this)->setTransparentForPositioner(true);
}

void QQuickUniversalFocusRectangle::paint(QPainter *painter)
{
    if (!isVisible() || width() <= 0 || height() <= 0)
        return;

    QRect bounds = boundingRect().toAlignedRect();
    const int boundsWidth = bounds.width();
    const int boundsHeight = bounds.width();
    const QString key = QStringLiteral("qquickuniversalfocusrectangle_%1_%2").arg(QString::number(boundsWidth), QString::number(boundsHeight));

    QPixmap pixmap(boundsWidth, boundsHeight);
    if (!QPixmapCache::find(key, &pixmap)) {
        bounds.adjust(0, 0, -1, -1);
        pixmap.fill(Qt::transparent);
        QPainter p(&pixmap);

        QPen pen;
        pen.setWidth(1);
        pen.setColor(Qt::white);
        p.setPen(pen);
        p.drawRect(bounds);

        pen.setColor(Qt::black);
        pen.setDashPattern(QList<qreal>(2, 1));
        p.setPen(pen);
        p.drawRect(bounds);

        QPixmapCache::insert(key, pixmap);
    }
    painter->drawPixmap(0, 0, pixmap);
}

QT_END_NAMESPACE

#include "moc_qquickuniversalfocusrectangle_p.cpp"
