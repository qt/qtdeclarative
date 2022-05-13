// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickstyleitemslider.h"

QT_BEGIN_NAMESPACE

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

QT_END_NAMESPACE

#include "moc_qquickstyleitemslider.cpp"
