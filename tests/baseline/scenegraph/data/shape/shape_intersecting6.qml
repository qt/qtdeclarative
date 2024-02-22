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
        ListElement { scaleToFit: 1; offsetX: 20; offsetY: 20; pathString: "M 0 0 L 100 0 L 100 50 L 0 50 Z M 0 50 L 100 50 L 100 100 L 0 100 Z " }
        ListElement { scaleToFit: 1; offsetX: 20; offsetY: 20; pathString: "M 0 0 L 0 50 L 100 50 L 100 0 Z M 0 50 L 0 100 L 100 100 L 100 50 Z " }
        ListElement { scaleToFit: 1; offsetX: 20; offsetY: 20; pathString: "M 0 0 L 0 50 L 100 50 L 100 0 Z M 0 50 L 100 50 L 100 100 L 0 100 Z " }
        ListElement { scaleToFit: 1; offsetX: 20; offsetY: 40; pathString: "M 0 0 L 100 0 L 100 50 L 0 50 Z M 20 0 Q 20 -30 50 -30 Q 80 -30 80 0 L 80 70 Q 50 100 20 70 Z" }
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
