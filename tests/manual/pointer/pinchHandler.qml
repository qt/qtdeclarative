/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

import QtQuick 2.8
import Qt.labs.handlers 1.0
import "content"

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

    Rectangle {
        // Purpose of this item is just to make sure the rectangles are transformed into
        // a coordinate system that is different from the scene coordinate system.
        anchors.fill: parent
        anchors.margins: 50
        color: "#ffe0e0e0"

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
            border.width: pinch2.active ? 2 : 0
            border.color: pinch2.active ? "red" : "transparent"

            Text {
                anchors.centerIn: parent
                text: "Pinch with 2 fingers to scale, rotate and translate"
                    + getTransformationDetails(rect2, pinch2)
            }

            PinchHandler {
                id: pinch2
                objectName: "2-finger pinch"
                minimumRotation: -45
                maximumRotation: 45
                minimumScale: 0.5
                maximumScale: 3
                minimumX: 0
                maximumX: 600
                pointDistanceThreshold: 0
            }
        }

        Rectangle {
            id: rect3
            x: 500
            width: 400
            height: 300
            color: "wheat"
            antialiasing: true
            border.width: (dragHandler.active || pinch3.active) ? 2 : 0
            border.color: border.width > 0 ? "red" : "transparent"

            Text {
                anchors.centerIn: parent
                text: "Pinch with 3 fingers to scale, rotate and translate\nDrag with 1 finger"
                    + getTransformationDetails(rect3, pinch3)
            }
            DragHandler {
                id: dragHandler
                objectName: "DragHandler"
            }

            PinchHandler {
                id: pinch3
                objectName: "3-finger pinch"
                requiredPointCount: 3
                minimumScale: 0.1
                maximumScale: 10
                onActiveChanged: {
                    if (!active)
                        anim.restart(centroidVelocity)
                }
            }

            MomentumAnimation { id: anim; target: rect3 }
        }
    }
    Rectangle {
        id: centroidIndicator
        property QtObject pincher: pinch2.active ? pinch2 : pinch3
        x: pincher.centroid.x - radius
        y: pincher.centroid.y - radius
        z: 1
        visible: pincher.active
        radius: width / 2
        width: 10
        height: width
        color: "red"
    }
}
