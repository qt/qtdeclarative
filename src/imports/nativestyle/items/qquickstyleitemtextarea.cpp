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

#include "qquickstyleitemtextarea.h"

QFont QQuickStyleItemTextArea::styleFont(QQuickItem *control)
{
    return style()->font(QStyle::CE_ComboBoxLabel, controlSize(control));
}

void QQuickStyleItemTextArea::connectToControl()
{
    QQuickStyleItem::connectToControl();
    auto textArea = control<QQuickTextArea>();
    connect(textArea, &QQuickTextArea::readOnlyChanged, this, &QQuickStyleItem::markImageDirty);
    connect(textArea, &QQuickTextArea::focusChanged, this, &QQuickStyleItem::markImageDirty);
}

StyleItemGeometry QQuickStyleItemTextArea::calculateGeometry()
{
    QStyleOptionFrame styleOption;
    initStyleOption(styleOption);
    StyleItemGeometry geometry;

    // There is no CT_TextEdit in QStyle, so we "borrow" CT_LineEdit for now
    geometry.minimumSize = style()->sizeFromContents(QStyle::CT_LineEdit, &styleOption, QSize(0, 0));
    geometry.implicitSize = style()->sizeFromContents(QStyle::CT_LineEdit, &styleOption, contentSize());
    styleOption.rect = QRect(QPoint(0, 0), geometry.implicitSize);
    geometry.contentRect = style()->subElementRect(QStyle::SE_LineEditContents, &styleOption);
    geometry.ninePatchMargins = style()->ninePatchMargins(QStyle::CE_ShapedFrame, &styleOption, geometry.minimumSize);

    return geometry;
}

void QQuickStyleItemTextArea::paintEvent(QPainter *painter)
{
    QStyleOptionFrame styleOption;
    initStyleOption(styleOption);
    style()->drawPrimitive(QStyle::PE_PanelLineEdit, &styleOption, painter);
}

void QQuickStyleItemTextArea::initStyleOption(QStyleOptionFrame &styleOption)
{
    initStyleOptionBase(styleOption);
    auto textArea = control<QQuickTextArea>();

    styleOption.lineWidth = 1;
    styleOption.midLineWidth = 0;
    styleOption.state |= QStyle::State_Sunken;
    if (textArea->isReadOnly())
        styleOption.state |= QStyle::State_ReadOnly;
}
