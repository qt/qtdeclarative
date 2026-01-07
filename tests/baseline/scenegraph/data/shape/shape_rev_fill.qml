// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Shapes

Item {
    width: 300
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

    property list<int> ys: [8, 7, 1, 9, 5, 7, 1, 8, 3, 6, 0, 5, 3, 9, 3, 4, 8, 4, 4, 5, 1, 7, 9, 0, 4, 6, 5, 4, 7, 1, 2, 0, 6, 7, 5, 9, 0, 4, 0, 7, 4, 1, 2, 8, 6, 0, 3, 7, 5, 1]
    property list<point> pts0: [Qt.point(100, 100), Qt.point(80, 100), Qt.point(80, 0), Qt.point(60, 0), Qt.point(60, 100), Qt.point(40, 100), Qt.point(40, 0), Qt.point(20, 100), Qt.point(0, 100)]
    property list<point> pts1: ys.map((y, i) => Qt.point(-i / 49 * 200, y * 20))
    ListModel {
        id: testPaths
        ListElement { scaleToFit: 1; offsetX: 20; offsetY: 20 }
        ListElement { scaleToFit: 0.6; offsetX: 135; offsetY: 20 }
    }

    Column {
        Repeater {
            model: renderers
            Column {
                Repeater {
                    model: fillRules
                    Row {
                        Repeater {
                            model: testPaths
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
                                        PathPolyline { path: index == 0 ? pts0 : pts1 }
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
