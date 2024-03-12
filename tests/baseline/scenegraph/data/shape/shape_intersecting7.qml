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
        ListElement { scaleToFit: 0.08; offsetX: 20; offsetY: 20; pathString: "M 150 100 L 1250 150 L 100 1000 L 150 100 M 600 1200 Q 700 600 800 1200 Q 700 700 600 1200 M 100 1000 Q 200 1500 700 1500 Q 1200 1500 1200 1000 Q 1100 700 800 600 L 800 800 Q 1000 600 1000 1000 Q 1000 1300 700 1300 Q 400 1300 400 500 L 100 1000 " }
        ListElement { scaleToFit: 1; offsetX: 20; offsetY: 20; pathString: "M 0 0 Q 50 -10 100 0 Q 110 50 100 100 Q 50 110 0 100 Q -10 50 0 0 M 10 10 Q 50 0 90 10 Q 100 50 90 90 Q 50 100 10 90 Q 0 50 10 10 M 20 20 Q 50 10 80 20 Q 100 50 80 80 Q 50 100 20 80 Q 0 50 20 20" }
        ListElement { scaleToFit: 1; offsetX: 20; offsetY: 20; pathString: "M 0 0 Q 50 -10 100 0 Q 110 50 100 100 Q 50 110 0 100 Q -10 50 0 0 M 10 10 Q 0 50 10 90 Q 50 100 90 90 Q 100 50 90 10 Q 50 0 10 10 M 20 20 Q 50 10 80 20 Q 100 50 80 80 Q 50 100 20 80 Q 0 50 20 20" }
        ListElement { scaleToFit: 1; offsetX: 20; offsetY: 20; pathString: "M 0 0 Q 50 -10 100 0 Q 110 50 100 100 Q 50 110 0 100 Q -10 50 0 0 M 10 10 Q 50 0 90 10 Q 100 50 90 90 Q 50 100 10 90 Q 0 50 10 10 M 20 20 Q 10 50 20 80 Q 50 100 80 80 Q 100 50 80 20 Q 50 0 20 20" }
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
