// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Templates as T

import MyStyle

T.Switch {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    padding: 6
    horizontalPadding: padding + 2
    spacing: 6

    indicator: Rectangle {
        implicitWidth: 56
        implicitHeight: 28

        x: control.text
           ? (control.mirrored ? control.width - width - control.rightPadding : control.leftPadding)
           : control.leftPadding + (control.availableWidth - width) / 2
        y: control.topPadding + (control.availableHeight - height) / 2

        radius: 8
        color: control.down ? Qt.darker(control.MyStyle.buttonColor, 1.1) : control.MyStyle.buttonColor

        Rectangle {
            x: Math.max(0, Math.min(parent.width - width, control.visualPosition * parent.width - (width / 2)))
            y: (parent.height - height) / 2
            width: 28
            height: 28
            radius: 8
            color: Qt.lighter(control.MyStyle.buttonColor)

            Behavior on x {
                enabled: !control.down
                SmoothedAnimation { velocity: 200 }
            }
        }
    }

    contentItem: Text {
        leftPadding: control.indicator && !control.mirrored ? control.indicator.width + control.spacing : 0
        rightPadding: control.indicator && control.mirrored ? control.indicator.width + control.spacing : 0

        text: control.text
        font: control.font
        color: control.MyStyle.windowTextColor
        elide: Text.ElideRight
        verticalAlignment: Text.AlignVCenter
    }
}
