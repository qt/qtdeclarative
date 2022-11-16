// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import "components"

Rectangle {
    width: 1024; height: 600
    color: "#eee"

    function getTransformationDetails(item, pinchhandler) {
        return "\n\npinch.scale:" + pinchhandler.scale.toFixed(2)
        + "\npinch.rotation:" + pinchhandler.rotation.toFixed(2)
        + "\npinch.translation:" + "(" + pinchhandler.translation.x.toFixed(2) + "," + pinchhandler.translation.y.toFixed(2) + ")"
        + "\nrect.scale: " + item.scale.toFixed(2)
        + "\nrect.rotation: " + item.rotation.toFixed(2)
        + "\nrect.position: " + "(" + item.x.toFixed(2) + "," + item.y.toFixed(2) + ")"
    }

    function activePincher() {
        if (grandparentPinch.active)
            return grandparentPinch
        else if (parentPinch.active)
            return parentPinch
        else if (pinch2.active)
            return pinch2
        return pinch3       // always return a pinch handler, even when its inactive. The indicator will be invisble anyway.
    }

    Rectangle {
        width: parent.width - 100; height: parent.height - 100; x: 50; y: 50
        color: "beige"
        border.width: grandparentPinch.active ? 2 : 0
        border.color: border.width > 0 ? "red" : "transparent"
        antialiasing: true

        PinchHandler {
            id: grandparentPinch
            objectName: "grandparent pinch"
            minimumScale: 0.5
            maximumScale: 3
            minimumPointCount: 3
            maximumPointCount: 6 // mutants are allowed; using both hands is not normal for a pinch gesture, but we can't tell
        }

        Text {
            text: "Pinch with 3 or more fingers to scale, rotate and translate"
                  + getTransformationDetails(parent, grandparentPinch)
        }

        Rectangle {
            width: parent.width - 100; height: parent.height - 100; x: 50; y: 50
            color: "#ffe0e0e0"
            antialiasing: true

            PinchHandler {
                id: parentPinch
                objectName: "parent pinch"
                minimumScale: 0.5
                maximumScale: 3
            }

            Text {
                text: "Pinch with 2 fingers to scale, rotate and translate"
                      + getTransformationDetails(parent, parentPinch)
            }

            Rectangle {
                id: rect2
                width: 400
                height: 300
                color: "lightsteelblue"
                antialiasing: true
                x: 100
                y: 200
                rotation: 30
                transformOrigin: Item.TopRight
                border.width: (lsbDragHandler.active || pinch2.active) ? 2 : 0
                border.color: border.width > 0 ? "red" : "transparent"

                Text {
                    anchors.centerIn: parent
                    text: "Pinch with 2 fingers to scale, rotate and translate\nDrag with 1 finger"
                          + getTransformationDetails(rect2, pinch2) + "\nz " + rect2.z
                }
                DragHandler {
                    id: lsbDragHandler
                    objectName: "lightsteelblue drag"
                }
                PinchHandler {
                    id: pinch2
                    objectName: "lightsteelblue pinch"
                    minimumRotation: -45
                    maximumRotation: 45
                    minimumScale: 0.5
                    maximumScale: 3
                    xAxis.minimum: 0
                    xAxis.maximum: 600
                    // acceptedModifiers: Qt.ControlModifier
                }
                TapHandler { gesturePolicy: TapHandler.DragThreshold; onTapped: rect2.z = rect3.z + 1 }
            }

            Rectangle {
                id: rect3
                x: 500
                width: 400
                height: 300
                color: "wheat"
                antialiasing: true
                border.width: (wheatDragHandler.active || pinch3.active) ? 2 : 0
                border.color: border.width > 0 ? "red" : "transparent"

                Text {
                    anchors.centerIn: parent
                    text: "Pinch with 3 fingers to scale, rotate and translate\nDrag with 1 finger"
                          + getTransformationDetails(rect3, pinch3) + "\nz " + rect3.z
                }
                DragHandler {
                    id: wheatDragHandler
                    objectName: "wheat drag"
                }
                PinchHandler {
                    id: pinch3
                    objectName: "wheat 3-finger pinch"
                    minimumPointCount: 3
                    minimumScale: 0.1
                    maximumScale: 10
                    onActiveChanged: {
                        if (!active)
                            anim.restart(centroid.velocity)
                    }
                    onGrabChanged: function (transition, point) {
                        if (transition === 0x10) { // GrabExclusive
                            console.log(point.id, "grabbed @", point.position)
                            Qt.createQmlObject("import QtQuick; Rectangle { opacity: 0.5; border.color: 'red'; radius: 8; width: radius * 2; height: radius * 2; " +
                                               "x: " + (point.position.x - 8) + "; y: " + (point.position.y - 8) + "}",
                                               rect3, "touchpoint" + point.id);
                        }
                    }
                }
                TapHandler { gesturePolicy: TapHandler.DragThreshold; onTapped: rect3.z = rect2.z + 1 }
                MomentumAnimation { id: anim; target: rect3 }
            }

            Rectangle {
                id: rect4
                x: 400
                y: 250
                width: 300
                height: 220
                color: "#609cbc3d"
                radius: 10
                antialiasing: true
                border {
                    width: 5
                    color: "maroon"
                }
                PinchHandler {
                    id: pinch4
                    target: null
                    xAxis.onActiveValueChanged: (delta) => rect4.width = Math.min(500, Math.max(120, 15 * Math.round((rect4.width + delta) / 15)))
                    yAxis.onActiveValueChanged: (delta) => rect4.opacity = Math.max(0.1, Math.min(0.9, rect4.opacity - delta / 200))
                    rotationAxis.onActiveValueChanged: (delta) => rect4.radius = Math.max(0, Math.min(60, rect4.radius + delta))
                    scaleAxis.onActiveValueChanged: (delta) => rect4.border.width *= delta
                }
                Text {
                    anchors.fill: parent
                    anchors.margins: rect4.radius / Math.PI + rect4.border.width
                    text: "Pinch with 2 fingers to tweak various properties"
                    wrapMode: Text.WordWrap
                }
                Text {
                    anchors.top: rect4.bottom
                    anchors.left: rect4.left
                    text: "opacity " + rect4.opacity.toFixed(3) + " width " + rect4.width +
                          " border " + rect4.border.width.toFixed(1) + " radius " + rect4.radius.toFixed(1)
                }
            }
        }
    }
    Rectangle {
        id: centroidIndicator
        property QtObject pincher: activePincher()
        x: pincher.centroid.scenePosition.x - radius
        y: pincher.centroid.scenePosition.y - radius
        z: 1
        visible: pincher.active
        radius: width / 2
        width: 10
        height: width
        color: "red"
    }
}
