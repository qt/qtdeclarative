// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickstyleitembutton.h"

QT_BEGIN_NAMESPACE

QFont QQuickStyleItemButton::styleFont(QQuickItem *control) const
{
    return style()->font(QStyle::CE_PushButtonLabel, controlSize(control));
}

void QQuickStyleItemButton::connectToControl() const
{
    QQuickStyleItem::connectToControl();
    auto button = control<QQuickButton>();
    connect(button, &QQuickButton::downChanged, this, &QQuickStyleItem::markImageDirty);
    connect(button, &QQuickButton::checkedChanged, this, &QQuickStyleItem::markImageDirty);
}

StyleItemGeometry QQuickStyleItemButton::calculateGeometry()
{
    QStyleOptionButton styleOption;
    initStyleOption(styleOption);
    StyleItemGeometry geometry;

    geometry.minimumSize = style()->sizeFromContents(QStyle::CT_PushButton, &styleOption, QSize(0, 0));
    geometry.implicitSize = style()->sizeFromContents(QStyle::CT_PushButton, &styleOption, contentSize());
    styleOption.rect = QRect(QPoint(0, 0), geometry.implicitSize);
    geometry.contentRect = style()->subElementRect(QStyle::SE_PushButtonContents, &styleOption);
    geometry.layoutRect = style()->subElementRect(QStyle::SE_PushButtonLayoutItem, &styleOption);
    geometry.ninePatchMargins = style()->ninePatchMargins(QStyle::CE_PushButtonBevel, &styleOption, geometry.minimumSize);
    geometry.focusFrameRadius = style()->pixelMetric(QStyle::PM_PushButtonFocusFrameRadius, &styleOption);

    return geometry;
}

void QQuickStyleItemButton::paintEvent(QPainter *painter) const
{
    QStyleOptionButton styleOption;
    initStyleOption(styleOption);
    style()->drawControl(QStyle::CE_PushButtonBevel, &styleOption, painter);
}

void QQuickStyleItemButton::initStyleOption(QStyleOptionButton &styleOption) const
{
    initStyleOptionBase(styleOption);
    auto button = control<QQuickButton>();

    if (button->isDown())
        styleOption.state |= QStyle::State_Sunken;
    if (!button->isFlat() && !button->isDown())
        styleOption.state |= QStyle::State_Raised;
    if (button->isHighlighted() || button->isChecked())
        styleOption.state |= QStyle::State_On;
    if (button->isFlat())
        styleOption.features |= QStyleOptionButton::Flat;
}

QT_END_NAMESPACE

#include "moc_qquickstyleitembutton.cpp"
