/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Labs Controls module of the Qt Toolkit.
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

import QtQuick 2.6
import QtGraphicalEffects 1.0
import Qt.labs.templates 1.0 as T
import Qt.labs.controls.material 1.0

T.SpinBox {
    id: control

    implicitWidth: Math.max(background ? background.implicitWidth : 0,
                            contentItem.implicitWidth + 2 * padding +
                            (up.indicator ? up.indicator.implicitWidth : 0) +
                            (down.indicator ? down.indicator.implicitWidth : 0))
    implicitHeight: Math.max(contentItem.implicitHeight + topPadding + bottomPadding,
                             background ? background.implicitHeight : 0,
                             up.indicator ? up.indicator.implicitHeight : 0,
                             down.indicator ? down.indicator.implicitHeight : 0)
    baselineOffset: contentItem.y + contentItem.baselineOffset

    padding: 6
    leftPadding: 6 + (control.mirrored ? (up.indicator ? up.indicator.width : 0) : (down.indicator ? down.indicator.width : 0))
    rightPadding: 6 + (control.mirrored ? (down.indicator ? down.indicator.width : 0) : (up.indicator ? up.indicator.width : 0))

    //! [validator]
    validator: IntValidator {
        locale: control.locale.name
        bottom: Math.min(control.from, control.to)
        top: Math.max(control.from, control.to)
    }
    //! [validator]

    //! [contentItem]
    contentItem: TextInput {
        text: control.textFromValue(control.value, control.locale)

        font: control.font
        color: control.Material.primaryTextColor
        selectionColor: control.Material.textSelectionColor
        selectedTextColor: control.Material.primaryTextColor
        horizontalAlignment: Qt.AlignHCenter
        verticalAlignment: Qt.AlignVCenter

        validator: control.validator
        inputMethodHints: Qt.ImhFormattedNumbersOnly
    }
    //! [contentItem]

    //! [up.indicator]
    up.indicator: Rectangle {
        x: control.mirrored ? 0 : parent.width - width
        implicitWidth: 26
        height: parent.height
        radius: 3
        color: Qt.tint(Qt.tint(control.Material.raisedButtonColor,
                               control.activeFocus ? control.Material.raisedButtonHoverColor : "transparent"),
                               control.up.pressed ? control.Material.raisedButtonPressColor: "transparent")

        Rectangle {
            x: (parent.width - width) / 2
            y: (parent.height - height) / 2
            width: Math.min(parent.width / 3, parent.width / 3)
            height: 2
            color: control.Material.primaryTextColor
        }
        Rectangle {
            x: (parent.width - width) / 2
            y: (parent.height - height) / 2
            width: 2
            height: Math.min(parent.width / 3, parent.width / 3)
            color: control.Material.primaryTextColor
        }
    }
    //! [up.indicator]

    //! [down.indicator]
    down.indicator: Rectangle {
        x: control.mirrored ? parent.width - width : 0
        implicitWidth: 26
        height: parent.height
        radius: 3
        color: Qt.tint(Qt.tint(control.Material.raisedButtonColor,
                               control.activeFocus ? control.Material.raisedButtonHoverColor : "transparent"),
                               control.down.pressed ? control.Material.raisedButtonPressColor : "transparent")

        Rectangle {
            x: (parent.width - width) / 2
            y: (parent.height - height) / 2
            width: parent.width / 3
            height: 2
            color: control.Material.primaryTextColor
        }
    }
    //! [down.indicator]

    //! [background]
    background: Item {
        implicitWidth: 100
        implicitHeight: 26

        Rectangle {
            x: parent.width / 2 - width / 2
            y: parent.y + parent.height - height
            width: control.availableWidth
            height: control.activeFocus ? 2 : 1
            color: control.activeFocus ? control.Material.accentColor : control.Material.hintTextColor
        }
    }
    //! [background]
}
