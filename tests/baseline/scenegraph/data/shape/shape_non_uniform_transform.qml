import QtQuick
import QtQuick.Shapes

Rectangle {
    width: 500
    height: 1200
    color: "black"

    ListModel {
        id: renderers
        ListElement { renderer: Shape.GeometryRenderer }
        ListElement { renderer: Shape.CurveRenderer  }
    }

    Component {
        id: shapeComponent

        Shape {
            preferredRendererType: selectedRenderer
            width: 250
            height: 400
            clip: true

            transform: [
                Translate {
                    x: -100
                    y: -100
                },
                Matrix4x4 {
                    matrix: Qt.matrix4x4(1, 0, 0, 0,
                                     0, 1, 0, 0,
                                     0, 0, 1, 0,
                                     0, -1/50, 0, 1)
                },
                Translate {
                    x: 100
                    y: 100
                }
            ]

            TextMetrics {
                id: tm
                font.pixelSize: 40
                text: "Qt!"
            }

            ShapePath {
                strokeWidth: selectedStrokeWidth
                fillColor: "red"
                strokeColor: "green"
                cosmeticStroke: selectedCosmeticStroke
                PathText {
                    x: 100 - tm.tightBoundingRect.width / 2
                    y: 110
                    font: tm.font
                    text: tm.text
                }
            }

        }
    }

    Row {
        anchors.fill: parent
        Repeater {
            model: renderers
            Column {
                Loader {
                    sourceComponent: shapeComponent
                    property var selectedRenderer: renderer
                    property real selectedStrokeWidth: 0
                    property bool selectedCosmeticStroke: false
                }
                Loader {
                    sourceComponent: shapeComponent
                    property var selectedRenderer: renderer
                    property real selectedStrokeWidth: 1
                    property bool selectedCosmeticStroke: false
                }
                Loader {
                    sourceComponent: shapeComponent
                    property var selectedRenderer: renderer
                    property real selectedStrokeWidth: 1
                    property bool selectedCosmeticStroke: true
                }
            }
        }
    }
}
