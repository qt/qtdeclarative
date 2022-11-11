// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Item {
    id: root
    property int frameCounter: 0
    property int frameCounterAvg: 0
    property int counter: 0
    property int fps: 0
    property int fpsAvg: 0

    width: 200 * dp
    height: Math.floor(42 * dp)

    Image {
        id: spinnerImage
        anchors.verticalCenter: parent.verticalCenter
        x: 4 * dp
        width: 32 * dp
        height: width
        source: "images/spinner.png"
        NumberAnimation on rotation {
            from:0
            to: 360
            duration: 800
            loops: Animation.Infinite
        }
        onRotationChanged: frameCounter++;
    }
    Image {
        anchors.centerIn: spinnerImage
        width: 18 * dp
        height: width
        source: settings.animateMovement ? "images/play.png" : "images/pause.png"
        opacity: 0.5
    }

    Text {
        anchors.left: spinnerImage.right
        anchors.leftMargin: 8 * dp
        anchors.verticalCenter: spinnerImage.verticalCenter
        color: "#c0c0c0"
        font.pixelSize: 22 * dp
        text: "Ø " + root.fpsAvg + " | " + root.fps + " fps"
    }

    Timer {
        interval: 2000
        repeat: true
        running: true
        onTriggered: {
            frameCounterAvg += frameCounter;
            root.fps = Math.ceil(frameCounter / 2);
            counter++;
            frameCounter = 0;
            if (counter >= 3) {
                root.fpsAvg = Math.ceil(frameCounterAvg / (2 * counter));
                frameCounterAvg = 0;
                counter = 0;
            }
        }
    }
}
