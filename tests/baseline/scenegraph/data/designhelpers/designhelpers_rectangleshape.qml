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
        spacing: 8
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
            strokeColor: "#777777"
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

        // capStyle/draw*
        Repeater {
            model: [
                { capStyle: ShapePath.FlatCap, topLeftBevel: true, drawRight: false },
                { capStyle: ShapePath.FlatCap, topRightBevel: true, drawBottom: false },
                { capStyle: ShapePath.FlatCap, bottomRightBevel: true, drawLeft: false },
                { capStyle: ShapePath.FlatCap, bottomLeftBevel: true, drawTop: false },
                { capStyle: ShapePath.SquareCap, topLeftBevel: true, drawRight: false },
                { capStyle: ShapePath.SquareCap, topRightBevel: true, drawBottom: false },
                { capStyle: ShapePath.SquareCap, bottomRightBevel: true, drawLeft: false },
                { capStyle: ShapePath.SquareCap, bottomLeftBevel: true, drawTop: false },
                { capStyle: ShapePath.RoundCap, topLeftBevel: true, drawRight: false },
                { capStyle: ShapePath.RoundCap, topRightBevel: true, drawBottom: false },
                { capStyle: ShapePath.RoundCap, bottomRightBevel: true, drawLeft: false },
                { capStyle: ShapePath.RoundCap, bottomLeftBevel: true, drawTop: false },
                { capStyle: ShapePath.RoundCap, drawLeft: false, drawRight: false },
                { capStyle: ShapePath.RoundCap, drawTop: false, drawBottom: false }
            ]

            RectangleShapeBase {
                topLeftBevel: !!modelData.topLeftBevel
                topRightBevel: !!modelData.topRightBevel
                bottomRightBevel: !!modelData.bottomRightBevel
                bottomLeftBevel: !!modelData.bottomLeftBevel
                strokeWidth: 20
                strokeColor: "darkgray"
                topLeftRadius: topLeftBevel ? 40 : undefined
                topRightRadius: topRightBevel ? 40 : undefined
                bottomRightRadius: bottomRightBevel ? 40 : undefined
                bottomLeftRadius: bottomLeftBevel ? 40 : undefined
                capStyle: modelData.capStyle
                drawRight: !modelData.hasOwnProperty("drawRight")
                drawBottom: !modelData.hasOwnProperty("drawBottom")
                drawLeft: !modelData.hasOwnProperty("drawLeft")
                drawTop: !modelData.hasOwnProperty("drawTop")

                required property var modelData

                // For debugging
                // Text {
                //     anchors.fill: parent
                //     wrapMode: Text.Wrap
                //     text: "tlb " + parent.topLeftBevel
                //         + "\ntrb " + parent.topRightBevel
                //         + "\nbrb " + parent.bottomRightBevel
                //         + "\nblb " + parent.bottomLeftBevel
                // }
                // Text {
                //     anchors.fill: parent
                //     wrapMode: Text.Wrap
                //     text: "dr " + parent.drawRight
                //         + "\ndb " + parent.drawBottom
                //         + "\ndl " + parent.drawLeft
                //         + "\ndt " + parent.drawTop
                // }
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

        // width, height
        RectangleShapeBase {
            id: testRect1
        }
        Item {
            Component.onCompleted: {
                testRect1.width = 100
                testRect1.height = 75
            }
        }
    }
}

