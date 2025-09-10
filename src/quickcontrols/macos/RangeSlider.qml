// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls.impl
import QtQuick.Controls.macOS.impl

T.RangeSlider {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
        Math.max(first.implicitHandleWidth, second.implicitHandleWidth) + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
        Math.max(first.implicitHandleHeight, second.implicitHandleHeight) + topPadding + bottomPadding)

    readonly property bool __notCustomizable: true

    first.handle: SwitchHandle {
        x: control.leftPadding + Math.round(control.horizontal
            ? control.first.visualPosition * (control.availableWidth - width)
            : (control.availableWidth - width) / 2)
        y: control.topPadding + Math.round(control.horizontal
            ? (control.availableHeight - height) / 2
            : control.first.visualPosition * (control.availableHeight - height))

        palette: control.palette
        down: control.first.pressed

        readonly property bool __ignoreNotCustomizable: true
    }

    second.handle: SwitchHandle {
        x: control.leftPadding + Math.round(control.horizontal
            ? control.second.visualPosition * (control.availableWidth - width)
            : (control.availableWidth - width) / 2)
        y: control.topPadding + Math.round(control.horizontal
            ? (control.availableHeight - height) / 2
            : control.second.visualPosition * (control.availableHeight - height))

        palette: control.palette
        down: control.second.pressed

        readonly property bool __ignoreNotCustomizable: true
    }

    background: Item {
        implicitWidth: control.horizontal ? 124 : 24
        implicitHeight: control.horizontal ? 24 : 124

        readonly property bool __ignoreNotCustomizable: true
        readonly property int barThickness: 4

        // Groove background.
        Rectangle {
            x: control.horizontal ? 0 : (parent.width - width) / 2
            y: control.horizontal ? (parent.height - height) / 2 : 0
            width: control.horizontal ? parent.width : parent.barThickness
            height: control.horizontal ? parent.barThickness : parent.height
            radius: height / 2
            color: control.palette.window

            Rectangle {
                width: parent.width
                height: parent.height
                radius: parent.radius
                // No border in dark mode, instead we fill.
                color: Application.styleHints.colorScheme === Qt.Light
                    ? "transparent" : Qt.lighter(control.palette.window, 1.6)
                border.color: Application.styleHints.colorScheme === Qt.Light
                    ? Qt.darker(control.palette.window, 1.1)
                    : "transparent"

                Rectangle {
                    x: 1
                    y: 1
                    width: parent.width - 2
                    height: parent.height - 2
                    radius: parent.radius
                    color: "transparent"
                    border.color: Qt.darker(control.palette.window, 1.05)
                    visible: Application.styleHints.colorScheme === Qt.Light
                }
            }
        }

        // Progress bar.
        Rectangle {
            x: control.horizontal ? control.first.position * parent.width + 3 : (parent.width - width) / 2
            y: control.horizontal ? (parent.height - height) / 2 : control.second.visualPosition * parent.height + 3
            width: control.horizontal
                ? control.second.position * parent.width - control.first.position * parent.width - parent.barThickness
                : parent.barThickness
            height: control.horizontal
                ? parent.barThickness
                : control.second.position * parent.height - control.first.position * parent.height - parent.barThickness
            color: control.palette.accent
        }
    }
}
