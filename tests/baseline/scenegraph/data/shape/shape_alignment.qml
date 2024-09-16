import QtQuick
import QtQuick.Shapes

Item {
    width: 400
    height: 600

    ListModel {
        id: renderers
        ListElement { renderer: Shape.GeometryRenderer }
        ListElement { renderer: Shape.CurveRenderer }
    }

    component ExampleShape : Shape {
        anchors.fill: parent
        preferredRendererType: renderer
        ShapePath {
            strokeWidth: 4
            strokeColor: "black"
            fillColor: "gray"

            startX: 0; startY: 0
            PathLine { x: 100; y: 0 }
            PathLine { x: 100; y: 30 }
            PathLine { x: 0; y: 30 }
            PathLine { x: 0; y: 0 }
        }
        ShapePath {
            strokeWidth: 2
            strokeColor: "yellow"
            fillColor: "transparent"
            startX: 0; startY: 0
            PathLine { x: 100; y: 30 }
        }
        z: -1
    }

    component ParentRect: Rectangle {
        width: 150
        height: 100
        color: "transparent"
        border.color: "blue"
        border.width: 2
        radius: 10
    }

    Row {
        x: 10
        y: 10
        spacing: 20
        Repeater {
            model: renderers
            Column {
                spacing: 10
                ParentRect {
                    ExampleShape {
                    }
                }
                ParentRect {
                    ExampleShape {
                        horizontalAlignment: Shape.AlignHCenter
                        verticalAlignment: Shape.AlignBottom
                    }
                }
                ParentRect {
                    ExampleShape {
                        horizontalAlignment: Shape.AlignHCenter
                        verticalAlignment: Shape.AlignVCenter
                        fillMode: Shape.PreserveAspectFit
                    }
                }
                ParentRect {
                    clip: true
                    ExampleShape {
                        horizontalAlignment: Shape.AlignHCenter
                        verticalAlignment: Shape.AlignVCenter
                        fillMode: Shape.PreserveAspectCrop
                    }
                }
                ParentRect {
                    ExampleShape {
                        horizontalAlignment: Shape.AlignHCenter
                        verticalAlignment: Shape.AlignBottom
                        fillMode: Shape.Stretch
                    }
                }
            }
        }
    }
}
