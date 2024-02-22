// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0

Item {
    Rectangle {
        width: 100
        height: 100
        anchors.centerIn: parent
        color: "red"
        NumberAnimation on rotation { from: 0; to: 360; duration: 2000; loops: Animation.Infinite; }

        Rectangle {
            color: "gray"
            width: 50
            height: 50
            anchors.centerIn: parent

            SequentialAnimation on opacity {
                loops: Animation.Infinite
                NumberAnimation {
                    from: 1.0
                    to: 0.0
                    duration: 4000
                }
                NumberAnimation {
                    from: 0.0
                    to: 1.0
                    duration: 4000
                    easing.type: Easing.InOutQuad
                }
            }
        }
    }

    Rectangle {
        color: "green"
        width: 100
        height: 200
        x: 0
        y: 0

        NumberAnimation on x {
            from: 0
            to: 300
            duration: 5000
        }
        NumberAnimation on y {
            from: 0
            to: 50
            duration: 2000
        }

        clip: true // scissor
        Rectangle {
            color: "lightGreen"
            width: 50
            height: 50
            x: 75
            y: 175
        }
    }

    Rectangle {
        color: "blue"
        width: 200
        height: 100
        x: 100
        y: 300
        radius: 16
        border.color: "red"
        border.width: 4

        SequentialAnimation on y {
            loops: Animation.Infinite
            NumberAnimation {
                from: 300
                to: 500
                duration: 7000
            }
            NumberAnimation {
                from: 500
                to: 300
                duration: 3000
            }
        }
    }

    Rectangle {
        anchors.right: parent.right
        width: 100
        height: 100
        gradient: Gradient {
            GradientStop { position: 0.0; color: "red" }
            GradientStop { position: 0.33; color: "yellow" }
            GradientStop { position: 1.0; color: "green" }
        }
    }
}
