/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
