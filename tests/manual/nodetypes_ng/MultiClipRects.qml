// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

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

        // Clip using scissor, up to 2 levels. This means that the
        // lightGreen-yellow-blue batch's clip list will have two clips.
        Row {
            spacing: 10
            Repeater {
                model: 5
                Rectangle {
                    color: "green"
                    width: 150
                    height: 150
                    y: 200
                    clip: true
                    Rectangle {
                        color: "lightGreen"
                        width: 150
                        height: 150
                        x: 25
                        y: 25
                        clip: true

                        Rectangle {
                            color: "yellow"
                            width: 50
                            height: 50
                            x: 100
                            y: 100
                            NumberAnimation on rotation { from: 360; to: 0; duration: 5000; loops: Animation.Infinite; }
                        }

                        Rectangle {
                            color: "blue"
                            width: 50
                            height: 50
                            x: -25
                            y: 100
                            NumberAnimation on rotation { from: 360; to: 0; duration: 5000; loops: Animation.Infinite; }
                        }
                    }
                }
            }
        }

        // Clip using stencil, up to 3 levels. This means that the
        // lightGreen-yellow-blue batch's clip list will have three clips and
        // so two stencil draw calls before drawing the actual content.
        Row {
            spacing: 10
            Repeater {
                model: 5
                Rectangle {
                    color: "green"
                    width: 200
                    height: 200
                    y: 450
                    NumberAnimation on rotation { from: 0; to: 360; duration: 5000; loops: Animation.Infinite; }
                    clip: true
                    Rectangle {
                        color: "lightGreen"
                        width: 150
                        height: 150
                        x: 50
                        y: 50
                        rotation: 30
                        clip: true

                        Rectangle {
                            color: "yellow"
                            width: 100
                            height: 100
                            x: 75
                            y: 75
                            NumberAnimation on rotation { from: 360; to: 0; duration: 5000; loops: Animation.Infinite; }
                            clip: true

                            Rectangle {
                                color: "blue"
                                width: 50
                                height: 50
                                x: 0
                                y: 0
                                NumberAnimation on rotation { from: 360; to: 0; duration: 5000; loops: Animation.Infinite; }
                            }
                        }
                    }
                }
            }
        }
    }
}
