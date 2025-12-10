import QtQuick
import QtQuick.Shapes
import QtQuick.Shapes.DesignHelpers

Rectangle {
    width: 800
    height: 800
    color:"#b5e7a0"

    component TestEllipseShape : EllipseShape {
        fillColor: "#37c1ff"
        strokeColor: "#663333"
        width: 90
        height: 90
    }

    Flow {
        spacing: 2
        anchors.fill: parent
        EllipseShape {
            width: 90
            height: 90
        }
        TestEllipseShape {
            startAngle: 0
            sweepAngle: 180
            cornerRadius: 0
            strokeStyle: ShapePath.DashLine
            capStyle: ShapePath.FlatCap
        }
        TestEllipseShape {
            startAngle: 0
            sweepAngle: 270
            cornerRadius: 0
            strokeStyle: ShapePath.DashLine
            capStyle: ShapePath.SquareCap
        }
        TestEllipseShape {
            startAngle: 0
            sweepAngle: 360
            strokeStyle: ShapePath.DashLine
            capStyle: ShapePath.RoundCap
        }
        TestEllipseShape {
            startAngle: -90
            sweepAngle: 360
            innerArcRatio: 0.5
        }
        TestEllipseShape {
            startAngle: -90
            sweepAngle: 350
            cornerRadius: 0
            innerArcRatio: 0.5
            strokeWidth: 1
        }
        TestEllipseShape {
            startAngle: -90
            sweepAngle: 180
            cornerRadius: 0
            strokeStyle: ShapePath.DashLine
        }
        TestEllipseShape {
            startAngle: -360
            sweepAngle: 360
        }
        TestEllipseShape {
            id: ellipseId
            startAngle: 360
            sweepAngle: -360
            strokeWidth: 1
            fillGradient: RadialGradient {
                focalX: ellipseId.width * 0.5
                focalY: ellipseId.height * 0.75
                centerX: ellipseId.width * 0.5
                centerY: ellipseId.height * 0.5
                centerRadius: ellipseId.width * 0.5
                GradientStop { position:0.1; color:"cyan" }
                GradientStop { position:0.2; color:"green" }
                GradientStop { position:0.4; color:"red" }
                GradientStop { position:0.6; color:"yellow" }
                GradientStop { position:1.0; color:"blue" }
            }
        }
        TestEllipseShape {
            startAngle: -360
            sweepAngle: 360
            innerArcRatio: 1
            fillGradient: LinearGradient {
                x1: 20
                y1: 20
                x2: 100
                y2: 100
                GradientStop { position: 0.0; color: "red" }
                GradientStop { position: 0.33; color: "yellow" }
                GradientStop { position: 1.0; color: "green" }
            }
        }
        TestEllipseShape {
            startAngle: -360
            sweepAngle: 360
            fillColor: "transparent"
        }
        TestEllipseShape {
            startAngle: -360
            sweepAngle: 360
            innerArcRatio: 0.9
            strokeWidth: 2
        }
        TestEllipseShape {
            startAngle: 360
            sweepAngle: 240
            cornerRadius: 0
            innerArcRatio: 0.7
            strokeWidth: 2
        }
        TestEllipseShape {
            startAngle: -90
            sweepAngle: 270
            innerArcRatio: 0.5
            strokeWidth: 2
        }
        TestEllipseShape {
            startAngle: 360
            sweepAngle: -270
            innerArcRatio: 0.5
            strokeWidth: 2
            strokeStyle: ShapePath.DashLine
        }
        TestEllipseShape {
            innerArcRatio: 0.7
        }
        TestEllipseShape {
            startAngle: 270
            sweepAngle: 360
            innerArcRatio: 0
            strokeWidth: 1
        }
        TestEllipseShape {
            startAngle: 0
            sweepAngle: 320
            strokeWidth: 2
            strokeStyle: ShapePath.DashLine
        }
        TestEllipseShape {
            startAngle: 0
            sweepAngle: 320
            strokeWidth: 2
            strokeStyle: ShapePath.DashLine
            innerArcRatio: 0.5
            cornerRadius: 0
        }
        TestEllipseShape {
            startAngle: 0
            sweepAngle: 320
            strokeWidth: 2
            strokeStyle: ShapePath.DashLine
            innerArcRatio: 0.5
        }
        TestEllipseShape {
            startAngle: 0
            sweepAngle: 360
            borderMode: EllipseShape.Inside
        }
        TestEllipseShape {
            startAngle: 0
            sweepAngle: 360
            borderMode: EllipseShape.Middle
        }
        TestEllipseShape {
            startAngle: 0
            sweepAngle: 360
            borderMode: EllipseShape.Outside
        }
        TestEllipseShape {
            startAngle: 0
            sweepAngle: 270
            cornerRadius: 0
            strokeWidth: 2
            innerArcRatio: 1
        }
        TestEllipseShape {
            startAngle: 0
            sweepAngle: 270
            cornerRadius: 0
            strokeWidth: 2
            innerArcRatio: 0
            fillColor: "transparent"
        }
        TestEllipseShape {
            startAngle: 0
            sweepAngle: 360
            strokeStyle: ShapePath.DashLine
            dashPattern: [1,2]
            dashOffset: 2
        }
        TestEllipseShape {
            startAngle: 0
            sweepAngle: 360
            strokeStyle: ShapePath.DashLine
            dashPattern: [2,4]
            dashOffset: 4
        }
        TestEllipseShape {
            startAngle: 360
            sweepAngle: 323
            strokeStyle: ShapePath.DashLine
            strokeWidth: 3
            joinStyle: ShapePath.BevelJoin
        }
        TestEllipseShape {
            startAngle: 360
            sweepAngle: 323
            strokeStyle: ShapePath.DashLine
            strokeWidth: 3
            joinStyle: ShapePath.MiterJoin
        }
        TestEllipseShape {
            startAngle: 360
            sweepAngle: 323
            strokeStyle: ShapePath.DashLine
            strokeWidth: 3
            joinStyle: ShapePath.RoundJoin
        }
    }
}
