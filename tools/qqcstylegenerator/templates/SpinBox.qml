// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QtQuick.FigmaStyle
import QtQuick.Controls.impl
import QtQuick.Templates as T

T.SpinBox {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            contentItem.implicitWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding,
                             up.implicitIndicatorHeight, down.implicitIndicatorHeight)

    property string __controlState: [
        (down.hovered || up.hovered || down.pressed || up.pressed) && "indicator",
        (down.pressed || up.pressed) && "pressed",
        hovered && !(down.pressed || up.pressed) && "hovered",
        visualFocus && "focused",
    ].filter(Boolean).join("-") || "normal"
    readonly property var config: ConfigReader.controls.spinbox[__controlState] || {}

    property string __indicatorDownState: [
        (down.hovered || down.pressed) && "indicator",
        down.pressed && "pressed",
        hovered && !down.pressed && "hovered",
        visualFocus && "focused",
    ].filter(Boolean).join("-") || "normal"
    readonly property var indicatorDownConfig: ConfigReader.controls.spinbox[__indicatorDownState] || {}

    property string __indicatorUpState: [
        (up.hovered || up.pressed) && "indicator",
        up.pressed && "pressed",
        hovered && !up.pressed && "hovered",
        visualFocus && "focused",
    ].filter(Boolean).join("-") || "normal"
    readonly property var indicatorUpConfig: ConfigReader.controls.spinbox[__indicatorUpState] || {}

    states: [
        State {
            when: !control.enabled
            PropertyChanges {
                target: control
                __controlState: "disabled"
                __indicatorDownState: "disabled"
                __indicatorUpState: "disabled"
            }
        }
    ]

    readonly property bool mirroredIndicators: control.mirrored !== (config.mirrored || false)

    leftPadding: (config ? config.spacing + config.leftPadding : 0)
        + (mirroredIndicators
        ? (up.indicator ? up.indicator.width : 0)
        : (down.indicator ? down.indicator.width : 0))
    rightPadding: (config ? config.spacing + config.rightPadding : 0)
        + (mirroredIndicators
        ? (down.indicator ? down.indicator.width : 0)
        : (up.indicator ? up.indicator.width : 0))
    topPadding: config?.topPadding || 0
    bottomPadding: config?.bottomPadding || 0

    topInset: -config?.topInset || 0
    bottomInset: -config?.bottomInset || 0
    leftInset: -config?.leftInset || 0
    rightInset: -config?.rightInset || 0

    validator: IntValidator {
        locale: control.locale.name
        bottom: Math.min(control.from, control.to)
        top: Math.max(control.from, control.to)
    }

    contentItem: TextInput {
        clip: width < implicitWidth
        text: control.displayText
        opacity: control.enabled ? 1 : 0.3

        font: control.font
        color: control.palette.text
        selectionColor: control.palette.highlight
        selectedTextColor: control.palette.highlightedText
        horizontalAlignment: Qt.AlignHCenter
        verticalAlignment: Qt.AlignVCenter

        readOnly: !control.editable
        validator: control.validator
        inputMethodHints: control.inputMethodHints
    }

    up.indicator: Image {
        x: control.mirroredIndicators
            ? (config?.leftPadding || 0)
            : control.width - width - (config?.rightPadding || 0)
        y: (config?.topPadding || 0)
            + (control.height - (config ? config.topPadding + config.bottomPadding : 0) - height) / 2
        source: control.indicatorUpConfig.indicator_up?.export === "image"
            ? Qt.resolvedUrl("images/" + control.indicatorUpConfig.indicator_up.name)
            : ""
    }

    down.indicator: Image {
        x: control.mirroredIndicators
            ? control.width - width - (config?.rightPadding || 0)
            : (config?.leftPadding || 0)
        y: (config?.topPadding || 0)
            + (control.height - (config ? config.topPadding + config.bottomPadding : 0) - height) / 2
        source: control.indicatorDownConfig.indicator_down?.export === "image"
            ? Qt.resolvedUrl("images/" + control.indicatorDownConfig.indicator_down.name)
            : ""
    }

    background: BorderImage {
        source: control.config.background?.export === "image"
            ? Qt.resolvedUrl("images/" + control.config.background.name)
            : ""

        border {
            top: control.config.background?.topOffset || 0
            bottom: control.config.background?.bottomOffset || 0
            left: control.config.background?.leftOffset || 0
            right: control.config.background?.rightOffset || 0
        }
    }
}
