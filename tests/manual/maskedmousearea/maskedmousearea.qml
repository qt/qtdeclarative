// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import Example

Rectangle {
    height: 480
    width: 320
    color: "black"

    Text {
        text: qsTr("CLICK AND HOVER")
        opacity: 0.6
        color: "white"
        font.pixelSize: 20
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.topMargin: 50
    }

    Image {
        id: moon
        anchors.centerIn: parent
        scale: moonArea.pressed ? 1.1 : 1.0
        opacity: moonArea.containsMouse ? 1.0 : 0.7
        source: Qt.resolvedUrl("images/moon.png")

        MaskedMouseArea {
            id: moonArea
            anchors.fill: parent
            alphaThreshold: 0.4
            maskSource: moon.source
        }

        Behavior on opacity {
            NumberAnimation { duration: 200 }
        }
        Behavior on scale {
            NumberAnimation { duration: 100 }
        }
    }

    Image {
        id: rightCloud
        anchors {
            centerIn: moon
            verticalCenterOffset: 30
            horizontalCenterOffset: 80
        }
        scale: rightCloudArea.pressed ? 1.1 : 1.0
        opacity: rightCloudArea.containsMouse ? 1.0 : 0.7
        source: Qt.resolvedUrl("images/cloud_2.png")

        MaskedMouseArea {
            id: rightCloudArea
            anchors.fill: parent
            alphaThreshold: 0.4
            maskSource: rightCloud.source
        }

        Behavior on opacity {
            NumberAnimation { duration: 200 }
        }
        Behavior on scale {
            NumberAnimation { duration: 100 }
        }
    }

    Image {
        id: leftCloud
        anchors {
            centerIn: moon
            verticalCenterOffset: 40
            horizontalCenterOffset: -80
        }
        scale: leftCloudArea.pressed ? 1.1 : 1.0
        opacity: leftCloudArea.containsMouse ? 1.0 : 0.7
        source: Qt.resolvedUrl("images/cloud_1.png")

        MaskedMouseArea {
            id: leftCloudArea
            anchors.fill: parent
            alphaThreshold: 0.4
            maskSource: leftCloud.source
        }

        Behavior on opacity {
            NumberAnimation { duration: 200 }
        }
        Behavior on scale {
            NumberAnimation { duration: 100 }
        }
    }
}
