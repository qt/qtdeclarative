// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.15

Rectangle {
    width: 1024; height: 600
    color: "#eee"

    function getTransformationDetails(item, pinchhandler) {
        return "\n\npinch.scale:" + pinchhandler.scale.toFixed(2)
                + "\npinch.activeScale:" + pinchhandler.activeScale.toFixed(2)
                + "\npinch.rotation:" + pinchhandler.rotation.toFixed(2)
                + "\npinch.translation:" + "(" + pinchhandler.translation.x.toFixed(2) + "," + pinchhandler.translation.y.toFixed(2) + ")"
                + "\nrect.scale: " + item.scale.toFixed(2)
                + "\nrect.rotation: " + item.rotation.toFixed(2)
                + "\nrect.position: " + "(" + item.x.toFixed(2) + "," + item.y.toFixed(2) + ")"
    }

    Rectangle {
        width: parent.width - 100; height: parent.height - 100; x: 50; y: 50
        color: "lightsteelblue"
        antialiasing: true
        scale: pinch.scale

        PinchHandler {
            id: pinch
            target: null
            minimumScale: 0.5
            maximumScale: 3
        }

        Text {
            text: "Pinch with 2 fingers to scale, rotate and translate"
                  + getTransformationDetails(parent, pinch)
        }
    }

    Rectangle {
        id: centroidIndicator
        x: pinch.centroid.scenePosition.x - radius
        y: pinch.centroid.scenePosition.y - radius
        z: 1
        visible: pinch.active
        radius: width / 2
        width: 10
        height: width
        color: "red"
    }
}
