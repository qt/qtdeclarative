// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Shapes

Item {
    width: 600
    height: 600

    ListModel {
        id: fillRules
        ListElement { fillrule: ShapePath.WindingFill }
        ListElement { fillrule: ShapePath.OddEvenFill }
    }

    ListModel {
        id: renderers
        ListElement { renderer: Shape.GeometryRenderer }
        ListElement { renderer: Shape.CurveRenderer }
    }

    ListModel {
        id: svgstrings
        ListElement { scaleToFit: 0.11; offsetX: 20; offsetY: 20; pathString: "M 200 200 Q 600 1000 1000 200 Q -100 200 200 1000 Q -100 1000 -100 500 Q -300 200 200 200 M 300 200 L 800 700 L 300 700 L 800 200 L 300 200"}
        ListElement { scaleToFit: 0.11; offsetX: 20; offsetY: 20; pathString: "M 200 200 Q 600 1000 1000 200 Q -100 200 200 1000 Q -100 1000 -100 500 Q -300 200 200 200 M 300 200 L 300 700 L 800 200 L 800 700 L 300 200"}
        ListElement { scaleToFit: 1; offsetX: 0; offsetY: 0; pathString: "M 128.84 103.6 L 112.71 68.78 L 64.32 19.15 L 144.33 94.34 L 47.33 98.82 Q 37.0 60.87 39.01 12.06 L 51.92 72.5 L 19.87 130.09 L 139.11 47.81 Q 110.47 70.31 113.71 87.09 "}
        ListElement { scaleToFit: 1; offsetX: 0; offsetY: 0; pathString: "M 85.23 92.08 Q 126.6 2.98 23.72 51.94 Z M 103.97 123.27 L 145.32 76.51 Z M 134.57 28.24 L 41.44 8.54 Z M 43.48 123.1 Q 58.58 39.75 92.72 144.17 Z M 23.86 116.3 Q 5.82 83.87 84.11 130.88"}
    }

    Column {
        Repeater {
            model: renderers
            Column {
                Repeater {
                    model: fillRules
                    Row {
                        Repeater {
                            model: svgstrings
                            Rectangle {
                                width: 150
                                height: 150
                                border.color: "black"

                                Shape {
                                    preferredRendererType: renderer
                                    ShapePath {
                                        fillColor: renderer == Shape.CurveRenderer ? "#99483d8b" : "#99dc143c"
                                        fillRule: fillrule
                                        strokeWidth: 0
                                        PathSvg { path: pathString }
                                    }

                                    transform: Matrix4x4 {
                                        matrix: Qt.matrix4x4(scaleToFit, 0, 0, offsetX,
                                                             0, scaleToFit, 0, offsetY,
                                                             0, 0, 1, 0,
                                                             0, 0, 0, 1)
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
