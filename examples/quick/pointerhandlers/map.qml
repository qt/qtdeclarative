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
        xAxis.enabled: false
        yAxis.enabled: false
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
