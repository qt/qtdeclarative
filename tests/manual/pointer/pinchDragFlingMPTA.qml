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

import QtQuick 2.8
import Qt.labs.handlers 1.0
import "content"

Rectangle {
    width: 1024; height: 600
    color: "beige"
    objectName: "beige root"

    function getTransformationDetails(item, pinchhandler) {
        return "scale:" + pinchhandler.scale.toFixed(2)
                + " rotation:" + pinchhandler.rotation.toFixed(2)
                + " translation:" + "(" + pinchhandler.translation.x.toFixed(2) + "," + pinchhandler.translation.y.toFixed(2) + ")"
    }

    Rectangle {
        id: container
        objectName: "container rect"
        width: 600
        height: 500
        color: pinch3.active ? "red" : "black"
        antialiasing: true
        Loader {
            source: "../touch/mpta-crosshairs.qml"
            anchors.fill: parent
            anchors.margins: 2
        }
        Item {
            anchors.fill: parent
            // In order for PinchHandler to get a chance to take a passive grab, it has to get the touchpoints first.
            // In order to get the touchpoints first, it has to be on top of the Z order: i.e. come last in paintOrderChildItems().
            // This is the opposite situation as with filtersChildMouseEvents: e.g. PinchArea would have wanted to be the parent,
            // if it even knew that trick (which it doesn't).
            PinchHandler {
                id: pinch3
                objectName: "3-finger pinch"
                target: container
                minimumPointCount: 3
                minimumScale: 0.1
                maximumScale: 10
                onActiveChanged: if (!active) fling.restart(centroidVelocity)
            }
            DragHandler {
                id: dragHandler
                objectName: "DragHandler"
                target: container
                acceptedModifiers: Qt.MetaModifier
                onActiveChanged: if (!active) fling.restart(point.velocity)
            }
        }
        MomentumAnimation { id: fling; target: container }
    }
    Text {
        anchors.bottom: parent.bottom
        text: pinch3.active ? getTransformationDetails(container, pinch3) : "Pinch with 3 fingers to scale, rotate and translate"
    }
}
