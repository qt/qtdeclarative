// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.12

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
        opacity: (whiteRect.width-blackRect.x+whiteRect.height-blackRect.y-199)/200
        Text { color: "white"; text: "opacity: " + blackRect.opacity + "\nscale: " + blackRect.scale}
        Rectangle {
            color: "red"
            width: 6; height: 6; radius: 3
            visible: pincharea.active
            x: pincharea.centroid.position.x - radius
            y: pincharea.centroid.position.y - radius
        }

        PinchHandler {
            id: pincharea
            objectName: "pinchHandler"
            minimumScale: 0.5
            maximumScale: 4.0
            minimumRotation: 0.0
            maximumRotation: 90.0
            xAxis.maximum: 140
            yAxis.maximum: 170
            onActiveChanged: {
                whiteRect.scale = pincharea.scale
                if (active) ++activeCount
                else ++deactiveCount;
            }

            onUpdated: {
                whiteRect.scale = pincharea.scale
                //whiteRect.pointCount = pincharea.pointCount
            }
         }
     }
 }
