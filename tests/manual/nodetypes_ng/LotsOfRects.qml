// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0

Item {
    Rectangle {
        anchors.margins: 4
        anchors.fill: parent

        // Background
        gradient: Gradient {
            GradientStop { position: 0; color: "steelblue" }
            GradientStop { position: 1; color: "black" }
        }

        // Animated gradient stops.
        // NB! Causes a full buffer rebuild on every animated change due to the geometry change!
        Row {
            spacing: 10
            Repeater {
                model: 20
                Rectangle {
                    width: 20
                    height: 20
                    gradient: Gradient {
                        GradientStop { position: 0.0; color: "red" }
                        GradientStop { NumberAnimation on position { from: 0.01; to: 0.99; duration: 5000; loops: Animation.Infinite } color: "yellow" }
                        GradientStop { position: 1.0; color: "green" }
                    }
                }
            }
        }

        // Rounded rects with border (smooth material)
        Row {
            spacing: 10
            Repeater {
                model: 5
                Rectangle {
                    color: "blue"
                    width: 100
                    height: 50
                    y: 50
                    radius: 16
                    border.color: "red"
                    border.width: 4

                    SequentialAnimation on y {
                        loops: Animation.Infinite
                        NumberAnimation {
                            from: 50
                            to: 150
                            duration: 7000
                        }
                        NumberAnimation {
                            from: 150
                            to: 50
                            duration: 3000
                        }
                    }
                }
            }
        }

        // Clip using scissor
        Row {
            spacing: 10
            Repeater {
                model: 5
                Rectangle {
                    color: "green"
                    width: 100
                    height: 100
                    y: 150
                    NumberAnimation on y {
                        from: 150
                        to: 200
                        duration: 2000
                        loops: Animation.Infinite
                    }
                    clip: true
                    Rectangle {
                        color: "lightGreen"
                        width: 50
                        height: 50
                        x: 75
                        y: 75
                    }
                }
            }
        }

        // Clip using scissor
        Row {
            spacing: 10
            Repeater {
                model: 5
                Rectangle {
                    color: "green"
                    width: 100
                    height: 100
                    y: 300
                    NumberAnimation on y {
                        from: 300
                        to: 400
                        duration: 2000
                        loops: Animation.Infinite
                    }
                    clip: true
                    Rectangle {
                        color: "lightGreen"
                        width: 50
                        height: 50
                        x: 75
                        y: 75
                        NumberAnimation on rotation { from: 0; to: 360; duration: 2000; loops: Animation.Infinite; }
                    }
                }
            }
        }

        // Clip using stencil
        Row {
            spacing: 10
            Repeater {
                model: 5
                Rectangle {
                    color: "green"
                    width: 100
                    height: 100
                    y: 450
                    NumberAnimation on y {
                        from: 450
                        to: 550
                        duration: 2000
                        loops: Animation.Infinite
                    }
                    NumberAnimation on rotation { from: 0; to: 360; duration: 2000; loops: Animation.Infinite; }
                    clip: true
                    Rectangle {
                        color: "lightGreen"
                        width: 50
                        height: 50
                        x: 75
                        y: 75
                    }
                }
            }
        }

        // The signature red square with another item with animated opacity blended on top
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

        // Animated size and color.
        // NB! Causes a full buffer rebuild on every animated change due to the geometry change!
        Rectangle {
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            width: 10
            height: 100
            ColorAnimation on color {
                from: "blue"
                to: "purple"
                duration: 5000
                loops: Animation.Infinite
            }
            NumberAnimation on width {
                from: 10
                to: 300
                duration: 5000
                loops: Animation.Infinite
            }
        }

        // Semi-transparent rect on top.
        Rectangle {
            anchors.centerIn: parent
            opacity: 0.2
            color: "black"
            anchors.fill: parent
            anchors.margins: 10
        }
    }
}
