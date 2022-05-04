/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls 2 module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

#include "qquickstyleitemcombobox.h"

QT_BEGIN_NAMESPACE

QFont QQuickStyleItemComboBox::styleFont(QQuickItem *control) const
{
    return style()->font(QStyle::CE_PushButtonLabel, controlSize(control));
}

void QQuickStyleItemComboBox::connectToControl() const
{
    QQuickStyleItem::connectToControl();
    auto comboBox = control<QQuickComboBox>();
    connect(comboBox, &QQuickComboBox::downChanged, this, &QQuickStyleItem::markImageDirty);
}

StyleItemGeometry QQuickStyleItemComboBox::calculateGeometry()
{
    QStyleOptionComboBox styleOption;
    initStyleOption(styleOption);
    StyleItemGeometry geometry;

    geometry.minimumSize = style()->sizeFromContents(QStyle::CT_ComboBox, &styleOption, QSize(0, 0));
    geometry.implicitSize = style()->sizeFromContents(QStyle::CT_ComboBox, &styleOption, contentSize());
    styleOption.rect = QRect(QPoint(0, 0), geometry.implicitSize);
    geometry.contentRect = style()->subControlRect(QStyle::CC_ComboBox, &styleOption, QStyle::SC_ComboBoxEditField);
    geometry.layoutRect = style()->subElementRect(QStyle::SE_ComboBoxLayoutItem, &styleOption);
    geometry.ninePatchMargins = style()->ninePatchMargins(QStyle::CC_ComboBox, &styleOption, geometry.minimumSize);
    geometry.focusFrameRadius = style()->pixelMetric(QStyle::PM_ComboBoxFocusFrameRadius, &styleOption);

    return geometry;
}

void QQuickStyleItemComboBox::paintEvent(QPainter *painter) const
{
    QStyleOptionComboBox styleOption;
    initStyleOption(styleOption);
    style()->drawComplexControl(QStyle::CC_ComboBox, &styleOption, painter);
}

void QQuickStyleItemComboBox::initStyleOption(QStyleOptionComboBox &styleOption) const
{
    initStyleOptionBase(styleOption);
    auto comboBox = control<QQuickComboBox>();

    styleOption.subControls = QStyle::SC_ComboBoxArrow | QStyle::SC_ComboBoxFrame | QStyle::SC_ComboBoxEditField;
    styleOption.frame = true;
    styleOption.state |= QStyle::State_Selected;
    styleOption.editable = comboBox->isEditable();

    if (comboBox->isDown())
        styleOption.state |= QStyle::State_Sunken;
    if (!comboBox->isFlat() && !comboBox->isDown())
        styleOption.state |= QStyle::State_Raised;
}

QT_END_NAMESPACE
