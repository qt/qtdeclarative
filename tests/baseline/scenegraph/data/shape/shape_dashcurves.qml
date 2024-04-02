import QtQuick 2.9
import QtQuick.Shapes 6.6

Item {
    id: root
    width: 320
    height: 480

    ListModel {
        id: renderers
        ListElement { renderer: Shape.GeometryRenderer }
        ListElement { renderer: Shape.CurveRenderer }
    }

    Row {
        Repeater {
            model: renderers
            Item {
                width: root.width / 2
                height: root.height

                Rectangle {
                    x: 10
                    y: 5
                    width : 140
                    height: 470
                    color: "yellow"
                }

                Shape {
                    anchors.fill: parent
                    preferredRendererType: renderer

                    ShapePath {
                        startY: 5
                        strokeWidth: 1
                        dashOffset: 0
                        strokeColor: "blue"
                        fillColor: "transparent"
                        startX: 10
                        strokeStyle: ShapePath.DashLine
                        PathQuad { relativeControlX: 30; relativeControlY: 30; relativeX: 140; relativeY: 0 }
                    }

                    ShapePath {
                        startY: 10.5
                        strokeWidth: 1
                        dashOffset: 0
                        strokeColor: "blue"
                        fillColor: "transparent"
                        startX: 10
                        strokeStyle: ShapePath.DashLine
                        PathQuad { relativeControlX: 30; relativeControlY: 30; relativeX: 140; relativeY: 0 }
                    }

                    ShapePath {
                        startY: 15
                        strokeWidth: 1
                        dashOffset: 0
                        capStyle: ShapePath.FlatCap
                        strokeColor: "blue"
                        fillColor: "transparent"
                        startX: 10
                        strokeStyle: ShapePath.DashLine
                        PathQuad { relativeControlX: 30; relativeControlY: 30; relativeX: 140; relativeY: 0 }
                    }

                    ShapePath {
                        startY: 20
                        strokeWidth: 3
                        dashOffset: 0
                        strokeColor: "blue"
                        fillColor: "transparent"
                        startX: 10
                        strokeStyle: ShapePath.DashLine
                        PathQuad { relativeControlX: 30; relativeControlY: 30; relativeX: 140; relativeY: 0 }
                    }

                    ShapePath {
                        startY: 25
                        strokeWidth: 3
                        dashOffset: 0
                        capStyle: ShapePath.RoundCap
                        strokeColor: "blue"
                        fillColor: "transparent"
                        startX: 10
                        strokeStyle: ShapePath.DashLine
                        PathQuad { relativeControlX: 30; relativeControlY: 30; relativeX: 140; relativeY: 0 }
                    }

                    ShapePath {
                        startY: 30
                        strokeWidth: 3
                        dashOffset: 0
                        capStyle: ShapePath.FlatCap
                        strokeColor: "blue"
                        fillColor: "transparent"
                        startX: 10
                        strokeStyle: ShapePath.DashLine
                        PathQuad { relativeControlX: 30; relativeControlY: 30; relativeX: 140; relativeY: 0 }
                    }

                    ShapePath {
                        startY: 40
                        strokeWidth: 3
                        dashOffset: 0.5
                        capStyle: ShapePath.RoundCap
                        strokeColor: "blue"
                        fillColor: "transparent"
                        startX: 10
                        strokeStyle: ShapePath.DashLine
                        PathQuad { relativeControlX: 30; relativeControlY: 30; relativeX: 140; relativeY: 0 }
                    }

                    ShapePath {
                        startY: 45
                        strokeWidth: 3
                        dashOffset: 2
                        capStyle: ShapePath.RoundCap
                        strokeColor: "blue"
                        fillColor: "transparent"
                        startX: 10
                        strokeStyle: ShapePath.DashLine
                        PathQuad { relativeControlX: 30; relativeControlY: 30; relativeX: 140; relativeY: 0 }
                    }

                    ShapePath {
                        startY: 50
                        strokeWidth: 3
                        dashOffset: 4
                        capStyle: ShapePath.RoundCap
                        strokeColor: "blue"
                        fillColor: "transparent"
                        startX: 10
                        strokeStyle: ShapePath.DashLine
                        PathQuad { relativeControlX: 30; relativeControlY: 30; relativeX: 140; relativeY: 0 }
                    }

                    ShapePath {
                        startY: 55
                        strokeWidth: 3
                        dashOffset: 100.7
                        capStyle: ShapePath.RoundCap
                        strokeColor: "blue"
                        fillColor: "transparent"
                        startX: 10
                        strokeStyle: ShapePath.DashLine
                        PathQuad { relativeControlX: 30; relativeControlY: 30; relativeX: 140; relativeY: 0 }
                    }

                    ShapePath {
                        startY: 60
                        strokeWidth: 3
                        dashOffset: 0
                        capStyle: ShapePath.FlatCap
                        strokeColor: "blue"
                        fillColor: "transparent"
                        startX: 10
                        strokeStyle: ShapePath.DashLine
                        PathQuad { relativeControlX: 30; relativeControlY: 30; relativeX: 140; relativeY: 0 }
                    }

                    ShapePath {
                        startY: 65
                        strokeWidth: 3
                        dashOffset: 6
                        capStyle: ShapePath.FlatCap
                        strokeColor: "blue"
                        fillColor: "transparent"
                        startX: 10
                        strokeStyle: ShapePath.DashLine
                        PathQuad { relativeControlX: 30; relativeControlY: 30; relativeX: 140; relativeY: 0 }
                    }

                    ShapePath {
                        startY: 70
                        strokeWidth: 3
                        dashOffset: -6
                        capStyle: ShapePath.FlatCap
                        strokeColor: "blue"
                        fillColor: "transparent"
                        startX: 10
                        strokeStyle: ShapePath.DashLine
                        PathQuad { relativeControlX: 30; relativeControlY: 30; relativeX: 140; relativeY: 0 }
                    }

                    ShapePath {
                        startY: 75
                        strokeWidth: 3
                        dashOffset: -2.5
                        capStyle: ShapePath.FlatCap
                        strokeColor: "blue"
                        fillColor: "transparent"
                        startX: 10
                        strokeStyle: ShapePath.DashLine
                        PathQuad { relativeControlX: 30; relativeControlY: 30; relativeX: 140; relativeY: 0 }
                    }

                    ShapePath {
                        startY: 85
                        strokeWidth: 3
                        dashPattern: [1, 1]
                        capStyle: ShapePath.FlatCap
                        strokeColor: "blue"
                        fillColor: "transparent"
                        startX: 10
                        strokeStyle: ShapePath.DashLine
                        PathQuad { relativeControlX: 30; relativeControlY: 30; relativeX: 140; relativeY: 0 }
                    }

                    ShapePath {
                        startY: 90
                        strokeWidth: 3
                        dashPattern: [1.3, 1]
                        capStyle: ShapePath.FlatCap
                        strokeColor: "blue"
                        fillColor: "transparent"
                        startX: 10
                        strokeStyle: ShapePath.DashLine
                        PathQuad { relativeControlX: 30; relativeControlY: 30; relativeX: 140; relativeY: 0 }
                    }

                    ShapePath {
                        startY: 95
                        strokeWidth: 3
                        dashPattern: [1, 2]
                        capStyle: ShapePath.FlatCap
                        strokeColor: "blue"
                        fillColor: "transparent"
                        startX: 10
                        strokeStyle: ShapePath.DashLine
                        PathQuad { relativeControlX: 30; relativeControlY: 30; relativeX: 140; relativeY: 0 }
                    }

                    ShapePath {
                        startY: 100
                        strokeWidth: 3
                        dashPattern: [2, 1]
                        capStyle: ShapePath.FlatCap
                        strokeColor: "blue"
                        fillColor: "transparent"
                        startX: 10
                        strokeStyle: ShapePath.DashLine
                        PathQuad { relativeControlX: 30; relativeControlY: 30; relativeX: 140; relativeY: 0 }
                    }

                    ShapePath {
                        startY: 105
                        strokeWidth: 3
                        dashPattern: [2, 2]
                        capStyle: ShapePath.FlatCap
                        strokeColor: "blue"
                        fillColor: "transparent"
                        startX: 10
                        strokeStyle: ShapePath.DashLine
                        PathQuad { relativeControlX: 30; relativeControlY: 30; relativeX: 140; relativeY: 0 }
                    }

                    ShapePath {
                        startY: 110
                        strokeWidth: 3
                        dashPattern: [4, 2, 2, 3, 5, 1]
                        capStyle: ShapePath.FlatCap
                        strokeColor: "blue"
                        fillColor: "transparent"
                        startX: 10
                        strokeStyle: ShapePath.DashLine
                        PathQuad { relativeControlX: 30; relativeControlY: 30; relativeX: 140; relativeY: 0 }
                    }

                    ShapePath {
                        startY: 115
                        strokeWidth: 3
                        dashPattern: [1, 2, 3, 4]
                        capStyle: ShapePath.FlatCap
                        strokeColor: "blue"
                        fillColor: "transparent"
                        startX: 10
                        strokeStyle: ShapePath.DashLine
                        PathQuad { relativeControlX: 30; relativeControlY: 30; relativeX: 140; relativeY: 0 }
                    }

                    ShapePath {
                        startY: 120
                        strokeWidth: 3
                        dashPattern: [1, 2, 3, 4]
                        dashOffset: 1
                        capStyle: ShapePath.FlatCap
                        strokeColor: "blue"
                        fillColor: "transparent"
                        startX: 10
                        strokeStyle: ShapePath.DashLine
                        PathQuad { relativeControlX: 30; relativeControlY: 30; relativeX: 140; relativeY: 0 }
                    }

                    ShapePath {
                        startY: 125
                        strokeWidth: 3
                        dashPattern: [1, 2, 3, 4]
                        dashOffset: 3
                        capStyle: ShapePath.FlatCap
                        strokeColor: "blue"
                        fillColor: "transparent"
                        startX: 10
                        strokeStyle: ShapePath.DashLine
                        PathQuad { relativeControlX: 30; relativeControlY: 30; relativeX: 140; relativeY: 0 }
                    }

                    ShapePath {
                        startY: 130
                        strokeWidth: 3
                        dashPattern: [1, 2, 3, 4]
                        dashOffset: 6
                        capStyle: ShapePath.FlatCap
                        strokeColor: "blue"
                        fillColor: "transparent"
                        startX: 10
                        strokeStyle: ShapePath.DashLine
                        PathQuad { relativeControlX: 30; relativeControlY: 30; relativeX: 140; relativeY: 0 }
                    }

                    ShapePath {
                        startY: 135
                        strokeWidth: 3
                        dashPattern: [1, 2, 3, 4]
                        dashOffset: 10
                        capStyle: ShapePath.FlatCap
                        strokeColor: "blue"
                        fillColor: "transparent"
                        startX: 10
                        strokeStyle: ShapePath.DashLine
                        PathQuad { relativeControlX: 30; relativeControlY: 30; relativeX: 140; relativeY: 0 }
                    }


                    ShapePath {
                        startY: 150
                        strokeWidth: 7
                        dashPattern: [3, 2]
                        capStyle: ShapePath.RoundCap
                        strokeColor: "blue"
                        fillColor: "transparent"
                        startX: 10
                        strokeStyle: ShapePath.DashLine
                        PathQuad { relativeControlX: 30; relativeControlY: 30; relativeX: 140; relativeY: 0 }
                    }

                    ShapePath {
                        startY: 160
                        strokeWidth: 7
                        dashPattern: [3, 1.5]
                        capStyle: ShapePath.RoundCap
                        strokeColor: "blue"
                        fillColor: "transparent"
                        startX: 10
                        strokeStyle: ShapePath.DashLine
                        PathQuad { relativeControlX: 30; relativeControlY: 30; relativeX: 140; relativeY: 0 }
                    }

                    ShapePath {
                        startY: 170
                        strokeWidth: 7
                        dashPattern: [3, 1]
                        capStyle: ShapePath.RoundCap
                        strokeColor: "blue"
                        fillColor: "transparent"
                        startX: 10
                        strokeStyle: ShapePath.DashLine
                        PathQuad { relativeControlX: 30; relativeControlY: 30; relativeX: 140; relativeY: 0 }
                    }

                    ShapePath {
                        startY: 180
                        strokeWidth: 7
                        dashPattern: [3, 0.7]
                        capStyle: ShapePath.RoundCap
                        strokeColor: "blue"
                        fillColor: "transparent"
                        startX: 10
                        strokeStyle: ShapePath.DashLine
                        PathQuad { relativeControlX: 30; relativeControlY: 30; relativeX: 140; relativeY: 0 }
                    }

                    ShapePath {
                        startY: 190
                        strokeWidth: 7
                        dashPattern: [3, 0.2]
                        capStyle: ShapePath.RoundCap
                        strokeColor: "blue"
                        fillColor: "transparent"
                        startX: 10
                        strokeStyle: ShapePath.DashLine
                        PathQuad { relativeControlX: 30; relativeControlY: 30; relativeX: 140; relativeY: 0 }
                    }

                    ShapePath {
                        startY: 200
                        strokeWidth: 7
                        dashPattern: [3, 0]
                        capStyle: ShapePath.RoundCap
                        strokeColor: "blue"
                        fillColor: "transparent"
                        startX: 10
                        strokeStyle: ShapePath.DashLine
                        PathQuad { relativeControlX: 30; relativeControlY: 30; relativeX: 140; relativeY: 0 }
                    }

                    ShapePath {
                        // QTBUG-123805
                        startY: 210
                        strokeWidth: 5.8575662686300545;
                        dashPattern: [2, 2]
                        dashOffset: 4
                        capStyle: ShapePath.FlatCap
                        strokeColor: "blue"
                        fillColor: "transparent"
                        startX: 10
                        strokeStyle: ShapePath.DashLine
                        PathQuad { relativeControlX: 30; relativeControlY: 30; relativeX: 140; relativeY: 0 }
                    }

                    ShapePath {
                        startY: 220
                        strokeWidth: 3
                        dashPattern: [3, 3, 2]
                        capStyle: ShapePath.FlatCap
                        strokeColor: "blue"
                        fillColor: "transparent"
                        startX: 10
                        strokeStyle: ShapePath.DashLine
                        PathQuad { relativeControlX: 30; relativeControlY: 30; relativeX: 140; relativeY: 0 }
                    }

                    ShapePath {
                        startY: 225
                        strokeWidth: 3
                        dashPattern: [0, 3, 2]
                        capStyle: ShapePath.FlatCap
                        strokeColor: "blue"
                        fillColor: "transparent"
                        startX: 10
                        strokeStyle: ShapePath.DashLine
                        PathQuad { relativeControlX: 30; relativeControlY: 30; relativeX: 140; relativeY: 0 }
                    }

                    ShapePath {
                        startY: 230
                        strokeWidth: 3
                        dashPattern: [-3, 3, 2]
                        capStyle: ShapePath.FlatCap
                        strokeColor: "blue"
                        fillColor: "transparent"
                        startX: 10
                        strokeStyle: ShapePath.DashLine
                        PathQuad { relativeControlX: 30; relativeControlY: 30; relativeX: 140; relativeY: 0 }
                    }

                    ShapePath {
                        startY: 235
                        strokeWidth: 3
                        dashPattern: [-3, 3, 2]
                        dashOffset: 1
                        capStyle: ShapePath.FlatCap
                        strokeColor: "blue"
                        fillColor: "transparent"
                        startX: 10
                        strokeStyle: ShapePath.DashLine
                        PathQuad { relativeControlX: 30; relativeControlY: 30; relativeX: 140; relativeY: 0 }
                    }

                    ShapePath {
                        startY: 240
                        strokeWidth: 3
                        dashPattern: [-3, 3, 2]
                        dashOffset: -1
                        capStyle: ShapePath.FlatCap
                        strokeColor: "blue"
                        fillColor: "transparent"
                        startX: 10
                        strokeStyle: ShapePath.DashLine
                        PathQuad { relativeControlX: 30; relativeControlY: 30; relativeX: 140; relativeY: 0 }
                    }

                    ShapePath {
                        startY: 245
                        strokeWidth: 3
                        dashPattern: [-5, 2]
                        capStyle: ShapePath.FlatCap
                        strokeColor: "blue"
                        fillColor: "transparent"
                        startX: 10
                        strokeStyle: ShapePath.DashLine
                        PathQuad { relativeControlX: 30; relativeControlY: 30; relativeX: 140; relativeY: 0 }
                    }

                    ShapePath {
                        startY: 250
                        strokeWidth: 3
                        dashPattern: [5, -2]
                        capStyle: ShapePath.FlatCap
                        strokeColor: "blue"
                        fillColor: "transparent"
                        startX: 10
                        strokeStyle: ShapePath.DashLine
                        PathQuad { relativeControlX: 30; relativeControlY: 30; relativeX: 140; relativeY: 0 }
                    }

                    ShapePath {
                        startY: 255
                        strokeWidth: 3
                        dashPattern: [-5, -2]
                        capStyle: ShapePath.FlatCap
                        strokeColor: "blue"
                        fillColor: "transparent"
                        startX: 10
                        strokeStyle: ShapePath.DashLine
                        PathQuad { relativeControlX: 30; relativeControlY: 30; relativeX: 140; relativeY: 0 }
                    }

                    ShapePath {
                        startY: 260
                        strokeWidth: 3
                        dashPattern: [-5, -2, -4]
                        capStyle: ShapePath.FlatCap
                        strokeColor: "blue"
                        fillColor: "transparent"
                        startX: 10
                        strokeStyle: ShapePath.DashLine
                        PathQuad { relativeControlX: 30; relativeControlY: 30; relativeX: 140; relativeY: 0 }
                    }

                    ShapePath {
                        startY: 265
                        strokeWidth: 3
                        dashPattern: [-5, -2, 4]
                        capStyle: ShapePath.FlatCap
                        strokeColor: "blue"
                        fillColor: "transparent"
                        startX: 10
                        strokeStyle: ShapePath.DashLine
                        PathQuad { relativeControlX: 30; relativeControlY: 30; relativeX: 140; relativeY: 0 }
                    }
                }
            }
        }
    }
}
