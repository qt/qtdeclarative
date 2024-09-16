// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
//![0]
import QtQuick

Item {
    width: 200; height: 200

    Rectangle {
        anchors.centerIn: parent
        width: text.implicitWidth + 20; height: text.implicitHeight + 10
        color: "green"
        radius: 5

        Drag.dragType: Drag.Automatic
        Drag.supportedActions: Qt.CopyAction
        Drag.mimeData: {
            "text/plain": "Copied text"
        }

        Text {
            id: text
            anchors.centerIn: parent
            text: "Drag me"
        }

        DragHandler {
            id: dragHandler
            onActiveChanged:
                if (active) {
                    parent.grabToImage(function(result) {
                        parent.Drag.imageSource = result.url
                        parent.Drag.active = true
                    })
                } else {
                    parent.Drag.active = false
                }
        }
    }
}
//![0]
