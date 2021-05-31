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

#include "qquickstyleitemspinbox.h"
#include <QtQuickTemplates2/private/qquickindicatorbutton_p.h>

QFont QQuickStyleItemSpinBox::styleFont(QQuickItem *control) const
{
    return style()->font(QStyle::CE_ComboBoxLabel, controlSize(control));
}

void QQuickStyleItemSpinBox::connectToControl() const
{
    QQuickStyleItem::connectToControl();
    auto spinbox = control<QQuickSpinBox>();
    connect(spinbox->up(), &QQuickIndicatorButton::pressedChanged, this, &QQuickStyleItem::markImageDirty);
    connect(spinbox->down(), &QQuickIndicatorButton::pressedChanged, this, &QQuickStyleItem::markImageDirty);
}

StyleItemGeometry QQuickStyleItemSpinBox::calculateGeometry()
{
    QStyleOptionSpinBox styleOption;
    initStyleOption(styleOption);
    StyleItemGeometry geometry;

    geometry.minimumSize = style()->sizeFromContents(QStyle::CT_SpinBox, &styleOption, QSize(0, 0));

    if (styleOption.subControls == QStyle::SC_SpinBoxFrame) {
        geometry.implicitSize = style()->sizeFromContents(QStyle::CT_SpinBox, &styleOption, contentSize());
        styleOption.rect = QRect(QPoint(0, 0), geometry.implicitSize);
        geometry.contentRect = style()->subControlRect(QStyle::CC_SpinBox, &styleOption, QStyle::SC_SpinBoxEditField);
        geometry.layoutRect = style()->subElementRect(QStyle::SE_SpinBoxLayoutItem, &styleOption);
        geometry.ninePatchMargins = style()->ninePatchMargins(QStyle::CC_SpinBox, &styleOption, geometry.minimumSize);
        geometry.focusFrameRadius = style()->pixelMetric(QStyle::PM_SpinBoxFocusFrameRadius, &styleOption);
    } else {
        geometry.implicitSize = geometry.minimumSize;
    }

    return geometry;
}

void QQuickStyleItemSpinBox::paintEvent(QPainter *painter) const
{
    QStyleOptionSpinBox styleOption;
    initStyleOption(styleOption);
    style()->drawComplexControl(QStyle::CC_SpinBox, &styleOption, painter);
}

void QQuickStyleItemSpinBox::initStyleOption(QStyleOptionSpinBox &styleOption) const
{
    initStyleOptionBase(styleOption);
    auto spinbox = control<QQuickSpinBox>();

    switch (m_subControl) {
    case Frame:
        styleOption.subControls = QStyle::SC_SpinBoxFrame;
        styleOption.frame = true;
        break;
    case Up:
        styleOption.subControls = (QStyle::SC_SpinBoxUp | QStyle::SC_SpinBoxDown);
        break;
    case Down:
        styleOption.subControls = QStyle::SC_SpinBoxDown;
        break;
    }

    if (spinbox->up()->isPressed()) {
        styleOption.activeSubControls = QStyle::SC_SpinBoxUp;
        styleOption.state |= QStyle::State_Sunken;
    } else if (spinbox->down()->isPressed()) {
        styleOption.activeSubControls = QStyle::SC_SpinBoxDown;
        styleOption.state |= QStyle::State_Sunken;
    }

    styleOption.buttonSymbols = QStyleOptionSpinBox::UpDownArrows;
    styleOption.stepEnabled = QStyleOptionSpinBox::StepEnabled;
}
