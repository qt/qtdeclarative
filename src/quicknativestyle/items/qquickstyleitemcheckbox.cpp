// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickstyleitemcheckbox.h"

QT_BEGIN_NAMESPACE

QFont QQuickStyleItemCheckBox::styleFont(QQuickItem *control) const
{
    return style()->font(QStyle::CE_RadioButtonLabel, controlSize(control));
}

void QQuickStyleItemCheckBox::connectToControl() const
{
    QQuickStyleItem::connectToControl();
    auto checkbox = control<QQuickCheckBox>();
    connect(checkbox, &QQuickCheckBox::downChanged, this, &QQuickStyleItem::markImageDirty);
    connect(checkbox, &QQuickCheckBox::checkStateChanged, this, &QQuickStyleItem::markImageDirty);
}

StyleItemGeometry QQuickStyleItemCheckBox::calculateGeometry()
{
    QStyleOptionButton styleOption;
    initStyleOption(styleOption);
    StyleItemGeometry geometry;

    geometry.minimumSize = style()->sizeFromContents(QStyle::CT_CheckBox, &styleOption, QSize(0, 0));
    geometry.implicitSize = geometry.minimumSize;
    styleOption.rect = QRect(QPoint(0, 0), geometry.implicitSize);
    geometry.contentRect = style()->subElementRect(QStyle::SE_CheckBoxContents, &styleOption);
    geometry.layoutRect = style()->subElementRect(QStyle::SE_CheckBoxLayoutItem, &styleOption);
    geometry.ninePatchMargins = style()->ninePatchMargins(QStyle::CE_CheckBox, &styleOption, geometry.minimumSize);
    geometry.focusFrameRadius = style()->pixelMetric(QStyle::PM_CheckBoxFocusFrameRadius, &styleOption);

    // Spacing seems to already be baked into SE_CheckBoxContents, so ignore until needed
    //const int space = style()->pixelMetric(QStyle::PM_CheckBoxLabelSpacing, &styleOption);

    return geometry;
}

void QQuickStyleItemCheckBox::paintEvent(QPainter *painter) const
{
    QStyleOptionButton styleOption;
    initStyleOption(styleOption);
    style()->drawControl(QStyle::CE_CheckBox, &styleOption, painter);
}

void QQuickStyleItemCheckBox::initStyleOption(QStyleOptionButton &styleOption) const
{
    initStyleOptionBase(styleOption);
    auto checkbox = control<QQuickCheckBox>();

    styleOption.state |= checkbox->isDown() ? QStyle::State_Sunken : QStyle::State_Raised;
    if (checkbox->isTristate() && checkbox->checkState() == Qt::PartiallyChecked)
        styleOption.state |= QStyle::State_NoChange;
    else
        styleOption.state |= checkbox->isChecked() ? QStyle::State_On : QStyle::State_Off;
}

QT_END_NAMESPACE

#include "moc_qquickstyleitemcheckbox.cpp"
