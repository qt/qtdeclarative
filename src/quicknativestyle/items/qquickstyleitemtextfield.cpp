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

#include "qquickstyleitemtextfield.h"

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
