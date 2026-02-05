// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

import QtQuick
import QtQuick.Controls.impl
import QtQuick.Templates as T
import Qt.labs.StyleKit
import Qt.labs.StyleKit.impl

T.TabButton {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    leftPadding: styleReader.leftPadding
    topPadding: styleReader.topPadding
    rightPadding: styleReader.rightPadding
    bottomPadding: styleReader.bottomPadding
    spacing: styleReader.spacing

    icon.width: 24
    icon.height: 24
    icon.color: styleReader.text.color

    StyleKitControl.controlType: styleReader.type
    StyleKitReader {
        id: styleReader
        type: StyleKitReader.TabButton
        enabled: control.enabled
        focused: control.activeFocus
        checked: control.checked
        hovered: control.hovered || control.pressed
        pressed: control.pressed
        palette: control.palette
        font: control.font
    }

    contentItem: IconLabel {
        spacing: control.spacing
        mirrored: control.mirrored
        display: control.display

        icon: control.icon
        text: control.text
        font: styleReader.effectiveFont
        color: styleReader.text.color
        alignment: styleReader.text.alignment
    }

    background: BackgroundDelegate {
        parentControl: control
        backgroundProperties: styleReader.background
    }
}
