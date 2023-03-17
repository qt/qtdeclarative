// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickbasicdial_p.h"

#include <QtCore/qmath.h>
#include <QtGui/qpainter.h>
#include <QtGui/qpainterpath.h>
#include <QtQuick/private/qquickitem_p.h>

QT_BEGIN_NAMESPACE

QQuickBasicDial::QQuickBasicDial(QQuickItem *parent) :
    QQuickPaintedItem(parent)
{
}

qreal QQuickBasicDial::progress() const
{
    return m_progress;
}

void QQuickBasicDial::setProgress(qreal progress)
{
    if (progress == m_progress)
        return;

    m_progress = progress;
    update();
}

qreal QQuickBasicDial::startAngle() const
{
    return m_startAngle;
}

void QQuickBasicDial::setStartAngle(qreal startAngle)
{
    if (startAngle == m_startAngle)
        return;

    m_startAngle = startAngle;
    update();
}

qreal QQuickBasicDial::endAngle() const
{
    return m_endAngle;
}

void QQuickBasicDial::setEndAngle(qreal endAngle)
{
    if (endAngle == m_endAngle)
        return;

    m_endAngle = endAngle;
    update();
}

QColor QQuickBasicDial::color() const
{
    return m_color;
}

void QQuickBasicDial::setColor(const QColor &color)
{
    if (color == m_color)
        return;

    m_color = color;
    update();
}

void QQuickBasicDial::paint(QPainter *painter)
{
    if (width() <= 0 || height() <= 0)
        return;

    QPen pen(m_color);
    pen.setWidth(8);
    pen.setCapStyle(Qt::FlatCap);
    painter->setPen(pen);

    const QRectF bounds = boundingRect();
    const qreal smallest = qMin(bounds.width(), bounds.height());
    QRectF rect = QRectF(pen.widthF() / 2.0 + 1, pen.widthF() / 2.0 + 1, smallest - pen.widthF() - 2, smallest - pen.widthF() - 2);
    rect.moveCenter(bounds.center());

    // Make sure the arc is aligned to whole pixels.
    if (rect.x() - int(rect.x()) > 0)
        rect.setX(qCeil(rect.x()));
    if (rect.y() - int(rect.y()) > 0)
        rect.setY(qCeil(rect.y()));
    if (rect.width() - int(rect.width()) > 0)
        rect.setWidth(qFloor(rect.width()));
    if (rect.height() - int(rect.height()) > 0)
        rect.setHeight(qFloor(rect.height()));

    painter->setRenderHint(QPainter::Antialiasing);

    const qreal startAngle = 90. - m_startAngle;
    const qreal spanAngle = m_progress * (m_startAngle - m_endAngle);
    QPainterPath path;
    path.arcMoveTo(rect, startAngle);
    path.arcTo(rect, startAngle, spanAngle);
    painter->drawPath(path);

    rect.adjust(-pen.widthF() / 2.0, -pen.widthF() / 2.0, pen.widthF() / 2.0, pen.widthF() / 2.0);
    pen.setWidth(1);
    painter->setPen(pen);

    path = QPainterPath();
    path.arcMoveTo(rect, 0);
    path.arcTo(rect, 0, 360);
    painter->drawPath(path);
}

QT_END_NAMESPACE

#include "moc_qquickbasicdial_p.cpp"
