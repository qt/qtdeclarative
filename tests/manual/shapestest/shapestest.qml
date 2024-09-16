// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.9
import QtQuick.Shapes 1.0

Rectangle {
    id: root
    width: 1024
    height: 768

    property color col: "lightsteelblue"
    gradient: Gradient {
        GradientStop { position: 0.0; color: Qt.tint(root.col, "#20FFFFFF") }
        GradientStop { position: 0.1; color: Qt.tint(root.col, "#20AAAAAA") }
        GradientStop { position: 0.9; color: Qt.tint(root.col, "#20666666") }
        GradientStop { position: 1.0; color: Qt.tint(root.col, "#20000000") }
    }

    Row {
        anchors.top: parent.top
        anchors.centerIn: parent
        spacing: 20

        Column {
            spacing: 20

            Rectangle {
                border.color: "purple"
                color: "transparent"
                width: 200
                height: 100
                Shape {
                    id: triangle
                    anchors.fill: parent
                    ShapePath {
                        strokeWidth: 4
                        strokeColor: "red"
                        fillGradient: LinearGradient {
                            x1: 0; y1: 0
                            x2: 200; y2: 100
                            GradientStop { position: 0; color: "blue" }
                            GradientStop { position: 0.2; color: "green" }
                            GradientStop { position: 0.4; color: "red" }
                            GradientStop { position: 0.6; color: "yellow" }
                            GradientStop { position: 1; color: "cyan" }
                        }
                        fillColor: "blue" // ignored with the gradient set
                        strokeStyle: ShapePath.DashLine
                        dashPattern: [ 1, 4 ]
                        PathLine { x: 200; y: 100 }
                        PathLine { x: 0; y: 100 }
                        PathLine { x: 0; y: 0 }
                    }
                    transform: Rotation { origin.x: 100; origin.y: 50; axis { x: 0; y: 1; z: 0 }
                        SequentialAnimation on angle {
                            NumberAnimation { from: 0; to: 75; duration: 2000 }
                            NumberAnimation { from: 75; to: -75; duration: 4000 }
                            NumberAnimation { from: -75; to: 0; duration: 2000 }
                            loops: Animation.Infinite
                        }
                    }
                }
            }

            Rectangle {
                border.color: "purple"
                color: "transparent"
                width: 200
                height: 100
                Shape {
                    anchors.fill: parent
                    ShapePath {
                        id: someCurve
                        property color sc: "gray"
                        strokeColor: sc
                        property color fc: "yellow"
                        fillColor: fc
                        startX: 20; startY: 10
                        PathQuad { x: 50; y: 80; controlX: 0; controlY: 80 }
                        PathLine { x: 150; y: 80 }
                        PathQuad { x: 180; y: 10; controlX: 200; controlY: 80 }
                        PathLine { x: 20; y: 10 }
                        // Dynamic changes via property bindings etc. all work but can
                        // be computationally expense with the generic backend for properties
                        // that need retriangulating on every change. Should be cheap with NVPR.
                        NumberAnimation on strokeWidth {
                            from: 1; to: 20; duration: 10000
                        }
                    }
                }
                // Changing colors for a solid stroke or fill is simple and
                // (relatively) cheap. However, changing to/from transparent
                // stroke/fill color and stroke width 0 are special as these
                // change the scenegraph node tree (with the generic backend).
                Timer {
                    interval: 2000
                    running: true
                    repeat: true
                    onTriggered: someCurve.fillColor = (someCurve.fillColor === someCurve.fc ? "transparent" : someCurve.fc)
                }
                Timer {
                    interval: 1000
                    running: true
                    repeat: true
                    onTriggered: someCurve.strokeColor = (someCurve.strokeColor === someCurve.sc ? "transparent" : someCurve.sc)
                }
            }

            Rectangle {
                border.color: "purple"
                color: "transparent"
                width: 300
                height: 100
                Shape {
                    id: linesAndMoves
                    anchors.fill: parent
                    ShapePath {
                        strokeColor: "black"
                        startX: 0; startY: 50
                        PathLine { relativeX: 100; y: 50 }
                        PathMove { relativeX: 100; y: 50 }
                        PathLine { relativeX: 100; y: 50 }
                    }
                }
            }

            Rectangle {
                border.color: "purple"
                color: "transparent"
                width: 200
                height: 120
                Shape {
                    anchors.fill: parent
                    ShapePath {
                        id: joinTest
                        strokeColor: "black"
                        strokeWidth: 16
                        fillColor: "transparent"
                        capStyle: ShapePath.RoundCap
                        startX: 30
                        startY: 30
                        PathLine { x: 100; y: 100 }
                        PathLine { x: 30; y: 100 }
                    }
                }
                Timer {
                    interval: 1000
                    repeat: true
                    running: true
                    property variant styles: [ ShapePath.BevelJoin, ShapePath.MiterJoin, ShapePath.RoundJoin ]
                    onTriggered: {
                        for (var i = 0; i < styles.length; ++i)
                            if (styles[i] === joinTest.joinStyle) {
                                joinTest.joinStyle = styles[(i + 1) % styles.length];
                                break;
                            }
                    }
                }
            }

            Rectangle {
                border.color: "purple"
                color: "transparent"
                width: 200
                height: 100
                Shape {
                    anchors.fill: parent
                    ShapePath {
                        id: star
                        strokeColor: "blue"
                        fillColor: "lightGray"
                        strokeWidth: 2
                        PathMove { x: 90; y: 50 }
                        PathLine { x: 50 + 40 * Math.cos(0.8 * 1 * Math.PI); y: 50 + 40 * Math.sin(0.8 * 1 * Math.PI) }
                        PathLine { x: 50 + 40 * Math.cos(0.8 * 2 * Math.PI); y: 50 + 40 * Math.sin(0.8 * 2 * Math.PI) }
                        PathLine { x: 50 + 40 * Math.cos(0.8 * 3 * Math.PI); y: 50 + 40 * Math.sin(0.8 * 3 * Math.PI) }
                        PathLine { x: 50 + 40 * Math.cos(0.8 * 4 * Math.PI); y: 50 + 40 * Math.sin(0.8 * 4 * Math.PI) }
                        PathLine { x: 90; y: 50 }
                    }
                }
                Timer {
                    interval: 1000
                    onTriggered: star.fillRule = (star.fillRule === ShapePath.OddEvenFill ? ShapePath.WindingFill : ShapePath.OddEvenFill)
                    repeat: true
                    running: true
                }
            }

            Rectangle {
                border.color: "purple"
                color: "transparent"
                width: 200
                height: 100
                Shape {
                    anchors.fill: parent
                    ShapePath {
                        strokeWidth: 4
                        strokeColor: "black"
                        fillColor: "transparent"
                        startX: 20; startY: 10
                        PathCubic {
                            id: cb
                            x: 180; y: 10
                            control1X: -10; control1Y: 90; control2Y: 90
                            NumberAnimation on control2X { from: 400; to: 0; duration: 5000; loops: Animation.Infinite }
                        }
                    }
                }
            }
        }

        Column {
            spacing: 20

            Rectangle {
                border.color: "purple"
                color: "transparent"
                width: 200
                height: 100
                Shape {
                    anchors.fill: parent
                    ShapePath {
                        fillColor: "transparent"
                        strokeColor: "red"
                        strokeWidth: 4
                        startX: 10; startY: 40
                        PathArc {
                            x: 10; y: 60
                            radiusX: 40; radiusY: 40
                            useLargeArc: true
                        }
                    }
                }
            }

            Rectangle {
                border.color: "purple"
                color: "transparent"
                width: 200
                height: 200
                Rectangle {
                    anchors.centerIn: parent
                    // have a size smaller than 150x150
                    width: 100
                    height: 100
                    // and enable clipping. Normally this goes via scissoring, unless
                    // some transform triggers the stencil-based path. Ensure this via rotation.
                    clip: true
                    NumberAnimation on rotation {
                        from: 0; to: 360; duration: 5000; loops: Animation.Infinite
                    }

                    Shape {
                        width: 150
                        height: 150

                        ShapePath {
                            fillColor: "blue"
                            strokeColor: "red"
                            strokeWidth: 4
                            startX: 10; startY: 10
                            PathLine { x: 140; y: 140 }
                            PathLine { x: 10; y: 140 }
                            PathLine { x: 10; y: 10 }
                        }
                    }
                }
            }

            // stencil clip test #2, something more complicated:
            Rectangle {
                border.color: "purple"
                color: "transparent"
                width: 150
                height: 150
                Rectangle {
                    anchors.centerIn: parent
                    width: 60
                    height: 60
                    clip: true
                    NumberAnimation on rotation {
                        from: 0; to: 360; duration: 5000; loops: Animation.Infinite
                    }
                    Shape {
                        width: 100
                        height: 100
                        ShapePath {
                            id: clippedStar
                            strokeColor: "blue"
                            fillColor: "lightGray"
                            strokeWidth: 2
                            PathMove { x: 90; y: 50 }
                            PathLine { x: 50 + 40 * Math.cos(0.8 * 1 * Math.PI); y: 50 + 40 * Math.sin(0.8 * 1 * Math.PI) }
                            PathLine { x: 50 + 40 * Math.cos(0.8 * 2 * Math.PI); y: 50 + 40 * Math.sin(0.8 * 2 * Math.PI) }
                            PathLine { x: 50 + 40 * Math.cos(0.8 * 3 * Math.PI); y: 50 + 40 * Math.sin(0.8 * 3 * Math.PI) }
                            PathLine { x: 50 + 40 * Math.cos(0.8 * 4 * Math.PI); y: 50 + 40 * Math.sin(0.8 * 4 * Math.PI) }
                            PathLine { x: 90; y: 50 }
                        }
                    }
                    Timer {
                        interval: 1000
                        onTriggered: clippedStar.fillRule = (clippedStar.fillRule === ShapePath.OddEvenFill ? ShapePath.WindingFill : ShapePath.OddEvenFill)
                        repeat: true
                        running: true
                    }
                }
            }

            Rectangle {
                border.color: "purple"
                color: "transparent"
                width: 100
                height: 100
                Shape {
                    anchors.fill: parent
                    ShapePath {
                        strokeColor: "red"
                        PathLine { x: 100; y: 100 }
                    }
                    ShapePath {
                        strokeColor: "blue"
                        startX: 100; startY: 0
                        PathLine { x: 0; y: 100 }
                    }
                }
            }

            Rectangle {
                border.color: "purple"
                color: "transparent"
                width: 200
                height: 100
                Shape {
                    anchors.fill: parent
                    ShapePath {
                        strokeWidth: -1
                        strokeColor: "red"
                        fillGradient: RadialGradient {
                            centerX: 100; centerY: 50
                            focalX: centerX; focalY: centerY
                            centerRadius: 50
                            spread: RadialGradient.ReflectSpread
                            GradientStop { position: 0; color: "blue" }
                            GradientStop { position: 0.2; color: "green" }
                            GradientStop { position: 0.4; color: "red" }
                            GradientStop { position: 0.6; color: "yellow" }
                            GradientStop { position: 1; color: "cyan" }
                        }
                        PathLine { x: 0; y: 100 }
                        PathLine { x: 200; y: 100 }
                        PathLine { x: 200; y: 0 }
                        PathLine { x: 0; y: 0 }
                    }
                }
            }

            Rectangle {
                border.color: "purple"
                color: "transparent"
                width: 200
                height: 100
                Shape {
                    anchors.fill: parent
                    ShapePath {
                        strokeWidth: -1
                        strokeColor: "red"
                        fillGradient: ConicalGradient {
                            centerX: 100; centerY: 50
                            angle: 90
                            GradientStop { position: 0; color: "blue" }
                            GradientStop { position: 0.2; color: "green" }
                            GradientStop { position: 0.4; color: "red" }
                            GradientStop { position: 0.6; color: "yellow" }
                            GradientStop { position: 1; color: "cyan" }
                        }
                        PathLine { x: 0; y: 100 }
                        PathLine { x: 200; y: 100 }
                        PathLine { x: 200; y: 0 }
                        PathLine { x: 0; y: 0 }
                    }
                }
            }
        }
    }

    Rectangle {
        id: stackTestRect
        SequentialAnimation on opacity {
            NumberAnimation { from: 0; to: 1; duration: 5000 }
            PauseAnimation { duration: 2000 }
            NumberAnimation { from: 1; to: 0; duration: 5000 }
            PauseAnimation { duration: 2000 }
            loops: Animation.Infinite
            id: opAnim
        }
        color: "blue"
        anchors.margins: 10
        anchors.fill: parent
    }
    MouseArea {
        anchors.fill: parent
        onClicked: stackTestRect.visible = !stackTestRect.visible
    }
}
