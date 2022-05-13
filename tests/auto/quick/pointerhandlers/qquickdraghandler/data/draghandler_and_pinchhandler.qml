// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.12

Item {
    id: root
    objectName: "DragHandler_and_PinchHandler"
    width: 640
    height: 480

    Rectangle {
        id: rect
        objectName: "Rect"
        color: dragHandler.active ? "blue" : (pinchHandler.active ? "magenta" : "grey")
        width: 200; height: 200; x: 100; y: 100

        PinchHandler {
            id: pinchHandler
            objectName: "PinchHandler"
        }
        DragHandler {
            id: dragHandler
            objectName: "DragHandler"
        }

        Text {
            color: "white"
            anchors.centerIn: parent
            horizontalAlignment: Text.AlignHCenter
            text: rect.objectName + "\n"
                  + "rotation:" + rect.rotation + "\n"
                  + dragHandler.centroid.position.x.toFixed(1) + "," + dragHandler.centroid.position.y.toFixed(1)
        }
    }
}
