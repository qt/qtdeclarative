/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

import QtQuick 2.8
//! [2]
import SceneGraphRendering 2.0
//! [2]

Item {
    Rectangle {
        id: bg
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0; color: "steelblue" }
            GradientStop { position: 1; color: "black" }
        }

        //! [5]
        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            onClicked: (mouse) => {
                if (mouse.button === Qt.LeftButton) {
                    clipper.clip = !clipper.clip
                } else if (mouse.button === Qt.RightButton) {
                    nonRectClipAnim.running = !nonRectClipAnim.running
                    if (!nonRectClipAnim.running)
                        clipper.rotation = 0;
                }
            }
        }
        // ![5]

        Rectangle {
            id: clipper
            width: parent.width / 2
            height: parent.height / 2
            anchors.centerIn: parent
            border.color: "yellow"
            border.width: 2
            color: "transparent"
            NumberAnimation on rotation {
                id: nonRectClipAnim
                from: 0; to: 360; duration: 5000; loops: Animation.Infinite
                running: false
            }

            //! [3]
            CustomRenderItem {
                id: renderer
                width: bg.width - 20
                height: bg.height - 20
                x: -clipper.x + 10
                y: -clipper.y + 10

                transform: [
                    Rotation { id: rotation; axis.x: 0; axis.z: 0; axis.y: 1; angle: 0; origin.x: renderer.width / 2; origin.y: renderer.height / 2; },
                    Translate { id: txOut; x: -renderer.width / 2; y: -renderer.height / 2 },
                    Scale { id: scale; },
                    Translate { id: txIn; x: renderer.width / 2; y: renderer.height / 2 }
                ]
            }
            //! [3]
        }

        SequentialAnimation {
            PauseAnimation { duration: 3000 }
            ParallelAnimation {
                NumberAnimation { target: scale; property: "xScale"; to: 0.6; duration: 1000; easing.type: Easing.InOutBack }
                NumberAnimation { target: scale; property: "yScale"; to: 0.6; duration: 1000; easing.type: Easing.InOutBack }
            }
            NumberAnimation { target: rotation; property: "angle"; to: 80; duration: 1000; easing.type: Easing.InOutCubic }
            NumberAnimation { target: rotation; property: "angle"; to: -80; duration: 1000; easing.type: Easing.InOutCubic }
            NumberAnimation { target: rotation; property: "angle"; to: 0; duration: 1000; easing.type: Easing.InOutCubic }
            NumberAnimation { target: renderer; property: "opacity"; to: 0.1; duration: 1000; easing.type: Easing.InOutCubic }
            PauseAnimation { duration: 1000 }
            NumberAnimation { target: renderer; property: "opacity"; to: 1.0; duration: 1000; easing.type: Easing.InOutCubic }
            ParallelAnimation {
                NumberAnimation { target: scale; property: "xScale"; to: 1; duration: 1000; easing.type: Easing.InOutBack }
                NumberAnimation { target: scale; property: "yScale"; to: 1; duration: 1000; easing.type: Easing.InOutBack }
            }
            running: true
            loops: Animation.Infinite
        }

        //! [4]
        Text {
            id: label
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.margins: 20
            color: "yellow"
            wrapMode: Text.WordWrap
            property int api: GraphicsInfo.api
            text: {
                var apiStr;
                switch (api) {
                case GraphicsInfo.OpenGL: apiStr = "OpenGL (direct)"; break;
                case GraphicsInfo.Direct3D12: apiStr = "Direct3D 12 (direct)"; break;
                case GraphicsInfo.Software: apiStr = "Software (QPainter)"; break;
                case GraphicsInfo.OpenGLRhi: apiStr = "OpenGL (RHI)"; break;
                case GraphicsInfo.MetalRhi: apiStr = "Metal (RHI)"; break;
                // the example has no other QSGRenderNode subclasses
                default: apiStr = "<UNSUPPORTED>"; break;
                }
                "Custom rendering via the graphics API " + apiStr
                        + "\nLeft click to toggle clipping to yellow rect"
                        + "\nRight click to rotate (can be used to exercise stencil clip instead of scissor)"
            }
        // ![4]
        }

        Text {
            id: label2
            anchors.top: parent.top
            anchors.right: parent.right
            color: "yellow"
            text: "Clip: " + (clipper.clip ? "ON" : "OFF") + " Rotation: " + (nonRectClipAnim.running ? "ON" : "OFF")
        }
    }
}
