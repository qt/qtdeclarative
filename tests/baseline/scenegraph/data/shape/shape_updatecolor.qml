import QtQuick
import QtQuick.Shapes

Item {
    width: 320
    height: 800

    ListModel {
        id: renderers
        ListElement { renderer: Shape.GeometryRenderer }
        ListElement { renderer: Shape.CurveRenderer }
    }

    Row {
        Repeater {
            model: renderers
            Column {
                Shape {
                    preferredRendererType: renderer
                    width: 160
                    height: 150

                    ShapePath {
                        strokeColor: "transparent"
                        fillColor: "red"

                        startX: 10; startY: 10
                        PathLine { relativeX: 140; relativeY: 0 }
                        PathLine { relativeX: 0; relativeY: 100 }
                        PathLine { relativeX: -140; relativeY: 0 }
                        PathLine { relativeX: 0; relativeY: -100 }
                    }
                }

                Shape {
                    preferredRendererType: renderer
                    width: 160
                    height: 150

                    ShapePath {
                        strokeColor: "transparent"
                        fillColor: "red"

                        startX: 10; startY: 10 + 1 * 140
                        PathLine { relativeX: 140; relativeY: 0 }
                        PathLine { relativeX: 0; relativeY: 100 }
                        PathLine { relativeX: -140; relativeY: 0 }
                        PathLine { relativeX: 0; relativeY: -100 }
                    }
                }

                Shape {
                    preferredRendererType: renderer
                    width: 160
                    height: 150
                    Timer {
                        interval: 100
                        running: true
                        onTriggered: s.fillColor = Qt.rgba(0, 1, 0, 1)
                    }

                    ShapePath {
                        id: s
                        strokeColor: "transparent"
                        fillColor: "red"

                        startX: 10; startY: 10 + 2 * 140
                        PathLine { relativeX: 140; relativeY: 0 }
                        PathLine { relativeX: 0; relativeY: 100 }
                        PathLine { relativeX: -140; relativeY: 0 }
                        PathLine { relativeX: 0; relativeY: -100 }
                    }
                }
            }
        }
    }
}
