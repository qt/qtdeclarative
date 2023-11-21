import QtQuick
import QtQuick.Shapes

Item {
    id: root
    property bool async: false

    property int counter: 0
    NumberAnimation {
        target: root
        property: "counter"
        duration: 2000
        from: 0
        to: 15
        running: true
    }

    component RectStack : Shape {
        asynchronous: root.async

        property alias p1FillColor: p1.fillColor
        property alias p2FillColor: p2.fillColor
        property alias p3FillColor: p3.fillColor
        property alias p4FillColor: p4.fillColor
        property alias p1StrokeColor: p1.strokeColor
        property alias p2StrokeColor: p2.strokeColor
        property alias p3StrokeColor: p3.strokeColor
        property alias p4StrokeColor: p4.strokeColor

        ShapePath {
            id: p1
            fillColor: "black"
            strokeColor: "transparent"
            strokeWidth: 5
            startX: 10
            startY: 10
            PathLine { relativeX: 100; relativeY: 0 }
            PathLine { relativeX: 0; relativeY: 60 }
            PathLine { relativeX: -100; relativeY: 0 }
            PathLine { relativeX: 0; relativeY: -60 }
        }
        ShapePath {
            id: p2
            fillColor: "red"
            strokeColor: "transparent"
            strokeWidth: 5
            startX: 20
            startY: 15
            PathLine { relativeX: 100; relativeY: 0 }
            PathLine { relativeX: 0; relativeY: 60 }
            PathLine { relativeX: -100; relativeY: 0 }
            PathLine { relativeX: 0; relativeY: -60 }
        }
        ShapePath {
            id: p3
            fillColor: "green"
            strokeColor: "transparent"
            strokeWidth: 5
            startX: 30
            startY: 20
            PathLine { relativeX: 100; relativeY: 0 }
            PathLine { relativeX: 0; relativeY: 60 }
            PathLine { relativeX: -100; relativeY: 0 }
            PathLine { relativeX: 0; relativeY: -60 }
        }
        ShapePath {
            id: p4
            fillColor: "blue"
            strokeColor: "transparent"
            strokeWidth: 5
            startX: 40
            startY: 25
            PathLine { relativeX: 100; relativeY: 0 }
            PathLine { relativeX: 0; relativeY: 60 }
            PathLine { relativeX: -100; relativeY: 0 }
            PathLine { relativeX: 0; relativeY: -60 }
        }
    }


    ListModel {
        id: renderers
        ListElement { renderer: Shape.GeometryRenderer }
        ListElement { renderer: Shape.CurveRenderer }
    }

    Row {
        spacing: 5
        Repeater {
            model: renderers
            Column {
                spacing: 5

                RectStack {
                    preferredRendererType: renderer
                }

                RectStack {
                    preferredRendererType: renderer
                    p1FillColor: counter % 16 >= 8 ?  "black" : "transparent"
                    p2FillColor: counter % 8 >= 4 ?  "red" : "transparent"
                    p3FillColor: counter % 4 >= 2 ? "green" : "transparent"
                    p4FillColor: counter % 2 >= 1 ? "blue" : "transparent"
                }

                RectStack {
                    preferredRendererType: renderer
                    property int shifter: counter < 4 ? counter : counter + 1
                    p1FillColor: shifter % 2 == 0 ?  "black" : "transparent"
                    p2FillColor: counter % 2 == 0 ?  "red" : "transparent"
                    p3FillColor: shifter % 2 == 1 ? "green" : "transparent"
                    p4FillColor: counter % 2 == 1 ? "blue" : "transparent"
                }

                RectStack {
                    preferredRendererType: renderer
                    p1FillColor: counter % 16 >= 8 ?  "black" : "transparent"
                    p2FillColor: counter % 8 >= 4 ?  "red" : "transparent"
                    p3FillColor: counter % 4 >= 2 ? "green" : "transparent"
                    p4FillColor: counter % 2 >= 1 ? "blue" : "transparent"
                    p1StrokeColor: counter % 2 >= 1 ? "transparent" : "lightblue"
                    p2StrokeColor: counter % 4 >= 2 ? "transparent" : "lightgreen"
                    p3StrokeColor: counter % 8 >= 4 ? "transparent" : "pink"
                    p4StrokeColor: counter % 16 >= 8 ? "transparent" : "gray"
                }

                RectStack {
                    preferredRendererType: renderer
                    p1FillColor: "transparent"
                    p2FillColor: "transparent"
                    p3FillColor: "transparent"
                    p4FillColor: "transparent"
                    p1StrokeColor: counter % 16 >= 8 ? "lightblue": "transparent"
                    p2StrokeColor: counter % 8 >= 4 ? "lightgreen": "transparent"
                    p3StrokeColor: counter % 4 >= 2 ? "pink": "transparent"
                    p4StrokeColor: counter % 2 >= 1 ? "gray" : "transparent"
                }
            }
        }
    }
}
