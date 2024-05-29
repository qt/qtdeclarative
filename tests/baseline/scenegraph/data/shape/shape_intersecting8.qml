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
        ListElement { scaleToFit: 0.6; offsetX: 20; offsetY: 20;         pathString: "M 10 100 Q 10 89.6447 17.3223 82.3223 Q 24.6447 75 35 75 Q 45.3553 75 52.6777 82.3223 Q 60 89.6447 60 100 Q 60 85.5025 67.3223 75.2513 Q 74.6447 65 85 65 Q 95.3553 65 102.678 75.2513 Q 110 85.5025 110 100 Q 110 75.1472 117.322 57.5736 Q 124.645 40 135 40 Q 145.355 40 152.678 57.5736 Q 160 75.1472 160 100 Q 171.603 83.923 185 83.923 Q 198.397 83.923 210 100 L 10 100" }
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
