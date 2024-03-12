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
        ListElement { scaleToFit: 0.11; offsetX: 20; offsetY: 20; pathString: "M 885.276 1047.5 Q 1310 -265 722.654 1071.85 Q 1100 1100 636.857 929.582 Q 100 1100 322.456 883.006 L 1003.49 134.715 Q 944.382 591.106 0 0 L 885.276 1047.5"}
        ListElement { scaleToFit: 0.035; offsetX: -60; offsetY: 0; pathString: "M 5000 3475 Q 5600 825 2290 245 Q 3635 -30 5360 3885 Q 730 2990 4685 390 L 2445 1435 L 5000 3475" }
        ListElement { scaleToFit: 0.11; offsetX: 10; offsetY: 10; pathString: "M 600 200 Q 1000 600 600 1000 Q 200 1200 200 600 Q 200 200 600 200 M 1000 200 Q 1400 600 1000 1000 Q 600 1200 600 600 Q 200 -200 1000 200" }
        ListElement { scaleToFit: 0.11; offsetX: -10; offsetY: 10; pathString: "M 865 270 Q 1000 600 955 820 Q 675 690 775 525 Q 200 200 865 270 M 1000 200 Q 1400 600 1000 1000 Q 600 1200 590 635 Q 200 -200 1000 200" }
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
