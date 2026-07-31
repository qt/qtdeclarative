// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Shapes
import QtQuick.Layouts

ApplicationWindow {
    id: window
    width: 720
    height: 560
    visible: true
    title: "ShapePath.visible renderer test"
    color: "lightgray"

    readonly property var boxes: [
        showTriangle, showSquare, showCubic, showRect, showArc, showRing,
        showStar, showZigzag, showMultiline, showCurve, showText
    ]

    function rendererName(type) {
        switch (type) {
        case Shape.GeometryRenderer: return "GeometryRenderer";
        case Shape.SoftwareRenderer: return "SoftwareRenderer";
        case Shape.CurveRenderer: return "CurveRenderer";
        default: return "UnknownRenderer";
        }
    }

    function setAll(on) {
        for (let box of boxes)
            box.checked = on;
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Label { text: "Preferred renderer:" }

            ComboBox {
                id: rendererBox
                textRole: "text"
                valueRole: "value"
                currentIndex: 0
                model: [
                    { text: "Default (backend's choice)", value: Shape.UnknownRenderer },
                    { text: "GeometryRenderer", value: Shape.GeometryRenderer },
                    { text: "CurveRenderer", value: Shape.CurveRenderer },
                    { text: "SoftwareRenderer", value: Shape.SoftwareRenderer }
                ]
            }

            Label {
                text: "in use: " + window.rendererName(shape.rendererType)
            }

            CheckBox {
                id: asyncBox
                text: "asynchronous"
            }

            Item { Layout.fillWidth: true }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 12

            Rectangle {
                Layout.preferredWidth: 240
                Layout.preferredHeight: 240
                Layout.alignment: Qt.AlignTop
                color: "white"
                border.color: "gray"

                Shape {
                    id: shape
                    anchors.fill: parent
                    preferredRendererType: rendererBox.currentValue
                    asynchronous: asyncBox.checked

                    // The ShapePaths below are declared back to front: #1 must always
                    // stay behind #2, #2 behind #3, and so on, no matter in which
                    // order they are toggled invisible and visible again.

                    // #1 Triangle: straight lines, solid fill.
                    ShapePath {
                        visible: showTriangle.checked
                        strokeColor: "black"
                        strokeWidth: 3
                        fillColor: "lightblue"
                        startX: 10; startY: 210
                        PathLine { x: 230; y: 210 }
                        PathLine { x: 120; y: 30 }
                        PathLine { x: 10; y: 210 }
                    }

                    // #2 Square: overlaps the triangle.
                    ShapePath {
                        visible: showSquare.checked
                        strokeColor: "black"
                        strokeWidth: 3
                        fillColor: "salmon"
                        startX: 70; startY: 70
                        PathLine { x: 170; y: 70 }
                        PathLine { x: 170; y: 170 }
                        PathLine { x: 70; y: 170 }
                        PathLine { x: 70; y: 70 }
                    }

                    // #3 Cubic blob: linear gradient fill.
                    ShapePath {
                        visible: showCubic.checked
                        strokeColor: "darkslateblue"
                        strokeWidth: 2
                        startX: 20; startY: 120
                        fillGradient: LinearGradient {
                            x1: 20; y1: 0; x2: 220; y2: 0
                            GradientStop { position: 0; color: "#8000c0ff" }
                            GradientStop { position: 1; color: "#80ff00c0" }
                        }
                        PathCubic {
                            x: 220; y: 120
                            control1X: 60; control1Y: 10
                            control2X: 180; control2Y: 230
                        }
                        PathCubic {
                            x: 20; y: 120
                            control1X: 180; control1Y: 180
                            control2X: 60; control2Y: 60
                        }
                    }

                    // #4 Rounded rectangle: per-corner radii, translucent fill.
                    ShapePath {
                        visible: showRect.checked
                        strokeColor: "darkgreen"
                        strokeWidth: 2
                        fillColor: "#6033cc33"
                        startX: 30; startY: 150
                        PathRectangle {
                            width: 180
                            height: 70
                            topLeftRadius: 30
                            bottomRightRadius: 30
                        }
                    }

                    // #5 Pie slice: PathArc plus closing lines.
                    ShapePath {
                        visible: showArc.checked
                        strokeColor: "saddlebrown"
                        strokeWidth: 2
                        fillColor: "#a0ffcc33"
                        startX: 120; startY: 120
                        PathLine { x: 220; y: 120 }
                        PathArc {
                            x: 155; y: 214
                            radiusX: 100; radiusY: 100
                            direction: PathArc.Clockwise
                        }
                        PathLine { x: 120; y: 120 }
                    }

                    // #6 Dashed ring: PathAngleArc, stroke only.
                    ShapePath {
                        visible: showRing.checked
                        strokeColor: "crimson"
                        strokeWidth: 6
                        fillColor: "transparent"
                        strokeStyle: ShapePath.DashLine
                        dashPattern: [3, 2]
                        capStyle: ShapePath.RoundCap
                        PathAngleArc {
                            centerX: 120; centerY: 120
                            radiusX: 105; radiusY: 105
                            startAngle: -110
                            sweepAngle: 300
                        }
                    }

                    // #7 Self-intersecting star: tests fillRule and radial gradients.
                    ShapePath {
                        visible: showStar.checked
                        strokeColor: "black"
                        strokeWidth: 2
                        fillRule: ShapePath.OddEvenFill
                        fillGradient: RadialGradient {
                            centerX: 120; centerY: 120; centerRadius: 100
                            GradientStop { position: 0; color: "white" }
                            GradientStop { position: 1; color: "gold" }
                        }
                        PathSvg { path: "M 120 25 L 176 195 L 30 90 L 210 90 L 64 195 Z" }
                    }

                    // #8 Zigzag polyline: thick stroke, no fill, round joins.
                    ShapePath {
                        visible: showZigzag.checked
                        strokeColor: "#303030"
                        strokeWidth: 8
                        fillColor: "transparent"
                        capStyle: ShapePath.RoundCap
                        joinStyle: ShapePath.RoundJoin
                        PathPolyline {
                            path: [Qt.point(15, 40), Qt.point(70, 90), Qt.point(125, 40),
                                   Qt.point(180, 90), Qt.point(225, 40)]
                        }
                    }

                    // #9 Two disjoint subpaths in one ShapePath.
                    ShapePath {
                        visible: showMultiline.checked
                        strokeColor: "mediumblue"
                        strokeWidth: 4
                        fillColor: "transparent"
                        PathMultiline {
                            paths: [
                                [Qt.point(20, 20), Qt.point(220, 220)],
                                [Qt.point(220, 20), Qt.point(20, 220)]
                            ]
                        }
                    }

                    // #10 Smooth Catmull-Rom curve with a conical gradient fill.
                    ShapePath {
                        visible: showCurve.checked
                        strokeColor: "teal"
                        strokeWidth: 3
                        startX: 25; startY: 200
                        fillGradient: ConicalGradient {
                            centerX: 120; centerY: 160; angle: 30
                            GradientStop { position: 0; color: "#8000ffc0" }
                            GradientStop { position: 1; color: "#80004060" }
                        }
                        PathCurve { x: 80; y: 120 }
                        PathCurve { x: 150; y: 190 }
                        PathCurve { x: 215; y: 110 }
                        PathCurve { x: 25; y: 200 }
                    }

                    // #11 Text outline: must stay in front of everything else.
                    ShapePath {
                        visible: showText.checked
                        strokeColor: "black"
                        strokeWidth: 1
                        fillColor: "white"
                        PathText {
                            x: 40; y: 100
                            text: "Shape"
                            font.pixelSize: 48
                            font.bold: true
                        }
                    }
                }
            }

            ColumnLayout {
                Layout.alignment: Qt.AlignTop
                spacing: 2

                Label { text: "Visible paths (top of list = drawn first / furthest back):"; font.bold: true }

                CheckBox { id: showTriangle; text: "1 - Triangle (PathLine)"; checked: true }
                CheckBox { id: showSquare; text: "2 - Square (PathLine)"; checked: true }
                CheckBox { id: showCubic; text: "3 - Cubic blob (linear gradient)" }
                CheckBox { id: showRect; text: "4 - Rounded rect (PathRectangle)" }
                CheckBox { id: showArc; text: "5 - Pie slice (PathArc)" }
                CheckBox { id: showRing; text: "6 - Dashed ring (PathAngleArc)" }
                CheckBox { id: showStar; text: "7 - Star (PathSvg, odd-even fill)" }
                CheckBox { id: showZigzag; text: "8 - Zigzag (PathPolyline)" }
                CheckBox { id: showMultiline; text: "9 - Cross (PathMultiline)" }
                CheckBox { id: showCurve; text: "10 - Catmull-Rom (conical gradient)" }
                CheckBox { id: showText; text: "11 - Text outline (PathText)" }

                RowLayout {
                    Layout.topMargin: 6
                    Button {
                        text: "All on"
                        onClicked: window.setAll(true)
                    }
                    Button {
                        text: "All off"
                        onClicked: window.setAll(false)
                    }
                    Button {
                        text: "Invert"
                        onClicked: {
                            for (let box of window.boxes)
                                box.checked = !box.checked;
                        }
                    }
                }
            }
        }

        Label {
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
            text: "Toggle paths off and on in a different order than they are declared. A path " +
                  "must always come back at its declared depth: a lower-numbered path drawn on " +
                  "top of a higher-numbered one means the ordering bug has regressed for the " +
                  "renderer selected above. Check every renderer; SoftwareRenderer is only " +
                  "actually used when the scenegraph runs with QT_QUICK_BACKEND=software."
        }
    }
}
