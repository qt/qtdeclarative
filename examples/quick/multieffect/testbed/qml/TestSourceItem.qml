// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material

Item {
    id: rootItem

    Material.theme: Material.Dark
    Material.accent: Material.LightGreen

    Image {
        anchors.centerIn: parent
        width: parent.width / 2
        height: parent.height / 2
        sourceSize.width: width
        sourceSize.height: height
        source: "images/Built_with_Qt.png"
        fillMode: Image.PreserveAspectFit
        SequentialAnimation on anchors.verticalCenterOffset {
            loops: Animation.Infinite
            paused: !settings.animateMovement
            NumberAnimation {
                to: 50
                duration: 2000
                easing.type: Easing.InOutQuad
            }
            NumberAnimation {
                to: -50
                duration: 2000
                easing.type: Easing.InOutQuad
            }
        }
    }
    Text {
        font.pixelSize: 50
        font.bold: true
        text: "TESTING"
        color: "white"
    }
    Image {
        source: "images/warning.png"
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        mipmap: true
        SequentialAnimation on scale {
            loops: Animation.Infinite
            paused: !settings.animateMovement
            NumberAnimation {
                to: 0.4
                duration: 3000
                easing.type: Easing.InOutQuad
            }
            NumberAnimation {
                to: 1.0
                duration: 1000
                easing.type: Easing.OutBack
            }
        }

    }
    Rectangle {
        width: parent.width * 0.2
        height: width
        anchors.top: parent.top
        anchors.topMargin: width * 0.2
        anchors.right: parent.right
        anchors.rightMargin: width * 0.2
        color: "#808080"
        border.color: "#f0f0f0"
        border.width: 2
        radius: width * 0.1
        SequentialAnimation on opacity {
            paused: !settings.animateMovement
            loops: Animation.Infinite
            NumberAnimation {
                to: 0.0
                duration: 2000
                easing.type: Easing.InOutQuad
            }
            NumberAnimation {
                to: 1.0
                duration: 2000
                easing.type: Easing.InOutQuad
            }
        }

        NumberAnimation on rotation {
            paused: !settings.animateMovement
            loops: Animation.Infinite
            from: 0
            to: 360
            duration: 10000
        }
    }

    Column {
        anchors.right: parent.right
        anchors.rightMargin: width * 0.2
        anchors.bottom: parent.bottom
        Button {
            id: button
            text: "Controls Button"
            Material.theme: Material.Light
        }
        Slider {
            width: button.width
        }
    }
}

