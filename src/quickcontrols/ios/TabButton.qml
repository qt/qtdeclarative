// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls.impl
import QtQuick.Controls.iOS.impl

T.TabButton {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)
    padding: 6
    spacing: 6

    icon.width: 25
    icon.height: 25
    icon.color: checked ? control.palette.button : control.palette.dark

    display: TabButton.TextUnderIcon
    font.pointSize: 12

    contentItem: IconLabel {
        spacing: control.spacing
        mirrored: control.mirrored
        display: control.display
        alignment: Qt.AlignCenter

        icon: control.icon
        text: control.text
        font: control.font
        color: checked ? control.palette.button : control.palette.dark
        opacity: control.enabled ? 1 : 0.5
    }

    background: Item {
        implicitHeight: 49
    }
}
