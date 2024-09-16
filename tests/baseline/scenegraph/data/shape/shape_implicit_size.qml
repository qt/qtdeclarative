import QtQuick
import QtQuick.Shapes

Item {
    width: 320
    height: 480

    ListModel {
        id: renderers
        ListElement { renderer: Shape.GeometryRenderer }
        ListElement { renderer: Shape.CurveRenderer }
    }


    component ExampleShape : Shape {
        preferredRendererType: renderer
        ShapePath {
            strokeWidth: 2
            strokeColor: "black"
            fillColor: "#77ff99"

            startX: 30; startY: 30
            PathQuad {
                x: 100; y: 100
                controlX: 30; controlY: 100
            }
        }
        ShapePath {
            fillColor: "gray"
            strokeColor: "transparent"
            PathText {
                x: 30
                y: 50
                font.family: "Arial"
                font.pixelSize: 64
                text: "Test"
            }
        }
    }

    component ShowRect: Rectangle {
        color: "transparent"
        border.color: "blue"
        border.width: 2
        radius: 10
    }

    Row {
        x: 10
        y: 10
        Repeater {
            model: renderers
            Column {
                Item {
                    width: 150
                    height: 150
                    ExampleShape {
                        id: testSize
                    }
                    ShowRect {
                        anchors.fill: testSize
                    }
                }
                Item {
                    width: 150
                    height: 150
                    ExampleShape {
                        id: testBoundingRect
                    }
                    ShowRect {
                        x: testBoundingRect.boundingRect.x
                        y: testBoundingRect.boundingRect.y
                        width: testBoundingRect.boundingRect.width
                        height: testBoundingRect.boundingRect.height
                    }
                }
                Item {
                    width: 150
                    height: 150
                    ExampleShape {
                        id: testImplicitSize
                    }
                    ShowRect {
                        width: testImplicitSize.implicitWidth
                        height: testImplicitSize.implicitHeight
                    }
                }
            }
        }
    }
}
