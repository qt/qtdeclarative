// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls.iOS.impl
import QtQuick.Controls.impl

T.SpinBox {
    id: control

    // Note: the width of the indicators are calculated into the padding
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            contentItem.implicitWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding,
                             up.implicitIndicatorHeight, down.implicitIndicatorHeight)

    leftPadding: control.mirrored ? (up.indicator ? up.indicator.width : 0) : (down.indicator ? down.indicator.width : 0)
    rightPadding: control.mirrored ? (down.indicator ? down.indicator.width : 0) : (up.indicator ? up.indicator.width : 0)

    validator: IntValidator {
        locale: control.locale.name
        bottom: Math.min(control.from, control.to)
        top: Math.max(control.from, control.to)
    }

    contentItem: TextInput {
        z: 2
        text: control.displayText
        padding: 6

        font: control.font
        color: control.palette.text
        selectionColor: control.palette.highlight
        selectedTextColor: control.palette.highlightedText
        horizontalAlignment: Qt.AlignHCenter
        verticalAlignment: Qt.AlignVCenter

        readOnly: !control.editable
        validator: control.validator
        inputMethodHints: control.inputMethodHints
        clip: width < implicitWidth
    }

    up.indicator: NinePatchImage {
        x: control.mirrored ? 0 : control.width - width
        height: control.height
        opacity: control.up.indicator.enabled ? 1 : 0.5

        source: IOS.url + "spinbox-indicator"
        NinePatchImageSelector on source {
            states: [
                {"up": true},
                {"pressed": control.up.pressed},
                {"light": Qt.styleHints.colorScheme === Qt.Light},
                {"dark": Qt.styleHints.colorScheme === Qt.Dark}
            ]
        }
    }

    down.indicator: NinePatchImage {
        x: control.mirrored ? control.width - width : 0
        height: control.height
        opacity: control.down.indicator.enabled ? 1 : 0.5

        source: IOS.url + "spinbox-indicator"
        NinePatchImageSelector on source {
            states: [
                {"down": true},
                {"pressed": control.down.pressed},
                {"light": Qt.styleHints.colorScheme === Qt.Light},
                {"dark": Qt.styleHints.colorScheme === Qt.Dark}
            ]
        }
    }

    background: Item {
        implicitWidth: 150
        implicitHeight: children[0].implicitHeight

        NinePatchImage {
            source: IOS.url + "spinbox-background"
            width: control.background.width
            height: control.background.height
            opacity: control.enabled ? 1 : 0.5
            y: (parent.height - height) / 2
            NinePatchImageSelector on source {
                states: [
                    {"light": Qt.styleHints.colorScheme === Qt.Light},
                    {"dark": Qt.styleHints.colorScheme === Qt.Dark}
                ]
            }
        }
    }
}
