// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
import QtQuick
import QtQuick.Controls

Column {
    width: 320
    height: 480
    spacing: 6
    y: 12

//! [image]
    AnimatedImage {
        id: animation

        source: "pics/Uniflow_steam_engine.gif"
        anchors.horizontalCenter: parent.horizontalCenter
        speed: speedSlider.value
        TapHandler {
            onTapped: animation.playing = !animation.playing
        }
    }
//! [image]

    Rectangle {
        id: timeline

        color: "steelblue"
        width: animation.width
        height: 1
        x: animation.x
        y: animation.height + 12
        visible: animation.playing

        Rectangle {
            property int frames: animation.frameCount
            width: 4
            height: 8
            x: (animation.width - width) * animation.currentFrame / frames
            y: -4
            color: "red"
        }
    }

    Row {
        spacing: 6
        anchors.horizontalCenter: parent.horizontalCenter

        Label {
            text: qsTr("Speed:")
            font.pointSize: 12
            anchors.verticalCenter: speedSlider.verticalCenter
        }

        Slider {
            id: speedSlider

            from: 0
            to: 5
            value: 1
        }

        Label {
            font: fontMetrics.font
            text: qsTr(Math.round(animation.speed * 100) + "%")
            width: fontMetrics.width
            anchors.verticalCenter: speedSlider.verticalCenter
        }

        TextMetrics {
            id: fontMetrics

            text: "100%"
            font.pointSize: 12
        }
    }

    Button {
        text: qsTr("Reset")
        enabled: speedSlider.value !== 1
        anchors.horizontalCenter: parent.horizontalCenter
        onClicked: {
            speedSlider.value = 1
            animation.playing = true
        }
    }
}
