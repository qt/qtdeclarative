import QtQuick
import QtQuick.Shapes

Item {
    width: 620
    height: 400
    transformOrigin: Item.TopLeft

    property var svgPaths: [
       "M 0 25 L 200 25",
       "M 0 0 L 215 125",
       "M 0 0 L 175 175",
       "M 100 0 Q 200 0 200 75 Q 200 150 100 150 Q 0 150 0 75 Q 0 0 100 0 z",
       "M 50 0 L 50 150"
       ]

    Flow {
        anchors.fill: parent
        padding: 25
        spacing: 25
        Repeater {
            model: svgPaths
            Shape {
                preferredRendererType: Shape.CurveRenderer
                ShapePath {
                    strokeColor: "red"
                    strokeWidth: 22
                    fillColor: "gray"
                    capStyle: ShapePath.SquareCap
                    PathSvg { path: modelData }
                }
                Rectangle {
                    color: "transparent"
                    border.color: "blue"
                    border.width: 2
                    x: parent.boundingRect.x
                    y: parent.boundingRect.y
                    width: parent.boundingRect.width
                    height: parent.boundingRect.height
                }
            }
        }

    }
}
