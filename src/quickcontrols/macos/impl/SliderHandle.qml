// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

import QtQuick
import QtQuick.Templates as T
import QtQuick.NativeStyle as NativeStyle
import QtQuick.Shapes
import QtQuick.Effects

Item {
    id: handle
    implicitWidth: 22
    implicitHeight: 22

    required property bool pressed
    required property real progress
    required property int orientation
    required property bool isRangeSlider
    required property bool isLeftHandle

    readonly property color handleColor: Application.styleHints.colorScheme === Qt.Light ? "white" : "#cccdce"
    readonly property color handlePressedColor1: Application.styleHints.colorScheme === Qt.Light
                                                ? Qt.alpha("white", 0.4) : Qt.alpha("white", 0.2)
    readonly property color handlePressedColor2: Application.styleHints.colorScheme === Qt.Light
                                                ? Qt.alpha("black", 0.07) : Qt.alpha("black", 0.2)
    readonly property real refraction1: Application.styleHints.colorScheme === Qt.Light ? 0.07 : 0.1
    readonly property real refraction2: Application.styleHints.colorScheme === Qt.Light ? 0.04 : 0.06
    readonly property color accentColor: NativeStyle.StyleConstants.accentColor

    Loader {
        active: NativeStyle.StyleConstants.runningWithLiquidGlass
        x: (parent.width - width) / 2
        y: (parent.height - height) / 2
        width: parent.width + (parent.pressed ? 5 : 0)
        height: parent.height + (parent.pressed ? 5 : 0)
        rotation: (!isRangeSlider && orientation === Qt.Vertical)
                || (isRangeSlider && isLeftHandle && orientation === Qt.Horizontal)
                || (isRangeSlider && !isLeftHandle && orientation === Qt.Vertical)
                  ? 180 : 0
        readonly property real refractionStrength: (isRangeSlider && isLeftHandle) ? 1 - handle.progress : handle.progress
        Behavior on width { NumberAnimation { duration: 100 } }
        Behavior on height { NumberAnimation { duration: 100 } }
        sourceComponent: Rectangle {
            radius: width / 2
            gradient: RadialGradient {
                orientation: handle.orientation
                GradientStop { position: 0.0; color: pressed ? Qt.alpha(accentColor, refractionStrength === 0 ? 0.1 : 1) : handleColor }
                GradientStop { position: refraction1 * refractionStrength; color: pressed ? Qt.alpha(accentColor, refractionStrength === 0 ? 0.1 : 1) : handleColor }
                GradientStop { position: (refraction1 * refractionStrength) + 0.1; color: pressed ? handlePressedColor1 : handleColor }
                GradientStop { position: 1 - (refraction2 * refractionStrength) - 0.1; color: pressed ? handlePressedColor2 : handleColor }
                GradientStop { position: 1 - (refraction2 * refractionStrength); color: pressed ? accentColor : handleColor }
                GradientStop { position: 1.0; color: pressed ? Qt.alpha(accentColor, handle.progress === 0 ? 0.1 : 1) : handleColor }
            }

            border.color: Application.styleHints.colorScheme === Qt.Light ? Qt.alpha("white", 0.5) : Qt.alpha("black", 0.5)
            border.width: pressed ? 1 : 0

            Behavior on color { ColorAnimation { duration: 100 } }

            layer.enabled: true
            layer.effect: MultiEffect {
                shadowEnabled: true
                blurMax: 10
                shadowBlur: 1
                shadowScale: 1.3
                shadowOpacity: 0.05
            }
        }
    }

    Loader {
        active: !NativeStyle.StyleConstants.runningWithLiquidGlass
        x: (parent.width - width) / 2
        y: (parent.height - height) / 2
        width: parent.width
        height: parent.height
        sourceComponent: Rectangle {
            radius: width / 2
            color: {
                const light = Application.styleHints.colorScheme === Qt.Light
                if (!control.enabled)
                    return light ? palette.base : "#4a4e52";
                if (Application.styleHints.colorScheme === Qt.Light)
                    return Qt.darker(handleColor, handle.pressed ? 1.05 : 1)
                else
                    return Qt.lighter(handleColor, handle.pressed ? 1.05 : 1)
            }

            border.color: "transparent"
            border.width: 1

            layer.enabled: true
            layer.effect: MultiEffect {
                shadowEnabled: true
                blurMax: 10
                shadowBlur: 0.2
                shadowScale: 0.92
                shadowOpacity: 1
            }
        }
    }
}
