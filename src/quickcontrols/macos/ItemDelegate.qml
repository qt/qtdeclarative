// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls.impl

T.ItemDelegate {
    id: control

    readonly property bool __notCustomizable: true

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding,
                             implicitIndicatorHeight + topPadding + bottomPadding)

    padding: 6
    spacing: 6

    icon.width: 16
    icon.height: 16

    contentItem: IconLabel {
        text: control.text
        font: control.font
        icon: control.icon
        color: control.palette.windowText
        spacing: control.spacing
        mirrored: control.mirrored
        display: control.display
        alignment: control.display === IconLabel.IconOnly || control.display === IconLabel.TextUnderIcon
            ? Qt.AlignCenter : Qt.AlignLeft

        readonly property bool __ignoreNotCustomizable: true
    }

    background: Rectangle {
        implicitWidth: 100
        implicitHeight: 20
        color: Qt.darker(control.highlighted
            ? control.palette.highlight : control.palette.button, control.down ? 1.05 : 1)

        readonly property bool __ignoreNotCustomizable: true
    }
}
