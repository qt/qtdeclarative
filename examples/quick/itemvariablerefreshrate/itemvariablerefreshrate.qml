// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

Rectangle {
    id: root
    width: 640
    height: 480
    visible: true

    //! [frameAnimation]
    FrameAnimation {
      id: frameAnimation
      property real fps: smoothFrameTime > 0 ? (1.0 / smoothFrameTime) : 0
      running: true
    }
    //! [frameAnimation]


    //! [item]
    Item {
        id: qt_logo
        width: 230
        height: 230
        anchors.fill: parent
        anchors.topMargin: 125
        layer.enabled: true
        layer.live: slider.value > 0 && frameAnimation.currentFrame % slider.value == 0
        //! [item]

        Rectangle {
            id: rectangle
            anchors.fill: parent
            color: "black"
            Image {
                anchors.fill: parent
                source: "content/qt_logo.png"
            }

            // one second is one full rotation
            RotationAnimation on rotation {
                loops: Animation.Infinite
                duration: 1000
                from: 0
                to: 360
            }
        }
    }

    Column {
        anchors.top: root.top
        anchors.left: root.left
        anchors.margins: 10

        Label {
            text: "FPS: " + frameAnimation.fps.toFixed(0)
            font.pixelSize: 20
            font.italic: true
        }

        Label {
            text: "Rect rotation (degrees): " + rectangle.rotation.toFixed(0)
            font.pixelSize: 20
            font.italic: true
        }

        Row {
            Label {
                text: qsTr("Draw every " + slider.value.toFixed(0) + " frame")
                font.pixelSize: 20
                font.italic: true
            }

            Slider {
                id: slider
                from: 0
                value: 1
                to: 10
                stepSize: 1
            }
        }
    }
}
