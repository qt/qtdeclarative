// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.12

Rectangle {
    id: whiteRect
    property real pinchScale: -1.0
    property int activeCount : 0
    property int deactiveCount : 0
    width: 320; height: 320
    color: "white"

    PointHandler {
        id: ph1
        acceptedDevices: PointerDevice.TouchScreen
        target: Rectangle {
            parent: whiteRect
            color: "cyan"
            x: ph1.point.position.x - 3
            y: ph1.point.position.y - 3
            width: 6; height: 6; radius: 3
        }
    }

    PointHandler {
        id: ph2
        acceptedDevices: PointerDevice.TouchScreen
        target: Rectangle {
            parent: whiteRect
            color: "lightgreen"
            x: ph2.point.position.x - 3
            y: ph2.point.position.y - 3
            width: 6; height: 6; radius: 3
        }
    }

    Text {
        color: "magenta"
        z: 1
        text: "scale: " + blackRect.scale +
              "\npos: " + blackRect.x.toFixed(2) + ", " + blackRect.y.toFixed(2) +
              "\ntranslation: active " + pincharea.activeTranslation.x.toFixed(2) + ", " + pincharea.activeTranslation.y.toFixed(2) +
              "\n  persistent " + pincharea.persistentTranslation.x.toFixed(2) + ", " + pincharea.persistentTranslation.y.toFixed(2)
    }

    Rectangle {
        id: blackRect
        objectName: "blackrect"
        color: "black"
        y: 50
        x: 50
        width: 100
        height: 100

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
                whiteRect.pinchScale = pincharea.scale
                if (active) ++activeCount
                else ++deactiveCount;
            }

            onUpdated: {
                whiteRect.pinchScale = pincharea.scale
                //whiteRect.pointCount = pincharea.pointCount
            }
        }
    }

    Rectangle {
        color: "transparent"; border.color: "green"
        width: 12; height: 12; radius: 6; border.width: 2
        visible: pincharea.active
        x: pincharea.centroid.scenePosition.x - radius
        y: pincharea.centroid.scenePosition.y - radius
    }
}
