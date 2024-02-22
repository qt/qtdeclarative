// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0
import QtQuick.Window 2.12

Item {
    Rectangle {
        x: 20
        y: 20
        width: 300
        height: 120
        color: "red"
        border.color: "black"
        border.width: 2
        Text {
            text: "Click to toggle window visibility\n(switch to another test to destroy)"
            font.bold: true
            anchors.centerIn: parent
        }
        MouseArea {
            anchors.fill: parent
            onClicked: win.visible = !win.visible
        }
    }

    Rectangle {
        width: 100
        height: 100
        anchors.centerIn: parent
        color: "green"
        NumberAnimation on rotation { from: 0; to: 360; duration: 2000; loops: Animation.Infinite; }
    }

    Window {
        id: win
        width: 640
        height: 480

        Rectangle {
            color: "lightGray"
            anchors.fill: parent
            Rectangle {
                width: 100
                height: 100
                anchors.centerIn: parent
                color: "red"
                NumberAnimation on rotation { from: 0; to: 360; duration: 2000; loops: Animation.Infinite; }
            }
            Text {
                text: "Another QQuickWindow"
                anchors.top: parent.top
                anchors.margins: 20
                ColorAnimation on color { from: "red"; to: "green"; duration: 2000; loops: Animation.Infinite }
            }
        }
    }
}
