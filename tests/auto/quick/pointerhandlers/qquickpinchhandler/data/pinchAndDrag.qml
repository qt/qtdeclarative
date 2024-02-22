// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Rectangle {
    id: whiteRect
    property real scale: -1.0
    property int activeCount : 0
    property int deactiveCount : 0
    width: 320; height: 320
    color: "white"
    Rectangle {
        id: blackRect
        objectName: "blackrect"
        color: "black"
        y: 50
        x: 50
        width: 100
        height: 100
        Text { color: "white"; text: "opacity: " + blackRect.opacity + "\nscale: " + blackRect.scale}
        Rectangle {
            color: "red"
            width: 6; height: 6; radius: 3
            visible: pincharea.active
            x: pincharea.centroid.position.x - radius
            y: pincharea.centroid.position.y - radius
        }

        DragHandler { }

        PinchHandler {
            id: pincharea
            objectName: "pinchHandler"
        }
     }
 }
