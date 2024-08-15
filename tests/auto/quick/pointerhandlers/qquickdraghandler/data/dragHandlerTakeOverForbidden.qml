// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import Test 1.0

Item {
    id: root
    width: 640
    height: 480

    Rectangle {
        id: rect
        objectName: "rect"
        width: parent.width
        height: 100
        color: "gray"

        TouchItem {
            id: touchItem
            anchors.fill: parent

            DragHandler {
                id: dragHandler
                objectName: "dragHandler"
                target: null
                grabPermissions: PointerHandler.TakeOverForbidden
            }

            Text {
                text: "Draggable Text"
                x: dragHandler.active ? dragHandler.centroid.position.x : 0
                y: dragHandler.active ? dragHandler.centroid.position.y : 0
            }
        }
    }
}

