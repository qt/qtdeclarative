// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickstyleitemscrollbar.h"

QT_BEGIN_NAMESPACE

QQuickStyleItemScrollBar::QQuickStyleItemScrollBar(QQuickItem *parent)
    : QQuickStyleItem(parent)
{
#ifdef QT_DEBUG
    setObjectName("styleItemScrollBar");
#endif
}

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
            styleOption.state &= ~QStyle::State_Sunken;
            styleOption.activeSubControls = (styleOption.subControls & (QStyle::SC_ScrollBarSlider | QStyle::SC_ScrollBarGroove | QStyle::SC_ScrollBarAddLine | QStyle::SC_ScrollBarSubLine));
        } else if (m_overrideState == NeverHovered) {
            styleOption.state &= ~QStyle::State_Sunken;
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

QT_END_NAMESPACE

#include "moc_qquickstyleitemscrollbar.cpp"
