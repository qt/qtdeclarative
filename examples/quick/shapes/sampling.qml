// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Shapes

Rectangle {
    id: root
    width: 1024
    height: 768

    readonly property color col: "lightsteelblue"
    gradient: Gradient {
        GradientStop {
            position: 0.0
            color: Qt.tint(root.col, "#20FFFFFF")
        }
        GradientStop {
            position: 0.1
            color: Qt.tint(root.col, "#20AAAAAA")
        }
        GradientStop {
            position: 0.9
            color: Qt.tint(root.col, "#20666666")
        }
        GradientStop {
            position: 1.0
            color: Qt.tint(root.col, "#20000000")
        }
    }

    Row {
        anchors {
            fill: parent
            margins: 20
        }
        spacing: 40

        Column {
            spacing: 40

            Text {
                text: qsTr("Original")
            }

            // A simple Shape without anything special.
            Rectangle {
                color: "lightGray"
                width: 400
                height: 200

                Shape {
                    x: 30
                    y: 20
                    width: 50
                    height: 50
                    scale: 2

                    ShapePath {
                        strokeColor: "green"
                        NumberAnimation on strokeWidth {
                            from: 1
                            to: 20
                            duration: 5000
                        }
                        fillColor: "transparent"
                        capStyle: ShapePath.RoundCap

                        startX: 40
                        startY: 30
                        PathQuad {
                            x: 50
                            y: 80
                            controlX: 0
                            controlY: 80
                        }
                        PathLine {
                            x: 150
                            y: 80
                        }
                        PathQuad {
                            x: 160
                            y: 30
                            controlX: 200
                            controlY: 80
                        }
                    }
                }
            }

            Text {
                text: qsTr("Supersampling (2x)")
            }

            // Now let's use 2x supersampling via layers. This way the entire subtree
            // is rendered into an FBO twice the size and then drawn with linear
            // filtering. This allows having some level of AA even when there is no
            // support for multisample framebuffers.
            Rectangle {
                id: supersampledItem
                color: "lightGray"
                width: 400
                height: 200

                layer.enabled: true
                layer.smooth: true
                layer.textureSize: Qt.size(supersampledItem.width * 2, supersampledItem.height * 2)

                Shape {
                    x: 30
                    y: 20
                    width: 50
                    height: 50
                    scale: 2

                    ShapePath {
                        strokeColor: "green"
                        NumberAnimation on strokeWidth {
                            from: 1
                            to: 20
                            duration: 5000
                        }
                        fillColor: "transparent"
                        capStyle: ShapePath.RoundCap

                        startX: 40
                        startY: 30
                        PathQuad {
                            x: 50
                            y: 80
                            controlX: 0
                            controlY: 80
                        }
                        PathLine {
                            x: 150
                            y: 80
                        }
                        PathQuad {
                            x: 160
                            y: 30
                            controlX: 200
                            controlY: 80
                        }
                    }
                }
            }
        }

        Column {
            spacing: 40

            Text {
                text: qsTr("Multisampling (4x)")
            }

            // Now let's use 4x MSAA, again via layers. This needs support for
            // multisample renderbuffers and framebuffer blits.
            Rectangle {
                color: "lightGray"
                width: 400
                height: 200

                layer.enabled: true
                layer.smooth: true
                layer.samples: 4

                Shape {
                    x: 30
                    y: 20
                    width: 50
                    height: 50
                    scale: 2

                    ShapePath {
                        strokeColor: "green"
                        NumberAnimation on strokeWidth {
                            from: 1
                            to: 20
                            duration: 5000
                        }
                        fillColor: "transparent"
                        capStyle: ShapePath.RoundCap

                        startX: 40
                        startY: 30
                        PathQuad {
                            x: 50
                            y: 80
                            controlX: 0
                            controlY: 80
                        }
                        PathLine {
                            x: 150
                            y: 80
                        }
                        PathQuad {
                            x: 160
                            y: 30
                            controlX: 200
                            controlY: 80
                        }
                    }
                }
            }
            Text {
                text: qsTr("CurveRenderer [tech preview]")
            }

            // Now let's use CurveRenderer with built-in antialiasing support.
            Rectangle {
                color: "lightGray"
                width: 400
                height: 200

                Shape {
                    x: 30
                    y: 20
                    width: 50
                    height: 50
                    scale: 2
                    preferredRendererType: Shape.CurveRenderer
                    antialiasing: true

                    ShapePath {
                        strokeColor: "green"
                        NumberAnimation on strokeWidth {
                            from: 1
                            to: 20
                            duration: 5000
                        }
                        fillColor: "transparent"
                        capStyle: ShapePath.RoundCap

                        startX: 40
                        startY: 30
                        PathQuad {
                            x: 50
                            y: 80
                            controlX: 0
                            controlY: 80
                        }
                        PathLine {
                            x: 150
                            y: 80
                        }
                        PathQuad {
                            x: 160
                            y: 30
                            controlX: 200
                            controlY: 80
                        }
                    }
                }
            }
        }
    }
}
