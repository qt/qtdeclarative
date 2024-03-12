// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.15

Item {
    id: root
    objectName: "Draggables"
    width: 640
    height: 480

    Repeater {
        model: 2

        Rectangle {
            id: ball
            objectName: "Ball " + (index + 1)
            color: dragHandler.active ? "blue" : "lightsteelblue"
            width: 80; height: 80; x: 200 + index * 200; y: 200; radius: width / 2
            onParentChanged: console.log(this + " parent " + parent)

            DragHandler {
                id: dragHandler
                objectName: "DragHandler " + (index + 1)
                cursorShape: Qt.ClosedHandCursor
            }

            Text {
                color: "white"
                anchors.centerIn: parent
                horizontalAlignment: Text.AlignHCenter
                text: ball.objectName + "\n"
                      + dragHandler.centroid.position.x.toFixed(1) + "," + dragHandler.centroid.position.y.toFixed(1) + "\n"
                      + ball.x.toFixed(1) + "," + ball.y.toFixed(1)
            }
        }
    }
}
