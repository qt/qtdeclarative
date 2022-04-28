/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls 2 module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

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
