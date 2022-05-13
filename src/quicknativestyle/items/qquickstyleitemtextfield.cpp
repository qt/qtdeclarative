// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickstyleitemtextfield.h"

QT_BEGIN_NAMESPACE

QFont QQuickStyleItemTextField::styleFont(QQuickItem *control) const
{
    return style()->font(QStyle::CE_ComboBoxLabel, controlSize(control));
}

void QQuickStyleItemTextField::connectToControl() const
{
    QQuickStyleItem::connectToControl();
    auto textField = control<QQuickTextField>();
    connect(textField, &QQuickTextField::readOnlyChanged, this, &QQuickStyleItem::markImageDirty);
    connect(textField, &QQuickTextField::focusChanged, this, &QQuickStyleItem::markImageDirty);
}

StyleItemGeometry QQuickStyleItemTextField::calculateGeometry()
{
    QStyleOptionFrame styleOption;
    initStyleOption(styleOption);
    StyleItemGeometry geometry;

    geometry.minimumSize = style()->sizeFromContents(QStyle::CT_LineEdit, &styleOption, QSize(0, 0));

    // Inspired by QLineEdit::sizeHint()
    QFontMetricsF fm(styleFont(const_cast<QQuickItem*>(control<QQuickItem>())));
    const QSize sz(qCeil(fm.horizontalAdvance(QLatin1Char('x')) * 17),
                   contentSize().height());
    geometry.implicitSize = style()->sizeFromContents(QStyle::CT_LineEdit, &styleOption, sz);

    styleOption.rect = QRect(QPoint(0, 0), geometry.implicitSize);
    geometry.layoutRect = styleOption.rect;
    geometry.contentRect = style()->subElementRect(QStyle::SE_LineEditContents, &styleOption);
    geometry.ninePatchMargins = style()->ninePatchMargins(QStyle::CE_ShapedFrame, &styleOption, geometry.minimumSize);
    geometry.focusFrameRadius = style()->pixelMetric(QStyle::PM_TextFieldFocusFrameRadius, &styleOption);

    return geometry;
}

void QQuickStyleItemTextField::paintEvent(QPainter *painter) const
{
    QStyleOptionFrame styleOption;
    initStyleOption(styleOption);
    style()->drawPrimitive(QStyle::PE_PanelLineEdit, &styleOption, painter);
}

void QQuickStyleItemTextField::initStyleOption(QStyleOptionFrame &styleOption) const
{
    initStyleOptionBase(styleOption);
    auto textField = control<QQuickTextField>();

    styleOption.lineWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth, &styleOption);
    styleOption.midLineWidth = 0;
    styleOption.state |= QStyle::State_Sunken;
    if (textField->isReadOnly())
        styleOption.state |= QStyle::State_ReadOnly;
}

QT_END_NAMESPACE

#include "moc_qquickstyleitemtextfield.cpp"
