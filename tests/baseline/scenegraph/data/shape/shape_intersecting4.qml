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
        ListElement { scaleToFit: 1; offsetX: 0; offsetY: 0; pathString: "M 64.9 142.96 Q 4.68 9.31 7.25 121.42 Z M 5.3 86.37 Q 31.57 11.81 34.04 5.0 Q 25.26 47.39 31.31 87.97 Z M 4.2 17.35 L 33.27 124.0 Q 111.36 33.37 8.23 118.77 L 65.78 27.48 Q 139.84 87.06 96.03 23.08"}
        ListElement { scaleToFit: 1; offsetX: 0; offsetY: 0; pathString: "M 88.78 105.24 L 69.72 114.27 L 26.94 71.0 Q 62.05 122.52 83.1 22.44 Q 105.46 40.25 108.12 54.23 L 103.51 40.41 Q 70.16 41.09 29.88 96.16 Z M 60.49 96.92 L 14.58 112.34 Q 1.76 12.05 106.52 128.97"}
        // These don't work because there are subpaths with just two lines in it:
        // ListElement { scaleToFit: 1; offsetX: 0; offsetY: 0; pathString: "M 70.21 146.61 L 54.01 6.38 Z M 52.51 5.75 L 70.75 44.78 L 81.98 128.43 Z M 2.71 72.51 L 96.86 4.48 L 136.96 83.88 Z M 25.32 52.79 Q 3.59 111.11 125.15 137.23"}
        // ListElement { scaleToFit: 1; offsetX: 0; offsetY: 0; pathString: "M 131.61 147.43 L 4.5 6.5 L 110.44 40.71 Q 128.69 39.15 136.85 6.84 Q 8.9 139.71 76.35 103.86 L 77.47 43.63 L 127.06 97.65 L 14.97 66.38 Z M 47.53 0.08 L 42.02 33.25"}
        // Removing those lines fixes the problem. We might want to do it in the intersectionSolver at some point.
        ListElement { scaleToFit: 1; offsetX: 0; offsetY: 0; pathString: "M 52.51 5.75 L 70.75 44.78 L 81.98 128.43 Z M 2.71 72.51 L 96.86 4.48 L 136.96 83.88 Z M 25.32 52.79 Q 3.59 111.11 125.15 137.23"}
        ListElement { scaleToFit: 1; offsetX: 0; offsetY: 0; pathString: "M 131.61 147.43 L 4.5 6.5 L 110.44 40.71 Q 128.69 39.15 136.85 6.84 Q 8.9 139.71 76.35 103.86 L 77.47 43.63 L 127.06 97.65 L 14.97 66.38 Z"}
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
