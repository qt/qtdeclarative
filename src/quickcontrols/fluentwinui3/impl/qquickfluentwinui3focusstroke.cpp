
// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include "qquickfluentwinui3focusstroke_p.h"
#include <QtGui/qpainter.h>
#include <QtGui/qpainterpath.h>
#include <QtQuick/private/qquickitem_p.h>
QT_BEGIN_NAMESPACE
QQuickFluentWinUI3FocusStroke::QQuickFluentWinUI3FocusStroke(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
}
void QQuickFluentWinUI3FocusStroke::paint(QPainter *painter)
{
    painter->setRenderHint(QPainter::Antialiasing);
    QPainterPath path;
    QRectF rect = boundingRect();
    path.moveTo(rect.left(), rect.top() + m_radius);
    path.lineTo(rect.left(), rect.bottom() - m_radius);
    path.arcTo(QRectF(rect.left(), rect.bottom() - 2 * m_radius, 2 * m_radius, 2 * m_radius), 180, 90);
    path.lineTo(rect.right() - m_radius, rect.bottom());
    path.arcTo(QRectF(rect.right() - 2 * m_radius, rect.bottom() - 2 * m_radius, 2 * m_radius, 2 * m_radius), 270, 90);
    path.lineTo(rect.right(), rect.top() + m_radius);
    path.lineTo(rect.right(), rect.top() + m_radius);
    path.lineTo(rect.right(), rect.top());
    path.lineTo(rect.left(), rect.top());
    painter->fillPath(path, m_color);
}
QColor QQuickFluentWinUI3FocusStroke::color() const
{
    return m_color;
}
void QQuickFluentWinUI3FocusStroke::setColor(const QColor &color)
{
    if (color == m_color)
        return;
    m_color = color;
    update();
}
int QQuickFluentWinUI3FocusStroke::radius() const
{
    return m_radius;
}
void QQuickFluentWinUI3FocusStroke::setRadius(int radius)
{
    if (m_radius == radius)
        return;
    m_radius = radius;
    update();
}
QT_END_NAMESPACE
#include "moc_qquickfluentwinui3focusstroke_p.cpp"
