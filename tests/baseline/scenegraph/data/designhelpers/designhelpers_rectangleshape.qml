import QtQuick
import QtQuick.Shapes
import QtQuick.Shapes.DesignHelpers

Rectangle {
    width: 800
    height: 800
    color: "lightgray"

    component RectangleShapeBase : RectangleShape {
        width: 120
        height: 90
    }

    Flow {
        spacing: 10
        anchors.fill: parent
        anchors.margins: 10

        // default-constructed
        RectangleShapeBase {}

        // radii
        RectangleShapeBase {
            topLeftRadius: 0
        }
        RectangleShapeBase {
            topRightRadius: 0
        }
        RectangleShapeBase {
            bottomRightRadius: 0
        }
        RectangleShapeBase {
            bottomLeftRadius: 0
        }
        RectangleShapeBase {
            topLeftRadius: 0
            topRightRadius: 0
        }
        RectangleShapeBase {
            topLeftRadius: 0
            topRightRadius: 0
        }
        RectangleShapeBase {
            topLeftRadius: 0
            topRightRadius: 0
            bottomLeftRadius: 0
        }
        RectangleShapeBase {
            topLeftRadius: 0
            topRightRadius: 0
            bottomLeftRadius: 0
            bottomRightRadius: 0
        }

        // bevel
        RectangleShapeBase {
            topLeftBevel: true
        }
        RectangleShapeBase {
            topRightBevel: true
        }
        RectangleShapeBase {
            bottomRightBevel: true
        }
        RectangleShapeBase {
            bottomLeftBevel: true
        }
        RectangleShapeBase {
            topLeftBevel: true
            topRightBevel: true
        }
        RectangleShapeBase {
            topLeftBevel: true
            topRightBevel: true
        }
        RectangleShapeBase {
            topLeftBevel: true
            topRightBevel: true
            bottomLeftBevel: true
        }
        RectangleShapeBase {
            topLeftBevel: true
            topRightBevel: true
            bottomLeftBevel: true
            bottomRightBevel: true
        }
        RectangleShapeBase {
            strokeColor: "red"
            bevel: true
        }

        // strokeWidth
        RectangleShapeBase {
            strokeWidth: 0
        }

        // fillColor
        RectangleShapeBase {
            fillColor: "red"
        }

        // joinStyle
        Repeater {
            model: joinStyles

            readonly property var joinStyles: [ ShapePath.MiterJoin, ShapePath.BevelJoin, ShapePath.RoundJoin ]

            RectangleShapeBase {
                topLeftBevel: true
                strokeWidth: 20
                topLeftRadius: 40
                joinStyle: modelData

                required property int modelData
            }
        }

        // capStyle
        Repeater {
            model: capStyles

            readonly property var capStyles: [ ShapePath.FlatCap, ShapePath.SquareCap, ShapePath.RoundCap ]

            RectangleShapeBase {
                topLeftBevel: true
                strokeWidth: 20
                strokeColor: "darkgray"
                topLeftRadius: 40
                capStyle: modelData

                required property int modelData
            }
        }

        // strokeStyle
        RectangleShapeBase {
            strokeStyle: ShapePath.DashLine
        }
        RectangleShapeBase {
            strokeStyle: ShapePath.DashLine
            dashPattern: [1, 2]
        }
        RectangleShapeBase {
            strokeStyle: ShapePath.DashLine
            dashPattern: [6, 12]
            dashOffset: 4
        }

        // fillGradient
        RectangleShapeBase {
            id: fillGradientRectangleShape
            fillGradient: RadialGradient {
                focalY: fillGradientRectangleShape.height * 0.5
                focalX: fillGradientRectangleShape.width * 0.5
                centerY: fillGradientRectangleShape.height * 0.5
                centerX: fillGradientRectangleShape.width * 0.5
                centerRadius: Math.min(fillGradientRectangleShape.width, fillGradientRectangleShape.height) * 0.5

                GradientStop {
                    position: 0
                    color: "#cea1f9fc"
                }

                GradientStop {
                    position: 1
                    color: "#ec7d7d"
                }
            }
        }

        // borderMode
        RectangleShapeBase {
            borderMode: RectangleShape.Inside
        }
        RectangleShapeBase {
            borderMode: RectangleShape.Middle
        }
        RectangleShapeBase {
            borderMode: RectangleShape.Outside
        }
    }
}

