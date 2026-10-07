// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QtQuick.Controls.impl
import QtQuick.Controls.macOS.impl
import QtQuick.NativeStyle as NativeStyle
import QtQuick.Templates as T

T.Slider {
    id: control
    readonly property Item __focusFrameTarget: handle
    readonly property Item __focusFrameStyleItem: handle

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitHandleWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitHandleHeight + topPadding + bottomPadding)

    readonly property bool __notCustomizable: true

    handle: SliderHandle {
        x: Math.round(control.horizontal
            ? control.leftPadding + (control.position * (control.availableWidth - width))
            : (control.width - width) / 2)
        y: Math.round(control.horizontal
            ? (control.height - height) / 2
            : control.leftPadding + (control.availableHeight - height - (control.position * (control.availableHeight - height + 4)) + 2))
        width: NativeStyle.StyleConstants.runningWithLiquidGlass ? (control.horizontal ? 20 : 16) : implicitWidth
        height: NativeStyle.StyleConstants.runningWithLiquidGlass ? (control.horizontal ? 16 : 20) : implicitHeight

        palette: control.palette
        pressed: control.pressed
        progress: control.position
        orientation: control.orientation
        isLeftHandle: true
        isRangeSlider: false

        readonly property bool __ignoreNotCustomizable: true
    }

    background: Item {
        implicitWidth: control.horizontal ? 90 : 24
        implicitHeight: control.horizontal ? 24 : 90

        readonly property bool __ignoreNotCustomizable: true
        readonly property int barThickness: NativeStyle.StyleConstants.runningWithLiquidGlass ? 6 : 4

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
                    ? "transparent" : Qt.lighter(control.palette.window, 1.4)
                border.color: Application.styleHints.colorScheme === Qt.Light
                    ? Qt.darker(control.palette.window, 1.06)
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
            x: control.leftPadding + Math.round(control.horizontal
                ? 0 : (parent.width - width) / 2)
            y: control.topPadding + Math.round(control.horizontal
                ? (control.height - height) / 2
                : control.height - (control.position * control.height))
            width: control.horizontal
                ? control.position * control.width
                : parent.barThickness
            height: control.horizontal
                ? parent.barThickness
                : control.position * control.height
            radius: height / 2
            color: {
                const light = Application.styleHints.colorScheme === Qt.Light
                if (!control.enabled)
                    return light ? "transparent" : Qt.lighter(control.palette.window, 1.4)
                if (Application.state !== Qt.ApplicationActive)
                    return Qt.lighter(control.palette.window, light ? 0.9 : 1.8)
                return NativeStyle.StyleConstants.accentColor
            }
        }
    }

}
