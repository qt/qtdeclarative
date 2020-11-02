/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls 2 module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquickstyleitemdial.h"

QFont QQuickStyleItemDial::styleFont(QQuickItem *control) const
{
    return style()->font(QStyle::CE_ProgressBarLabel, controlSize(control));
}

void QQuickStyleItemDial::connectToControl() const
{
    QQuickStyleItem::connectToControl();
    auto dial = control<QQuickDial>();
    connect(dial, &QQuickDial::fromChanged, this, &QQuickStyleItem::markImageDirty);
    connect(dial, &QQuickDial::toChanged, this, &QQuickStyleItem::markImageDirty);
    connect(dial, &QQuickDial::positionChanged, this, &QQuickStyleItem::markImageDirty);
    connect(dial, &QQuickDial::valueChanged, this, &QQuickStyleItem::markImageDirty);
    connect(dial, &QQuickDial::stepSizeChanged, this, &QQuickStyleItem::markImageDirty);
    connect(dial, &QQuickDial::pressedChanged, this, &QQuickStyleItem::markImageDirty);
}

StyleItemGeometry QQuickStyleItemDial::calculateGeometry()
{
    QStyleOptionSlider styleOption;
    initStyleOption(styleOption);

    StyleItemGeometry geometry;
    geometry.minimumSize = style()->sizeFromContents(QStyle::CT_Dial, &styleOption, QSize(0, 0));
    geometry.implicitSize = geometry.minimumSize;
    geometry.layoutRect = style()->subElementRect(QStyle::SE_SliderLayoutItem, &styleOption);
    geometry.ninePatchMargins = style()->ninePatchMargins(QStyle::CC_Dial, &styleOption, geometry.minimumSize);
    geometry.focusFrameRadius = style()->pixelMetric(QStyle::PM_DialFocusFrameRadius, &styleOption);

    return geometry;
}

void QQuickStyleItemDial::paintEvent(QPainter *painter) const
{
    QStyleOptionSlider styleOption;
    initStyleOption(styleOption);
    style()->drawComplexControl(QStyle::CC_Dial, &styleOption, painter);
}

void QQuickStyleItemDial::initStyleOption(QStyleOptionSlider &styleOption) const
{
    initStyleOptionBase(styleOption);
    auto dial = control<QQuickDial>();

    styleOption.subControls = QStyle::SC_SliderGroove | QStyle::SC_SliderHandle;
    styleOption.activeSubControls = QStyle::SC_None;
    styleOption.tickInterval = dial->stepSize();
    styleOption.dialWrapping = dial->wrap();
    styleOption.upsideDown = true;

    if (dial->isPressed())
        styleOption.state |= QStyle::State_Sunken;

    if (dial->stepSize() == 0) {
        styleOption.minimum = 0;
        styleOption.maximum = 10000;
        styleOption.sliderPosition = dial->position() * styleOption.maximum;
    } else {
        styleOption.minimum = dial->from();
        styleOption.maximum = dial->to();
        styleOption.sliderPosition = dial->value();
    }

    // TODO: add proper API for tickmarks
    const int index = dial->metaObject()->indexOfProperty("qqc2_style_tickPosition");
    if (index != -1) {
        const int tickPosition = dial->metaObject()->property(index).read(dial).toInt();
        styleOption.tickPosition = QStyleOptionSlider::TickPosition(tickPosition);
        if (styleOption.tickPosition != QStyleOptionSlider::NoTicks)
            styleOption.subControls |= QStyle::SC_DialTickmarks;
    }

}
