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

#include "qquickstyleitemslider.h"

QFont QQuickStyleItemSlider::styleFont(QQuickItem *control) const
{
    return style()->font(QStyle::CE_ProgressBarLabel, controlSize(control));
}

void QQuickStyleItemSlider::connectToControl() const
{
    QQuickStyleItem::connectToControl();
    auto slider = control<QQuickSlider>();
    connect(slider, &QQuickSlider::fromChanged, this, &QQuickStyleItem::markImageDirty);
    connect(slider, &QQuickSlider::toChanged, this, &QQuickStyleItem::markImageDirty);
    connect(slider, &QQuickSlider::positionChanged, this, &QQuickStyleItem::markImageDirty);
    connect(slider, &QQuickSlider::valueChanged, this, &QQuickStyleItem::markImageDirty);
    connect(slider, &QQuickSlider::stepSizeChanged, this, &QQuickStyleItem::markImageDirty);
    connect(slider, &QQuickSlider::pressedChanged, this, &QQuickStyleItem::markImageDirty);
    connect(slider, &QQuickSlider::orientationChanged, this, &QQuickStyleItem::markImageDirty);
}

StyleItemGeometry QQuickStyleItemSlider::calculateGeometry()
{
    QStyleOptionSlider styleOption;
    initStyleOption(styleOption);

    StyleItemGeometry geometry;
    geometry.minimumSize = style()->sizeFromContents(QStyle::CT_Slider, &styleOption, QSize(0, 0));
    geometry.implicitSize = geometry.minimumSize;
    styleOption.rect = QRect(QPoint(0, 0), geometry.implicitSize);
    geometry.layoutRect = style()->subElementRect(QStyle::SE_SliderLayoutItem, &styleOption);
    geometry.ninePatchMargins = style()->ninePatchMargins(QStyle::CC_Slider, &styleOption, geometry.minimumSize);
    geometry.focusFrameRadius = style()->pixelMetric(QStyle::PM_SliderFocusFrameRadius, &styleOption);

    return geometry;
}

void QQuickStyleItemSlider::paintEvent(QPainter *painter) const
{
    QStyleOptionSlider styleOption;
    initStyleOption(styleOption);
    style()->drawComplexControl(QStyle::CC_Slider, &styleOption, painter);
}

void QQuickStyleItemSlider::initStyleOption(QStyleOptionSlider &styleOption) const
{
    initStyleOptionBase(styleOption);
    auto slider = control<QQuickSlider>();

    styleOption.subControls = QStyle::SC_None;
    if (m_subControl & Groove)
        styleOption.subControls |= QStyle::SC_SliderGroove;
    if (m_subControl & Handle)
        styleOption.subControls |= QStyle::SC_SliderHandle;
    styleOption.activeSubControls = QStyle::SC_None;
    styleOption.orientation = slider->orientation();

    if (slider->isPressed())
        styleOption.state |= QStyle::State_Sunken;

    qreal min = 0;
    qreal max = 1;
    if (!qFuzzyIsNull(slider->stepSize())) {
        min = slider->from();
        max = slider->to();

        // TODO: add proper API for tickmarks
        const int index = slider->metaObject()->indexOfProperty("qqc2_style_tickPosition");
        if (index != -1) {
            const int tickPosition = slider->metaObject()->property(index).read(slider).toInt();
            styleOption.tickPosition = QStyleOptionSlider::TickPosition(tickPosition);
            if (styleOption.tickPosition != QStyleOptionSlider::NoTicks)
                styleOption.subControls |= QStyle::SC_SliderTickmarks;
        }
    }

    // Since the [from, to] interval in QQuickSlider is floating point, users can
    // specify very small ranges and step sizes, (e.g. [0.., 0.25], step size 0.05).
    // Since the style operates on ints, we cannot pass these values directly to the style,
    // so we normalize all values to the range [0, 10000]
    static const qreal Scale = 10000;
    const qreal normalizeMultiplier = Scale/(max - min);
    styleOption.tickInterval = int(slider->stepSize() * normalizeMultiplier);
    styleOption.minimum = 0;
    styleOption.maximum = int(Scale);
    styleOption.sliderValue = int((slider->value() - min) * normalizeMultiplier);
    styleOption.sliderPosition = int(slider->position() * styleOption.maximum);
}
