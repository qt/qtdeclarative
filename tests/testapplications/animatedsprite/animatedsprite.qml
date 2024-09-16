// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0

Rectangle {
    id: main

    property bool reversed: false
    property real speed: 5
    property bool framesync: false

    width: 320
    height: 480
    color: "lightgray"

    Column {
        id: controls
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 30
        width: parent.width - 10
        spacing: 5
        Text {
            text: framesync ? "Rate: FrameSync" : "Rate: FrameRate"
            width: controls.width
            height: 50
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
            MouseArea {
                anchors.fill: parent
                onClicked: framesync = !framesync
            }
            Rectangle { anchors.fill: parent; color: "transparent"; border.color: "black"; radius: 5 }
        }
        Text {
            text:  reversed ? "Reverse" : "Forward"
            width: controls.width
            height: 50
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
            MouseArea {
                anchors.fill: parent
                onClicked: reversed = !reversed
            }
            Rectangle { anchors.fill: parent; color: "transparent"; border.color: "black"; radius: 5 }
        }

        Text {
            text: "FPS: "+s1.frameRate
            width: controls.width
            height: 50
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
            Rectangle {
                height: parent.height
                width: height
                Text { anchors.centerIn: parent; text: "-" }
                MouseArea {
                    anchors.fill: parent
                    onClicked: speed = speed - 1
                }
            }
            Rectangle {
                height: parent.height
                width: height
                anchors.right: parent.right
                Text { anchors.centerIn: parent; text: "+" }
                MouseArea {
                    anchors.fill: parent
                    onClicked: speed = speed + 1
                }
            }
            Rectangle { anchors.fill: parent; color: "transparent"; border.color: "black"; radius: 5 }
        }
    }

    AnimatedSprite {
        id: s1
        anchors.centerIn: parent
        anchors.verticalCenterOffset: -80
        running: true
        height: 125
        width: 125
        frameCount: 13
        frameDuration: 50
        frameRate: speed
        frameSync: framesync
        reverse: reversed
        interpolate: false
        source: "bear_tiles.png"
    }
}
