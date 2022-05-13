// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

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
