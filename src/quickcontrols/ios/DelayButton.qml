// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls.impl

T.DelayButton {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    bottomPadding: 4
    topPadding: 4
    rightPadding: 4
    leftPadding: 4

    icon.width: 17
    icon.height: 17

    transition: Transition {
        NumberAnimation {
            duration: control.delay * (control.pressed ? 1.0 - control.progress : 0.3 * control.progress)
        }
    }

    contentItem: IconLabel {
        spacing: control.spacing
        mirrored: control.mirrored
        display: control.display
        mnemonicEnabled: false

        icon: control.icon
        defaultIconColor: control.down ? control.palette.highlight : control.palette.button
        text: control.text
        font: control.font
        color: defaultIconColor
    }

    background: Rectangle {
        implicitWidth: 17
        implicitHeight: 10
        radius: 4

        color: control.down && control.progress === 1 ? "transparent" : control.palette.disabled.button

        Rectangle {
            width: control.progress * parent.width
            height: parent.height
            radius: 4
            color: control.down ? control.palette.highlight : control.palette.button
        }
    }
}
