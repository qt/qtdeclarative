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
import "components"

Rectangle {
    id: root
    width: 640
    height: 480
    color: "black"

    Repeater {
        model: 2

        Image {
            id: ball
            objectName: "ball" + index
            source: "images/redball.png"
            property real homeX: 200 + index * 200
            property real homeY: 200
            width: 80; height: 80; x: homeX; y: 200

            Text {
                anchors.centerIn: parent
                color: "white"
                text: momentum.velocity.x.toFixed(2) + "," + momentum.velocity.y.toFixed(2)
            }

            SequentialAnimation {
                id: anim

                function restart(vel) {
                    stop()
                    momentum.velocity = vel
                    start()
                }

                MomentumAnimation { id: momentum; target: ball }

                PauseAnimation { duration: 500 }

                ParallelAnimation {
                    id: ballReturn
                    NumberAnimation {
                        target: ball
                        property: "x"
                        to: homeX
                        duration: 1000
                        easing.period: 50
                        easing.type: Easing.OutElastic
                    }
                    NumberAnimation {
                        target: ball
                        property: "y"
                        to: homeY
                        duration: 1000
                        easing.type: Easing.OutElastic
                    }
                }
            }

            DragHandler {
                id: dragHandler
                objectName: "dragHandler" + index
                onActiveChanged: {
                    if (!active)
                        anim.restart(centroid.velocity)
                }
            }
            Rectangle {
                visible: dragHandler.active
                anchors.fill: parent
                anchors.margins: -5
                radius: width / 2
                opacity: 0.25
            }

            Rectangle {
                visible: width > 0
                width: dragHandler.centroid.velocity.length() / 10
                height: 2
                x: ball.width / 2
                y: ball.height / 2
                z: -1
                rotation: Math.atan2(dragHandler.centroid.velocity.y, dragHandler.centroid.velocity.x) * 180 / Math.PI
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
        }
    }
}
