// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Controls.iOS.impl

Button {
    id: delegate

    implicitHeight: 44

    readonly property var dialogButtonBox: DialogButtonBox.buttonBox
    readonly property bool hasVerticalLayout: dialogButtonBox && (dialogButtonBox.count !== 2)
    readonly property bool isLastItem: dialogButtonBox
                                       && (dialogButtonBox.contentChildren)
                                       && (dialogButtonBox.itemAt(count - 1) == delegate)
    readonly property int count: dialogButtonBox ? dialogButtonBox.count : 0

    flat: true

    contentItem: IconLabel {
        readonly property var redColor: Application.styleHints.colorScheme === Qt.Light ? "#ff3b30" : "#ff453a"
        text: delegate.text
        font: delegate.font
        spacing: delegate.spacing
        color: delegate.DialogButtonBox.buttonRole === DialogButtonBox.DestructiveRole
                ? redColor
                : (delegate.down ? delegate.palette.highlight : delegate.palette.button)
    }

    background: Item {
        // The assets below only support the typical iOS Dialog look with buttons
        // positioned at the bottom and that fill the entire width of the parent.
        // Don't draw a background if these conditions are not met
        visible: delegate.dialogButtonBox
                  && delegate.dialogButtonBox.position === DialogButtonBox.Footer
                  && delegate.dialogButtonBox.alignment === 0
        implicitHeight: 44
        readonly property bool leftItem: !delegate.hasVerticalLayout && !delegate.isLastItem
        readonly property bool rightItem: !delegate.hasVerticalLayout && !leftItem
        readonly property bool flip: delegate.mirrored ? leftItem : rightItem

        NinePatchImage {
            transform: [
                // flip
                Translate { x: (!delegate.background.flip ? 0 : -width) },
                Scale { xScale: (!delegate.background.flip ? 1 : -1) }
            ]
            width: parent.width
            height: parent.height
            source: IOS.url + "dialogbuttonbox-delegate"
            NinePatchImageSelector on source {
                states: [
                    {"horizontal": !delegate.hasVerticalLayout},
                    {"vertical": delegate.hasVerticalLayout},
                    {"last": delegate.hasVerticalLayout && delegate.isLastItem},
                    {"pressed": delegate.down},
                    {"light": Application.styleHints.colorScheme === Qt.Light},
                    {"dark": Application.styleHints.colorScheme === Qt.Dark},
                ]
            }
        }


    }
}
