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

#include "qquickstyleitemgroupbox.h"

QT_BEGIN_NAMESPACE

QFont QQuickStyleItemGroupBox::styleFont(QQuickItem *control) const
{
    return style()->font(QStyle::CE_HeaderLabel, controlSize(control));
}

StyleItemGeometry QQuickStyleItemGroupBox::calculateGeometry()
{
    QStyleOptionGroupBox styleOption;
    initStyleOption(styleOption);

    StyleItemGeometry geometry;
    geometry.minimumSize = style()->sizeFromContents(QStyle::CT_GroupBox, &styleOption, QSize(0, 0));

    if (!control<QQuickGroupBox>()->title().isEmpty()) {
        // We don't draw the title, but we need to take
        // it into calculation for the control size
        styleOption.text = QStringLiteral(" ");
        styleOption.subControls |= QStyle::SC_GroupBoxLabel;
    }

    geometry.implicitSize = style()->sizeFromContents(QStyle::CT_GroupBox, &styleOption, contentSize());
    styleOption.rect.setSize(geometry.implicitSize);
    geometry.contentRect = style()->subControlRect(QStyle::CC_GroupBox, &styleOption, QStyle::SC_GroupBoxContents);
    geometry.layoutRect = style()->subElementRect(QStyle::SE_GroupBoxLayoutItem, &styleOption);
    geometry.ninePatchMargins = style()->ninePatchMargins(QStyle::CC_GroupBox, &styleOption, geometry.minimumSize);

    const QQuickStyleMargins oldGroupBoxPadding = m_groupBoxPadding;
    const QRect frame = style()->subControlRect(QStyle::CC_GroupBox, &styleOption, QStyle::SC_GroupBoxFrame);
    m_groupBoxPadding = QQuickStyleMargins(QRect(QPoint(), geometry.implicitSize), frame);
    if (m_groupBoxPadding != oldGroupBoxPadding)
        emit groupBoxPaddingChanged();

    const QPointF oldLabelPos = m_labelPos;
    m_labelPos = style()->subControlRect(QStyle::CC_GroupBox, &styleOption, QStyle::SC_GroupBoxLabel).topLeft();
    if (m_labelPos != oldLabelPos)
        emit labelPosChanged();
    return geometry;
}

void QQuickStyleItemGroupBox::paintEvent(QPainter *painter) const
{
    QStyleOptionGroupBox styleOption;
    initStyleOption(styleOption);
    style()->drawComplexControl(QStyle::CC_GroupBox, &styleOption, painter);
}

void QQuickStyleItemGroupBox::initStyleOption(QStyleOptionGroupBox &styleOption) const
{
    initStyleOptionBase(styleOption);
    styleOption.subControls = QStyle::SC_GroupBoxFrame;
    styleOption.lineWidth = 1;
}

QQuickStyleMargins QQuickStyleItemGroupBox::groupBoxPadding() const
{
    return m_groupBoxPadding;
}

QPointF QQuickStyleItemGroupBox::labelPos() const
{
    return m_labelPos;
}

QT_END_NAMESPACE
