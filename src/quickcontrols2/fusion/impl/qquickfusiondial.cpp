/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls 2 module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquickfusiondial_p.h"

#include <QtGui/qpainter.h>
#include <QtGui/private/qmath_p.h>
#include <QtQuick/private/qquickpalette_p.h>
#include <QtQuick/private/qquickitem_p.h>

QT_BEGIN_NAMESPACE

QQuickFusionDial::QQuickFusionDial(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
}

bool QQuickFusionDial::highlight() const
{
    return m_highlight;
}

void QQuickFusionDial::setHighlight(bool highlight)
{
    if (m_highlight == highlight)
        return;

    m_highlight = highlight;
    update();
}

// based on QStyleHelper::drawDial()
void QQuickFusionDial::paint(QPainter *painter)
{
    const int width = QQuickItem::width();
    const int height = QQuickItem::height();
    if (width <= 0 || height <= 0 || !isVisible())
        return;

    const bool enabled = isEnabled();
    qreal r = qMin(width, height) / 2.0;
    r -= r/50;
    const qreal penSize = r/20.0;

    painter->setRenderHint(QPainter::Antialiasing);

    const qreal d_ = r / 6;
    const qreal dx = d_ + (width - 2 * r) / 2 + 1;
    const qreal dy = d_ + (height - 2 * r) / 2 + 1;

    QRectF br = QRectF(dx + 0.5, dy + 0.5,
                       int(r * 2 - 2 * d_ - 2),
                       int(r * 2 - 2 * d_ - 2));
    QColor buttonColor = QQuickItemPrivate::get(this)->palette()->button().toHsv();
    buttonColor.setHsv(buttonColor .hue(),
                       qMin(140, buttonColor .saturation()),
                       qMax(180, buttonColor.value()));

    if (enabled) {
        // Drop shadow
        qreal shadowSize = qMax(1.0, penSize/2.0);
        QRectF shadowRect= br.adjusted(-2*shadowSize, -2*shadowSize,
                                       2*shadowSize, 2*shadowSize);
        QRadialGradient shadowGradient(shadowRect.center().x(),
                                       shadowRect.center().y(), shadowRect.width()/2.0,
                                       shadowRect.center().x(), shadowRect.center().y());
        shadowGradient.setColorAt(qreal(0.91), QColor(0, 0, 0, 40));
        shadowGradient.setColorAt(qreal(1.0), Qt::transparent);
        painter->setBrush(shadowGradient);
        painter->setPen(Qt::NoPen);
        painter->translate(shadowSize, shadowSize);
        painter->drawEllipse(shadowRect);
        painter->translate(-shadowSize, -shadowSize);

        // Main gradient
        QRadialGradient gradient(br.center().x() - br.width()/3, dy,
                                 br.width()*1.3, br.center().x(),
                                 br.center().y() - br.height()/2);
        gradient.setColorAt(0, buttonColor.lighter(110));
        gradient.setColorAt(qreal(0.5), buttonColor);
        gradient.setColorAt(qreal(0.501), buttonColor.darker(102));
        gradient.setColorAt(1, buttonColor.darker(115));
        painter->setBrush(gradient);
    } else {
        painter->setBrush(Qt::NoBrush);
    }

    painter->setPen(QPen(buttonColor.darker(280)));
    painter->drawEllipse(br);
    painter->setBrush(Qt::NoBrush);
    painter->setPen(buttonColor.lighter(110));
    painter->drawEllipse(br.adjusted(1, 1, -1, -1));

    if (m_highlight) {
        QColor highlight = QQuickItemPrivate::get(this)->palette()->highlight().toHsv();
        highlight.setHsv(highlight.hue(),
                         qMin(160, highlight.saturation()),
                         qMax(230, highlight.value()));
        highlight.setAlpha(127);
        painter->setPen(QPen(highlight, 2.0));
        painter->setBrush(Qt::NoBrush);
        painter->drawEllipse(br.adjusted(-1, -1, 1, 1));
    }
}

QT_END_NAMESPACE

#include "moc_qquickfusiondial_p.cpp"
