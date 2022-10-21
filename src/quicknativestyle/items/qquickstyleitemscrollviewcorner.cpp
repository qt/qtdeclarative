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
