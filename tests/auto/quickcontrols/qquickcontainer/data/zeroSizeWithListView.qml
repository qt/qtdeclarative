// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 640
    height: 480

    property alias container: container
    property alias text1: text1
    property alias text2: text2
    property alias text3: text3

    component TextItem: Text {
        font.pointSize: 24
        width: container.width
        height: container.height
    }

    Component {
        id: textComponent
        TextItem {}
    }

    function addTextItem() {
        container.addItem(textComponent.createObject(container, { text: "      4  " }))
    }

    Item {
        id: root
        objectName: "root"

        Container {
            id: container
            anchors.fill: parent
            contentItem: ListView {
                model: container.contentModel
                snapMode: ListView.SnapOneItem
                orientation: ListView.Horizontal
            }

            TextItem {
                id: text1
                text: "1        "
            }
            TextItem {
                id: text2
                text: "  2      "
            }
            TextItem {
                id: text3
                text: "    3    "
            }
        }
    }
}
