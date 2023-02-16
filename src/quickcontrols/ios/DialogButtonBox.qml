// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls.iOS.impl

T.DialogButtonBox {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            contentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             contentHeight + topPadding + bottomPadding)

    readonly property var orientation: (count === 2 || alignment !== 0)
                                        ? ListView.Horizontal : ListView.Vertical
    readonly property bool delegatePressed: count === 2 && (itemAt(0).down || itemAt(1).down)

    spacing: orientation === ListView.Horizontal
             ? (background && background.separatorLine ? background.separatorLine.width : 0)
             : 0

    delegate: DialogButtonBoxDelegate {
        width: control.orientation === ListView.Vertical
                ? control.width : undefined
    }

    contentItem: ListView {
        clip: true
        implicitWidth: contentWidth
        implicitHeight: contentHeight
        model: control.contentModel
        spacing: control.spacing
        orientation: control.orientation
        boundsBehavior: Flickable.StopAtBounds
        snapMode: ListView.SnapToItem
    }

    background: Item {
        implicitHeight: control.orientation === ListView.Horizontal ? 44 : Math.max(contentItem.implicitHeight, 44)
        implicitWidth: 270

        readonly property NinePatchImage backgroundImage : NinePatchImage {
            parent: control.background
            width: parent.width
            height: parent.height
            rotation: control.position === DialogButtonBox.Header ? 180 : 0
            source: IOS.url + "dialogbuttonbox-delegate-vertical-last"
            NinePatchImageSelector on source {
                states: [
                    {"pressed": delegate.down},
                    {"light": Qt.styleHints.colorScheme === Qt.Light},
                    {"dark": Qt.styleHints.colorScheme === Qt.Dark},
                ]
            }
        }

        readonly property NinePatchImage separatorLine : NinePatchImage {
            parent: control.background.backgroundImage
            x: control.itemAt(0) ? control.itemAt(0).width : 0
            height: parent.height
            rotation: control.position === DialogButtonBox.Header ? 180 : 0
            visible: control.alignment === 0 && (control.count === 2 && !control.delegatePressed)

            source: IOS.url + "dialogbuttonbox-separator"
            NinePatchImageSelector on source {
                states: [
                    {"light": Qt.styleHints.colorScheme === Qt.Light},
                    {"dark": Qt.styleHints.colorScheme === Qt.Dark}
                ]
            }
        }
    }
}
