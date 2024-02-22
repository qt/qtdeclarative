// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.12

Item {
    width: 640
    height: 480

    Rectangle {
        width: 200
        height: 200
        color: mouseArea.pressed ? "red" : "blue"
        opacity: 0.6

        MouseArea {
            id: mouseArea
            anchors.fill: parent
        }
    }
    Rectangle {
        y: 100
        z: -1
        width: 200
        height: 200
        color: dragHandler.active ? "orange" : "green"
        opacity: 0.6

        DragHandler {
            id: dragHandler
        }
    }
}
