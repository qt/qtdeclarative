/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls module of the Qt Toolkit.
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
import QtQuick.Controls 2.0

AbstractSpinBox {
    id: control

    implicitWidth: Math.max(background ? background.implicitWidth : 0,
                             (upButton ? upButton.implicitWidth : 0) +
                           (downButton ? downButton.implicitWidth : 0) +
                                (input ? input.implicitWidth : 0) + style.padding * 2)
    implicitHeight: Math.max(background ? background.implicitHeight : 0,
                              (upButton ? upButton.implicitHeight : 0),
                            (downButton ? downButton.implicitHeight : 0),
                                 (input ? input.implicitHeight : 0) + style.padding * 2)

    Accessible.role: Accessible.SpinBox

    input: TextInput {
        x: upButton.width
        width: parent.width - upButton.width - downButton.width
        height: parent.height

        color: style.textColor
        selectionColor: style.selectionColor
        selectedTextColor: style.selectedTextColor
        horizontalAlignment: Qt.AlignHCenter
        verticalAlignment: Qt.AlignVCenter
        Keys.forwardTo: control
    }

    validator: IntValidator { }

    upButton: Item {
        implicitWidth: 26
        height: parent.height
        x: parent.width - width

        opacity: enabled ? 1.0 : style.disabledOpacity
        clip: true
        Rectangle {
            x: -radius
            width: parent.width + radius
            height: parent.height
            radius: style.roundness
            color: Qt.tint(Qt.tint(style.accentColor,
                                   control.activeFocus ? style.focusColor : "transparent"),
                                   pressed === AbstractSpinBox.UpButton ? style.pressColor : "transparent")
        }
        Rectangle {
            x: (parent.width - width) / 2
            y: (parent.height - height) / 2
            width: parent.width / 3
            height: 2
            color: control.style.selectedTextColor
        }
        Rectangle {
            x: (parent.width - width) / 2
            y: (parent.height - height) / 2
            width: 2
            height: parent.height / 3
            color: control.style.selectedTextColor
        }
    }

    downButton: Item {
        implicitWidth: 26
        height: parent.height

        opacity: enabled ? 1.0 : style.disabledOpacity
        clip: true
        Rectangle {
            width: parent.width + radius
            height: parent.height
            radius: style.roundness
            color: Qt.tint(Qt.tint(style.accentColor,
                                   control.activeFocus ? style.focusColor : "transparent"),
                                   pressed === AbstractSpinBox.DownButton ? style.pressColor : "transparent")
        }
        Rectangle {
            x: (parent.width - width) / 2
            y: (parent.height - height) / 2
            width: parent.width / 3
            height: 2
            color: control.style.selectedTextColor
        }
    }

    background: Rectangle {
        implicitWidth: 120
        radius: style.roundness
        border.width: control.activeFocus ? 2 : 1
        border.color: control.activeFocus ? style.focusColor : style.frameColor
        opacity: enabled ? 1.0 : style.disabledOpacity
        color: input.acceptableInput ? "white" : "lightpink"
    }
}
