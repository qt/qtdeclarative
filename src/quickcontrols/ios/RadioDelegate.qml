// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls.impl
import QtQuick.Controls.iOS.impl

T.RadioDelegate {
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
    icon.color: control.palette.text

    indicator: Image {
        x: control.mirrored ? control.leftPadding : control.width - width - control.rightPadding
        y: control.topPadding + (control.availableHeight - height) / 2
        opacity: control.enabled ? 1 : 0.5
        visible: control.checked

        source: IOS.url + "radiodelegate-indicator"
        ImageSelector on source {
            states: [
                {"light": Qt.styleHints.colorScheme === Qt.Light},
                {"dark": Qt.styleHints.colorScheme === Qt.Dark}
            ]
        }
    }

    contentItem: IconLabel {
        leftPadding: control.mirrored ? control.indicator.width + control.spacing : 0
        rightPadding: !control.mirrored ? control.indicator.width + control.spacing : 0

        spacing: control.spacing
        mirrored: control.mirrored
        display: control.display
        alignment: control.display === IconLabel.IconOnly || control.display === IconLabel.TextUnderIcon ? Qt.AlignCenter : Qt.AlignLeft

        icon: control.icon
        text: control.text
        font: control.font
        color: control.palette.text
    }

    background: Rectangle {
        implicitHeight: 44
        color: Qt.styleHints.colorScheme === Qt.Dark ? control.palette.light : control.palette.base
        NinePatchImage {
            property real offset: control.icon.source.toString() !== "" ? control.icon.width + control.spacing : 0
            x: control.down ? 0 : control.leftPadding + offset
            y: control.down ? -1 : 0
            height: control.height + (control.down ? 1 : 0)
            width: control.down ? control.width : control.availableWidth + control.rightPadding - offset
            source: IOS.url + "itemdelegate-background"
            NinePatchImageSelector on source {
                states: [
                    {"light": Qt.styleHints.colorScheme === Qt.Light},
                    {"dark": Qt.styleHints.colorScheme === Qt.Dark},
                    {"pressed": control.down}
                ]
            }
        }
    }
}
