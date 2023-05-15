// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickstyleitemdial.h"

QT_BEGIN_NAMESPACE

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
    connect(dial, &QQuickDial::startAngleChanged, this, &QQuickStyleItem::markImageDirty);
    connect(dial, &QQuickDial::endAngleChanged, this, &QQuickStyleItem::markImageDirty);
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
    styleOption.startAngle = dial->startAngle();
    styleOption.endAngle = dial->endAngle();

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

QT_END_NAMESPACE

#include "moc_qquickstyleitemdial.cpp"
