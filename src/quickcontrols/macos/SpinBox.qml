// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QtQuick.Templates as T
import QtQuick.NativeStyle as NativeStyle

T.SpinBox {
    id: control

    // Note: the width of the indicators are calculated into the padding
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            contentItem.implicitWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding,
                             up.implicitIndicatorHeight, down.implicitIndicatorHeight)

    spacing: 2
    rightPadding: up.implicitIndicatorWidth + spacing

    readonly property bool __notCustomizable: true

    validator: IntValidator {
        locale: control.locale.name
        bottom: Math.min(control.from, control.to)
        top: Math.max(control.from, control.to)
    }

    contentItem: TextField {
        text: control.displayText
        font: control.font
        color: control.palette.text
        selectionColor: control.palette.highlight
        selectedTextColor: control.palette.highlightedText
        horizontalAlignment: Qt.AlignLeft
        verticalAlignment: Qt.AlignVCenter
        implicitWidth: Math.max(100 /* from IB XCode */, contentWidth + leftPadding + rightPadding)

        topPadding: 2
        bottomPadding: 2
        leftPadding: 10
        rightPadding: 10

        readOnly: !control.editable
        validator: control.validator
        inputMethodHints: control.inputMethodHints

        readonly property bool __ignoreNotCustomizable: true
    }

    NativeStyle.SpinBox {
        id: upAndDown
        control: control
        subControl: NativeStyle.SpinBox.Up
        x: up.indicator.x
        y: up.indicator.y
        useNinePatchImage: false
    }

    up.indicator: Item {
        x: control.width - width
        y: (control.height / 2) - height
        implicitWidth: upAndDown.width
        implicitHeight: upAndDown.height / 2

        readonly property bool __ignoreNotCustomizable: true
    }

    down.indicator: Item {
        x: control.width - width
        y: up.indicator.y + upAndDown.height / 2
        implicitWidth: upAndDown.width
        implicitHeight: upAndDown.height / 2

        readonly property bool __ignoreNotCustomizable: true
    }
}
