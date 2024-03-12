// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


import QtQuick 2.0
Rectangle {
    id: page
    width: 640
    height: 480
    color: "white"
    Rectangle {
        id: header
        color: "#c0c0c0"
        height: usage.height + chkClip.height
        anchors.left: parent.left
        anchors.right: parent.right
        Text {
            id: usage
            text: "Use an a11y inspect tool to see if all visible rectangles can be found with hit testing."
        }
        Rectangle {
            id: chkClip
            property bool checked: true

            color: (checked ? "#f0f0f0" : "#c0c0c0")
            height: label.height
            width: label.width
            anchors.left: parent.left
            anchors.bottom: parent.bottom

            MouseArea {
                anchors.fill: parent
                onClicked: chkClip.checked = !chkClip.checked
            }
            Text {
                id: label
                text: "Click here to toggle clipping"
            }
        }
    }
    Rectangle {
        clip: chkClip.checked
        z: 2
        id: rect1
        width: 100
        height: 100
        color: "#ffc0c0"
        anchors.top: header.bottom
        Rectangle {
            id: rect2
            width: 100
            height: 100
            x: 50
            y: 50
            color: "#ffa0a0"
            Rectangle {
                id: rect31
                width: 100
                height: 100
                x: 80
                y: 80
                color: "#ff8080"
            }
            Rectangle {
                id: rect32
                x: 100
                y: 70
                z: 3
                width: 100
                height: 100
                color: "#e06060"
            }
            Rectangle {
                id: rect33
                width: 100
                height: 100
                x: 150
                y: 60
                color: "#c04040"
            }
        }
    }

    Rectangle {
        x: 0
        y: 50
        id: recta1
        width: 100
        height: 100
        color: "#c0c0ff"
        Rectangle {
            id: recta2
            width: 100
            height: 100
            x: 50
            y: 50
            color: "#a0a0ff"
            Rectangle {
                id: recta31
                width: 100
                height: 100
                x: 80
                y: 80
                color: "#8080ff"
            }
            Rectangle {
                id: recta32
                x: 100
                y: 70
                z: 100
                width: 100
                height: 100
                color: "#6060e0"
            }
            Rectangle {
                id: recta33
                width: 100
                height: 100
                x: 150
                y: 60
                color: "#4040c0"
            }
        }
    }

}
