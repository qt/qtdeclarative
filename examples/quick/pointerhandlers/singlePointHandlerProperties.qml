// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Rectangle {
    id: root
    width: 480
    height: 480
    color: "black"

    Item {
        id: crosshairs
        x: pointHandler.point.position.x - width / 2
        y: pointHandler.point.position.y - height / 2
        width: parent.width / 2; height: parent.height / 2
        visible: pointHandler.active
        rotation: pointHandler.point.rotation

        Rectangle {
            color: "goldenrod"
            anchors.centerIn: parent
            width: 2; height: parent.height
            antialiasing: true
        }
        Rectangle {
            color: "goldenrod"
            anchors.centerIn: parent
            width: parent.width; height: 2
            antialiasing: true
        }
        Rectangle {
            color: "goldenrod"
            width: Math.max(2, 50 * pointHandler.point.pressure)
            height: width
            radius: width / 2
            anchors.centerIn: parent
            antialiasing: true
            Rectangle {
                y: -56
                anchors.horizontalCenter: parent.horizontalCenter
                color: "lightsteelblue"
                implicitWidth: label.implicitWidth
                implicitHeight: label.implicitHeight
                Text {
                    id: label
                    text: 'seat: ' + pointHandler.point.device.seatName + '\ndevice: ' + pointHandler.point.device.name +
                        '\nid: ' + pointHandler.point.id.toString(16) + " uid: " + pointHandler.point.uniqueId.numericId +
                        '\npos: (' + pointHandler.point.position.x.toFixed(2) + ', ' + pointHandler.point.position.y.toFixed(2) + ')' +
                        '\nmodifiers: ' + pointHandler.point.modifiers.toString(16)
                }
            }
        }
        Rectangle {
            color: "transparent"
            border.color: "white"
            antialiasing: true
            width: pointHandler.point.ellipseDiameters.width
            height: pointHandler.point.ellipseDiameters.height
            radius: Math.min(width / 2, height / 2)
            anchors.centerIn: parent
        }
    }
    Rectangle {
        id: velocityVector
        visible: width > 0
        width: pointHandler.point.velocity.length() / 10
        height: 2
        x: pointHandler.point.position.x
        y: pointHandler.point.position.y
        rotation: Math.atan2(pointHandler.point.velocity.y, pointHandler.point.velocity.x) * 180 / Math.PI
        transformOrigin: Item.BottomLeft
        antialiasing: true

        Image {
            source: "images/arrowhead.png"
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            width: 16
            height: 12
            antialiasing: true
        }
    }

    Component {
        id: grabbingLocationIndicator
        Image {
            source: "components/images/grabbing-location.svg"
            sourceSize.width: 32
            sourceSize.height: 32
        }
    }

    Component {
        id: mouseButtonIndicator
        Image {
            property int buttons
            source: "components/images/mouse.png"
            Image {
                source: "components/images/mouse_left.png"
                visible: buttons & Qt.LeftButton
            }
            Image {
                source: "components/images/mouse_middle.png"
                visible: buttons & Qt.MiddleButton
            }
            Image {
                source: "components/images/mouse_right.png"
                visible: buttons & Qt.RightButton
            }
        }
    }

    PointHandler {
        id: pointHandler
        target: null
        acceptedButtons: Qt.AllButtons
        onGrabChanged: function(transition, point) {
            if (active) {
                console.log("grabbed " + point.pointId + " @ " + point.sceneGrabPos)
                grabbingLocationIndicator.createObject(root, {"x": point.sceneGrabPosition.x, "y": point.sceneGrabPosition.y - 16})
            }
        }
        onPointChanged: {
            if (point.pressedButtons)
                mouseButtonIndicator.createObject(root, {"x": point.pressPosition.x - 44, "y": point.pressPosition.y - 64, "buttons": point.pressedButtons})
        }
    }

    Text {
        color: "white"
        text: "drag to see feedback"
        anchors {
            bottom: parent.bottom
            horizontalCenter: parent.horizontalCenter
            margins: 6
        }
    }
}
