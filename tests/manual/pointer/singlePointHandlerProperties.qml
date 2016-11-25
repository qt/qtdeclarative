/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the manual tests of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
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

import QtQuick 2.9
import Qt.labs.handlers 1.0

Rectangle {
    id: root
    width: 480
    height: 480
    color: "black"

    Item {
        id: crosshairs
        x: dragHandler.pos.x - width / 2
        y: dragHandler.pos.y - height / 2
        width: parent.width / 2; height: parent.height / 2
        visible: dragHandler.active
        rotation: dragHandler.rotation

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
            width: Math.max(2, 50 * dragHandler.pressure)
            height: width
            radius: width / 2
            anchors.centerIn: parent
            antialiasing: true
            Rectangle {
                y: -40
                anchors.horizontalCenter: parent.horizontalCenter
                color: "lightsteelblue"
                implicitWidth: label.implicitWidth
                implicitHeight: label.implicitHeight
                Text {
                    id: label
                    text: 'id: ' + dragHandler.pointId.toString(16) + " uid: " + dragHandler.uniquePointId.numericId +
                        '\npos: (' + dragHandler.pos.x.toFixed(2) + ', ' + dragHandler.pos.y.toFixed(2) + ')'
                }
            }
        }
        Rectangle {
            color: "transparent"
            border.color: "white"
            antialiasing: true
            width: dragHandler.ellipseDiameters.width
            height: dragHandler.ellipseDiameters.height
            radius: Math.min(width / 2, height / 2)
            anchors.centerIn: parent
        }
    }
    Rectangle {
        id: velocityVector
        visible: width > 0
        width: dragHandler.velocity.length() * 100
        height: 2
        x: dragHandler.pos.x
        y: dragHandler.pos.y
        rotation: Math.atan2(dragHandler.velocity.y, dragHandler.velocity.x) * 180 / Math.PI
        transformOrigin: Item.BottomLeft
        antialiasing: true

        Image {
            source: "resources/arrowhead.png"
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
            source: "resources/grabbing-location.svg"
            sourceSize.width: 32
            sourceSize.height: 32
        }
    }

    Component {
        id: mouseButtonIndicator
        Image {
            property int buttons
            source: "resources/mouse.png"
            Image {
                source: "resources/mouse_left.png"
                visible: buttons & Qt.LeftButton
            }
            Image {
                source: "resources/mouse_middle.png"
                visible: buttons & Qt.MidButton
            }
            Image {
                source: "resources/mouse_right.png"
                visible: buttons & Qt.RightButton
            }
        }
    }

    DragHandler {
        id: dragHandler
        target: null
        onGrabChanged: if (active) {
            console.log("grabbed " + point.pointId + " @ " + sceneGrabPos)
            grabbingLocationIndicator.createObject(root, {"x": sceneGrabPos.x, "y": sceneGrabPos.y - 16})
        }
        onPressedButtonsChanged: {
            if (pressedButtons)
                mouseButtonIndicator.createObject(root, {"x": pressPos.x - 44, "y": pressPos.y - 64, "buttons": pressedButtons})
        }
    }
}
