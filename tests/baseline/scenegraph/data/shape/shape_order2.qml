import QtQuick
import QtQuick.Shapes

Item {
    id: root
    width: 320
    height: 480

    property int counter: 0
    NumberAnimation {
        target: root
        property: "counter"
        duration: 150
        from: 0
        to: 1
        running: true
    }

    component MyShapes : Item {
        width: 100
        height: 200
        property bool async: false
        property int preferredRenderer: Shape.GeometryRenderer

        Shape {
            id: s1
            asynchronous: parent.async
            preferredRendererType: parent.preferredRenderer
            ShapePath {
                id: p1
                fillColor: "orange"
                strokeColor: counter < 1 ? "transparent" : "lime"
                strokeWidth: 10
                strokeStyle: ShapePath.DashLine
                PathAngleArc {
                    centerX: 50
                    centerY: 50
                    radiusX: 40
                    radiusY: 40
                    sweepAngle: 270
                }
            }
        }

        Shape {
            id: s2
            y: 100
            asynchronous: parent.async
            preferredRendererType: parent.preferredRenderer
            ShapePath {
                id: p2
                fillColor: counter < 1 ? "transparent" : "orange"
                strokeColor: "lime"
                strokeWidth: 10
                strokeStyle: ShapePath.DashLine
                PathAngleArc {
                    moveToStart: true
                    centerX: 50
                    centerY: 50
                    radiusX: 40
                    radiusY: 40
                    sweepAngle: 270
                }
            }
        }
    }

    ListModel {
        id: renderers
        ListElement { renderer: Shape.GeometryRenderer }
        ListElement { renderer: Shape.CurveRenderer }
    }

    Row {
        spacing: 25
        Repeater {
            model: renderers
            Column {
                spacing: 25
                MyShapes {
                    async: false
                    preferredRenderer: renderer
                }
                MyShapes {
                    async: true
                    preferredRenderer: renderer
                }
            }
        }
    }
}
