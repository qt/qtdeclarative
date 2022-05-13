// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickstyleitemscrollviewcorner.h"

StyleItemGeometry QQuickStyleItemScrollViewCorner::calculateGeometry()
{
    QStyleOptionSlider styleOption;
    initStyleOption(styleOption);

    StyleItemGeometry geometry;

    // The size of the corner should be the width of the vertical
    // scrollbar and the height of the horizontal scrollbar.
    styleOption.orientation = Qt::Vertical;
    const auto vScrollBarWidth = style()->sizeFromContents(QStyle::CT_ScrollBar, &styleOption, QSize(0, 0)).width();
    styleOption.orientation = Qt::Horizontal;
    const auto hScrollBarHeight = style()->sizeFromContents(QStyle::CT_ScrollBar, &styleOption, QSize(0, 0)).height();

    geometry.minimumSize = QSize(vScrollBarWidth, hScrollBarHeight);
    geometry.implicitSize = geometry.minimumSize;

    return geometry;
}

void QQuickStyleItemScrollViewCorner::paintEvent(QPainter *painter) const
{
    QStyleOptionSlider styleOption;
    initStyleOption(styleOption);

    // Grab a center piece of a vertical scrollbar groove onto a QImage, and use this
    // image to draw a corner. We draw the corner by first drawing the piece as it is, and
    // then rotate it 90 degrees, clip it into a triangle, and draw it once more on top.
    // The result is that we end up with one vertical and one horizontal triangle that
    // together form a filled corner rectangle.

    styleOption.orientation = Qt::Vertical;

    const qreal scale = window()->effectiveDevicePixelRatio();
    const int grooveWidth = minimumSize().width();
    const int grooveHeight = minimumSize().height();
    const QSize scrollBarMinSize = style()->sizeFromContents(QStyle::CT_ScrollBar, &styleOption, QSize(0, 0));
    const QSize scrollBarSize = scrollBarMinSize + QSize(0, grooveHeight);
    const int hStart = scrollBarMinSize.height() / 2;
    const QRect targetImageRect(0, hStart * scale, grooveWidth * scale, grooveHeight * scale);

    QImage scrollBarImage(scrollBarSize * scale, QImage::Format_ARGB32_Premultiplied);
    scrollBarImage.setDevicePixelRatio(scale);
    scrollBarImage.fill(Qt::transparent);
    QPainter scrollBarPainter(&scrollBarImage);
    styleOption.rect = QRect(QPoint(0, 0), scrollBarSize);
    style()->drawComplexControl(QStyle::CC_ScrollBar, &styleOption, &scrollBarPainter);

    // Draw vertical groove
    painter->drawImage(QPoint(0, 0), scrollBarImage, targetImageRect);

    QPainterPath path;
    path.moveTo(0, 0);
    path.lineTo(0, grooveHeight);
    path.lineTo(grooveWidth, grooveHeight);
    path.closeSubpath();

    QTransform transform;
    transform.translate(grooveWidth, 0);
    transform.rotate(90);

    painter->save();
    painter->setCompositionMode(QPainter::CompositionMode_Source);
    painter->setClipPath(path);
    painter->setTransform(transform);
    // Draw horizontal groove, clipped to a triangle
    painter->drawImage(QPoint(0, 0), scrollBarImage, targetImageRect);
    painter->restore();
}

void QQuickStyleItemScrollViewCorner::initStyleOption(QStyleOptionSlider &styleOption) const
{
    initStyleOptionBase(styleOption);

    styleOption.subControls = QStyle::SC_ScrollBarGroove;
    styleOption.activeSubControls = QStyle::SC_None;
    styleOption.pageStep = 1000;
    styleOption.minimum = 0;
    styleOption.maximum = 1;
    styleOption.sliderValue = 0;
}
