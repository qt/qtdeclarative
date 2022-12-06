// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Item {
    width: 640
    height: 480

    Rectangle {
        id: map
        color: "aqua"
        x: (parent.width - width) / 2
        y: (parent.height - height) / 2
        width: image.width
        height: image.height
        transform: Rotation {
            id: tilt
            origin.x: width / 2
            origin.y: height / 2
            axis { x: 1; y: 0; z: 0 }
            angle: tiltHandler.persistentTranslation.y / -2
        }

        WheelHandler {
            id: wheelHandler
            objectName: "vertical mouse wheel for scaling"
            property: "scale"
            onWheel: function(event) {
                console.log("rotation " + event.angleDelta + " scaled " + rotation + " @ " + point.position + " => " + map.scale)
            }
        }

        WheelHandler {
            id: horizontalWheelHandler
            objectName: "horizontal mouse wheel for side-scrolling"
            property: "x"
            orientation: Qt.Horizontal
        }

        Image {
            id: image
            anchors.centerIn: parent
            fillMode: Image.PreserveAspectFit
            source: "images/map.svgz"
            Component.onCompleted: { width = implicitWidth; height = implicitHeight }
        }

        Text {
            anchors.centerIn: parent
            text: image.sourceSize.width + " x " + image.sourceSize.height +
                  " scale " + map.scale.toFixed(2) + " active scale " + pinch.activeScale.toFixed(2)
        }
    }

    DragHandler {
        objectName: "single-point drag"
        target: map
    }

    DragHandler {
        id: tiltHandler
        objectName: "two-point tilt"
        minimumPointCount: 2
        maximumPointCount: 2
        xAxis.enabled: false
        target: null
    }

    PinchHandler {
        id: pinch
        objectName: "two-point pinch"
        target: map
        minimumScale: 0.1
        maximumScale: 10
        onActiveChanged: if (!active) reRenderIfNecessary()
        grabPermissions: PinchHandler.TakeOverForbidden // don't allow takeover if pinch has started
    }

    function reRenderIfNecessary() {
        var newSourceWidth = image.sourceSize.width * pinch.scale
        var ratio = newSourceWidth / image.sourceSize.width
        if (ratio > 1.1 || ratio < 0.9)
            image.sourceSize.width = newSourceWidth
    }
}
