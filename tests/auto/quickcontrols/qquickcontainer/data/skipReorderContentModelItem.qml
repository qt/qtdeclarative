// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQml.Models

Window {
    width: 640
    height: 480

    property alias navigationBar: navigationbar

    component NavigationBar: Container {
        id: container

        property alias model: repeater.model
        property alias listView: list

        implicitHeight: 20

        contentItem: Item {
            width: parent.width
            height: parent.height

            ListView {
                id: list
                model: container.contentModel
                snapMode: ListView.SnapToItem
                orientation: ListView.Horizontal
                interactive: width < contentWidth
                clip: interactive
                spacing: 20

                anchors {
                    right: parent.right
                    top: parent.top
                    bottom: parent.bottom
                    left: parent.left
                }
            }
        }

        Repeater {
            id: repeater
            delegate: Text {
                text: modelData
            }
        }
    }

    NavigationBar {
        id: navigationbar
        width: 100
        model: ["1", "2", "3", "4", "5", "6", "7", "8", "9", "10"]
    }
}
