// Copyright (C) 2025 The Qt Company Ltd.
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
        ListElement { scaleToFit: 0.6; offsetX: 20; offsetY: 20;         pathString: "M 0 100 L 50 100 L 75 0 L 100 100 L 150 100" }
        ListElement { scaleToFit: 0.6; offsetX: 20; offsetY: 20;         pathString: "M 150 100 L 100 100 L 75 0 L 50 100 L 0 100" }
        ListElement { scaleToFit: 0.6; offsetX: 20; offsetY: 20;         pathString: "M 100 0 L 100 50 L 0 75 L 100 100 L 100 150" }
        ListElement { scaleToFit: 0.6; offsetX: 20; offsetY: 20;         pathString: "M 100 150 L 100 100 L 0 75 L 100 50 L 100 0" }
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
                                        strokeColor: "brown"
                                        strokeWidth: 3
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
