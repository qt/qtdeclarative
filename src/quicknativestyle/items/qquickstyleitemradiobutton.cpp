// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickstyleitemradiobutton.h"

QT_BEGIN_NAMESPACE

QFont QQuickStyleItemRadioButton::styleFont(QQuickItem *control) const
{
    return style()->font(QStyle::CE_RadioButtonLabel, controlSize(control));
}

void QQuickStyleItemRadioButton::connectToControl() const
{
    QQuickStyleItem::connectToControl();
    auto checkbox = control<QQuickRadioButton>();
    connect(checkbox, &QQuickRadioButton::downChanged, this, &QQuickStyleItem::markImageDirty);
    connect(checkbox, &QQuickRadioButton::checkedChanged, this, &QQuickStyleItem::markImageDirty);
}

StyleItemGeometry QQuickStyleItemRadioButton::calculateGeometry()
{
    QStyleOptionButton styleOption;
    initStyleOption(styleOption);
    StyleItemGeometry geometry;

    geometry.minimumSize = style()->sizeFromContents(QStyle::CT_RadioButton, &styleOption, QSize(0, 0));
    geometry.implicitSize = geometry.minimumSize;
    styleOption.rect = QRect(QPoint(0, 0), geometry.implicitSize);
    geometry.contentRect = style()->subElementRect(QStyle::SE_RadioButtonContents, &styleOption);
    geometry.layoutRect = style()->subElementRect(QStyle::SE_RadioButtonLayoutItem, &styleOption);
    geometry.ninePatchMargins = style()->ninePatchMargins(QStyle::CE_RadioButton, &styleOption, geometry.minimumSize);
    geometry.focusFrameRadius = style()->pixelMetric(QStyle::PM_RadioButtonFocusFrameRadius, &styleOption);

    return geometry;
}

void QQuickStyleItemRadioButton::paintEvent(QPainter *painter) const
{
    QStyleOptionButton styleOption;
    initStyleOption(styleOption);
    style()->drawControl(QStyle::CE_RadioButton, &styleOption, painter);
}

void QQuickStyleItemRadioButton::initStyleOption(QStyleOptionButton &styleOption) const
{
    initStyleOptionBase(styleOption);
    auto checkbox = control<QQuickRadioButton>();

    styleOption.state |= checkbox->isDown() ? QStyle::State_Sunken : QStyle::State_Raised;
    styleOption.state |= checkbox->isChecked() ? QStyle::State_On : QStyle::State_Off;
}

QT_END_NAMESPACE

#include "moc_qquickstyleitemradiobutton.cpp"
