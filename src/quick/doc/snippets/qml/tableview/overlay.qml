// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Window

Window {
    width: 480
    height: 640
    visible: true
    visibility: Window.AutomaticVisibility

    TableView {
        id: tableView
        anchors.fill: parent
        clip: true
    }

    //![0]
    Rectangle {
        id: overlay
        width: 20
        height: 20
        radius: 10
        color: "blue"

        z: 10
        parent: tableView.contentItem

        Connections {
            target: tableView
            function onLayoutChanged() {
                let item = tableView.itemAtCell(5, 5)
                let insideViewport = item !== null

                overlay.visible = insideViewport
                if (insideViewport) {
                    overlay.x = item.x
                    overlay.y = item.y
                }
            }
        }
    }
    //![0]

}
