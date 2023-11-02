// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls.impl
import QtQuick.Controls.iOS.impl

T.SwipeDelegate {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding,
                             implicitIndicatorHeight + topPadding + bottomPadding)
    padding: 7
    leftPadding: 16
    rightPadding: 16
    spacing: 14

    icon.width: 29
    icon.height: 29

    swipe.transition: Transition { SmoothedAnimation { velocity: 3; easing.type: Easing.InOutCubic } }

    contentItem: IconLabel {
        spacing: control.spacing
        mirrored: control.mirrored
        display: control.display
        alignment: control.display === IconLabel.IconOnly || control.display === IconLabel.TextUnderIcon ? Qt.AlignCenter : Qt.AlignLeft
        mnemonicEnabled: false

        icon: control.icon
        defaultIconColor: control.palette.text
        text: control.text
        font: control.font
        color: defaultIconColor
    }

    background: Rectangle {
        implicitHeight: 44
        color: Application.styleHints.colorScheme === Qt.Dark ? control.palette.light : control.palette.base
        NinePatchImage {
            property real offset: control.icon.source.toString() !== "" ? control.icon.width + control.spacing : 0
            x: control.down ? 0 : control.leftPadding + offset
            y: control.down ? -1 : 0
            height: control.height + (control.down ? 1 : 0)
            width: control.down ? control.width : control.availableWidth + control.rightPadding - offset
            source: IOS.url + "itemdelegate-background"
            NinePatchImageSelector on source {
                states: [
                    {"light": Application.styleHints.colorScheme === Qt.Light},
                    {"dark": Application.styleHints.colorScheme === Qt.Dark},
                    {"pressed": control.down}
                ]
            }
        }
    }
}
