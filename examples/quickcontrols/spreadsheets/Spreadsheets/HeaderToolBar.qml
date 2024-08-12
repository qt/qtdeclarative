// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    implicitHeight: 40
    implicitWidth: 40

    readonly property int __icon_size: 20

    signal helpRequested()
    signal cutRequested()
    signal copyRequested()
    signal pasteRequested()

    ToolBar {
        anchors.fill: parent

        RowLayout {
            anchors.fill: parent

            ToolButton {
                id: helpButton
                icon.source: "icons/help.svg"
                icon.width: root.__icon_size
                icon.height: root.__icon_size
                icon.color: palette.text
                onClicked: helpRequested()
                ToolTip {
                    text: qsTr("Help")
                    visible: helpButton.hovered
                }
            }

            ToolButton {
                id: cutButton
                icon.source: "icons/cut.svg"
                icon.color: palette.text
                icon.width: root.__icon_size
                icon.height: root.__icon_size
                onClicked: cutRequested()
                ToolTip {
                    text: qsTr("Cut")
                    visible: cutButton.hovered
                }
            }

            ToolButton {
                id: copyButton
                icon.source: "icons/copy.svg"
                icon.color: palette.text
                icon.width: root.__icon_size
                icon.height: root.__icon_size
                onClicked: copyRequested()
                ToolTip {
                    text: qsTr("Copy")
                    visible: copyButton.hovered
                }
            }

            ToolButton {
                id: pasteButton
                icon.source: "icons/paste.svg"
                icon.color: palette.text
                icon.width: root.__icon_size
                icon.height: root.__icon_size
                onClicked: pasteRequested()
                ToolTip {
                    text: qsTr("Paste")
                    visible: pasteButton.hovered
                }
            }

            Item { Layout.fillWidth: true }
        }
    }
}
