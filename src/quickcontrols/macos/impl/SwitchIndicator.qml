// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QtQuick.Templates as T

Rectangle {
    id: indicator
    implicitWidth: 38
    implicitHeight: 22
    radius: implicitHeight / 2

    required property T.AbstractButton control
    readonly property real downTintFactor: 1.05

    // For QQuickMacFocusFrame.
    readonly property real __focusFrameRadius: radius

    color: Qt.styleHints.colorScheme === Qt.Light
        ? Qt.darker(indicator.control.checked
            ? indicator.palette.accent : "#d9d6d2", indicator.control.down ? indicator.downTintFactor : 1)
        : Qt.lighter(indicator.control.checked
            ? indicator.palette.accent : "#454545", indicator.control.down ? indicator.downTintFactor : 1)

    states: [
        State {
            name: "checked"
            when: indicator.control.checked

            // Do a bit of duplication with the bindings here just so that
            // we can trigger the property change for the transition. We only
            // the ColorAnimation to happen when changing checked state.
            PropertyChanges {
                target: indicator
                color: Qt.styleHints.colorScheme === Qt.Light
                    ? indicator.control.checked ? indicator.palette.accent : "#d9d6d2"
                    : indicator.control.checked ? indicator.palette.accent : "#454545"
            }
        }
    ]

    transitions: Transition {
        ColorAnimation {
            targets: indicator
            property: "color"
            // We try to match the speed of x's SmoothedAnimation below,
            // and 17 pixels (handle travel distance) / 75 pixels a second = 0.226.
            duration: 226
            easing.type: Easing.InOutQuad
        }
    }

    // Since an equivalent to InnerShadow doesn't exist in Qt 6 (QTBUG-116161),
    // we approximate it using semi-transparent rectangle borders.
    Rectangle {
        width: parent.width
        height: parent.height
        radius: height / 2
        color: "transparent"
        border.color: Qt.styleHints.colorScheme === Qt.Light
            ? Qt.darker("#06000000", indicator.control.down ? indicator.downTintFactor : 1)
            : Qt.lighter("#1affffff", indicator.control.down ? indicator.downTintFactor : 1)

        Rectangle {
            x: 1
            y: 1
            implicitWidth: parent.width - 2
            implicitHeight: parent.height - 2
            radius: parent.radius
            color: "transparent"
            border.color: Qt.styleHints.colorScheme === Qt.Light
                ? Qt.darker("#02000000", indicator.control.down ? indicator.downTintFactor : 1)
                : Qt.lighter("#04ffffff", indicator.control.down ? indicator.downTintFactor : 1)
        }
    }

    SwitchHandle {
        id: handle
        x: Math.max(1, Math.min(parent.width - width - 1, indicator.control.visualPosition * parent.width - (width / 2)))
        y: (parent.height - height) / 2
        down: indicator.control.down

        // We have this here because we don't want this behavior for RangeSlider,
        // which also uses SwitchHandle.
        Behavior on x {
            enabled: !handle.down

            SmoothedAnimation {
                velocity: 200
            }
        }
    }
}
