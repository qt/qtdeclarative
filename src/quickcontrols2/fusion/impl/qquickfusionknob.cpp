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

#include "qquickfusionknob_p.h"

#include <QtCore/qmath.h>
#include <QtGui/qpainter.h>
#include <QtQuick/private/qquickpalette_p.h>
#include <QtQuick/private/qquickitem_p.h>

QT_BEGIN_NAMESPACE

QQuickFusionKnob::QQuickFusionKnob(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
    connect(this, &QQuickItem::paletteChanged, this, [this](){ update(); });
}

// extracted from QStyleHelper::drawDial()
void QQuickFusionKnob::paint(QPainter *painter)
{
    const qreal w = width();
    const qreal h = height();
    if (w <= 0 || h <= 0)
        return;

    QColor color = QQuickItemPrivate::get(this)->palette()->button();
    color.setHsv(color.hue(),
                 qMin(140, color .saturation()),
                 qMax(180, color.value()));
    color = color.lighter(104);
    color.setAlphaF(0.8f);

    const qreal sz = qMin(w, h);
    QRectF rect(0, 0, sz, sz);
    rect.moveCenter(QPointF(w / 2.0, h / 2.0));
    const QPointF center = rect.center();

    QRadialGradient gradient(center.x() + rect.width() / 2,
                             center.y() + rect.width(),
                             rect.width() * 2,
                             center.x(), center.y());
    gradient.setColorAt(1, color.darker(140));
    gradient.setColorAt(qreal(0.4), color.darker(120));
    gradient.setColorAt(0, color.darker(110));

    painter->setRenderHint(QPainter::Antialiasing);
    painter->setBrush(gradient);
    painter->setPen(QColor(255, 255, 255, 150));
    painter->drawEllipse(rect);
    painter->setPen(QColor(0, 0, 0, 80));
    painter->drawEllipse(rect.adjusted(1, 1, -1, -1));
}

QT_END_NAMESPACE

#include "moc_qquickfusionknob_p.cpp"
