// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.12

Item {
    id: root
    objectName: "snapMode"
    width: 640
    height: 480

    Instantiator {
        model: 3
        // non-interfering, just for visual monitoring of points
        delegate: PointHandler {
            id: ph
            required property int index
            objectName: "ph" + index
            parent: root

            target: Rectangle {
                parent: root
                visible: ph.active
                x: ph.point.position.x - width / 2
                y: ph.point.position.y - height / 2
                width: 10; height: width; radius: width / 2
                color: Qt.rgba(1, 0.33 * ph.index, 1 - 0.3 * ph.index)
            }
        }
    }

    Rectangle {
        id: rect1
        objectName: "rect1"
        width: 90
        height: 100
        x: 100
        y: 100
        color: "teal"

        Rectangle {
            width: parent.width/2
            height: parent.width/2
            x: width/2
            y: -x
            color: dragHandler1.active ? "red" : "salmon"

            DragHandler {
                id: dragHandler1
                objectName: "dragHandler1"
                target: rect1
            }
        }
    }


    Rectangle {
        id: rect2
        objectName: "rect2"
        width: 90
        height: 100
        x: 200
        y: 100
        color: "teal"

        DragHandler {
            id: dragHandler2
            objectName: "dragHandler2"
            target: rect2b
        }

        Rectangle {
            id: rect2b
            width: parent.width/2
            height: parent.width/2
            anchors.horizontalCenter: parent.horizontalCenter
            y: -width/2
            color: dragHandler2.active ? "red" : "salmon"
        }
    }
}
