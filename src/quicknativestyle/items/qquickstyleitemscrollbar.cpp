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

#include "qquickstyleitemscrollbar.h"

QFont QQuickStyleItemScrollBar::styleFont(QQuickItem *control) const
{
    return style()->font(QStyle::CE_ProgressBarLabel, controlSize(control));
}

void QQuickStyleItemScrollBar::connectToControl() const
{
    QQuickStyleItem::connectToControl();
    auto scrollBar = control<QQuickScrollBar>();
    connect(scrollBar, &QQuickScrollBar::orientationChanged, this, &QQuickStyleItem::markImageDirty);
    connect(scrollBar, &QQuickScrollBar::pressedChanged, this, &QQuickStyleItem::markImageDirty);
}

StyleItemGeometry QQuickStyleItemScrollBar::calculateGeometry()
{
    QStyleOptionSlider styleOption;
    initStyleOption(styleOption);

    StyleItemGeometry geometry;
    geometry.minimumSize = style()->sizeFromContents(QStyle::CT_ScrollBar, &styleOption, QSize(0, 0));
    if (m_subControl == SubLine || m_subControl == AddLine) {
        // So far, we know that only the windows style uses these subcontrols,
        // so we can use hardcoded sizes...
        QSize sz(16, 17);
        if (styleOption.orientation == Qt::Vertical)
            sz.transpose();
        geometry.minimumSize = sz;
    }
    geometry.implicitSize = geometry.minimumSize;
    styleOption.rect = QRect(QPoint(0, 0), geometry.implicitSize);
    geometry.layoutRect = style()->subElementRect(QStyle::SE_ScrollBarLayoutItem, &styleOption);
    geometry.ninePatchMargins = style()->ninePatchMargins(QStyle::CC_ScrollBar, &styleOption, geometry.minimumSize);

    return geometry;
}

void QQuickStyleItemScrollBar::paintEvent(QPainter *painter) const
{
    QStyleOptionSlider styleOption;
    initStyleOption(styleOption);
    if (m_subControl == SubLine || m_subControl == AddLine) {
        QStyle::SubControl sc = m_subControl == SubLine ? QStyle::SC_ScrollBarSubLine : QStyle::SC_ScrollBarAddLine;
        QStyleOptionSlider opt = styleOption;
        opt.subControls = QStyle::SC_ScrollBarAddLine
                        | QStyle::SC_ScrollBarSubLine
                        | QStyle::SC_ScrollBarGroove;

        const qreal scale = window()->effectiveDevicePixelRatio();
        const QSize scrollBarMinSize = style()->sizeFromContents(QStyle::CT_ScrollBar, &opt, QSize(0, 0));
        const QSize sz = scrollBarMinSize * scale;
        QImage scrollBarImage(sz, QImage::Format_ARGB32_Premultiplied);
        scrollBarImage.setDevicePixelRatio(scale);
        QPainter p(&scrollBarImage);
        opt.rect = QRect(QPoint(0, 0), scrollBarMinSize);
        style()->drawComplexControl(QStyle::CC_ScrollBar, &opt, &p);
        QRect sourceImageRect = style()->subControlRect(QStyle::CC_ScrollBar, &opt, sc);
        sourceImageRect = QRect(sourceImageRect.topLeft() * scale, sourceImageRect.size() * scale);
        painter->drawImage(QPoint(0, 0), scrollBarImage, sourceImageRect);
    } else {
        style()->drawComplexControl(QStyle::CC_ScrollBar, &styleOption, painter);
    }
}

void QQuickStyleItemScrollBar::initStyleOption(QStyleOptionSlider &styleOption) const
{
    initStyleOptionBase(styleOption);
    auto scrollBar = control<QQuickScrollBar>();

    switch (m_subControl) {
    case Groove:
        styleOption.subControls = QStyle::SC_ScrollBarGroove | QStyle::SC_ScrollBarAddLine | QStyle::SC_ScrollBarSubLine;
        break;
    case Handle:
        styleOption.subControls = QStyle::SC_ScrollBarSlider;
        break;
    case AddLine:
        styleOption.subControls = QStyle::SC_ScrollBarAddLine;
        break;
    case SubLine:
        styleOption.subControls = QStyle::SC_ScrollBarSubLine;
        break;
    }

    styleOption.activeSubControls = QStyle::SC_None;
    styleOption.orientation = scrollBar->orientation();
    if (styleOption.orientation == Qt::Horizontal)
        styleOption.state |= QStyle::State_Horizontal;

    if (scrollBar->isPressed())
        styleOption.state |= QStyle::State_Sunken;

    if (m_overrideState != None) {
        // In ScrollBar.qml we fade between two versions of
        // the handle, depending on if it's hovered or not

        if (m_overrideState == AlwaysHovered) {
            styleOption.activeSubControls = (styleOption.subControls & (QStyle::SC_ScrollBarSlider | QStyle::SC_ScrollBarGroove | QStyle::SC_ScrollBarAddLine | QStyle::SC_ScrollBarSubLine));
        } else if (m_overrideState == NeverHovered) {
            styleOption.activeSubControls &= ~(styleOption.subControls & (QStyle::SC_ScrollBarSlider | QStyle::SC_ScrollBarGroove | QStyle::SC_ScrollBarAddLine | QStyle::SC_ScrollBarSubLine));
        } else if (m_overrideState  == AlwaysSunken) {
            styleOption.state |= QStyle::State_Sunken;
            styleOption.activeSubControls = (styleOption.subControls & (QStyle::SC_ScrollBarSlider | QStyle::SC_ScrollBarGroove | QStyle::SC_ScrollBarAddLine | QStyle::SC_ScrollBarSubLine));
        }
    }

    // The following values will let the handle fill 100% of the
    // groove / imageSize. But when the handle is resized by
    // QQuickScrollBar, it will end up with the correct size visually.
    styleOption.pageStep = 1000;
    styleOption.minimum = 0;
    styleOption.maximum = 1;
    styleOption.sliderValue = 0;
}
