import QtQuick 2.15
import QtQuick.Shapes 1.0

Item {
    width: 320
    height: 480

    Column {
        Item {
            width: 200
            height: 160

            Shape {
                anchors.fill: parent
                vendorExtensionsEnabled: false

                ShapePath {
                    fillColor: "transparent"
                    strokeColor: "blue"
                    strokeStyle: ShapePath.DashLine
                    strokeWidth: 4

                    PathText {
                        x: 96
                        y: 10
                        font.pixelSize: 120
                        text: "Qt"
                    }
                }
            }
        }

        Item {
            width: 200
            height: 160

            Rectangle {
                anchors.fill: parent
                color: "blue"
            }

            Shape {
                anchors.fill: parent
                vendorExtensionsEnabled: false

                ShapePath {
                    fillColor: "red"
                    strokeColor: "blue"
                    strokeStyle: ShapePath.DashLine
                    capStyle: ShapePath.RoundCap
                    strokeWidth: 8

                    PathText {
                        x: 96; y: 10
                        font.pixelSize: 150
                        text: "Qt"
                    }
                }
            }
        }

        Item {
            width: 200
            height: 160

            Shape {
                anchors.fill: parent
                vendorExtensionsEnabled: false

                ShapePath {
                    fillGradient: LinearGradient {
                        x1: 0; x2: 200; y1: 0; y2: 160
                        spread: ShapeGradient.PadSpread
                        GradientStop { position: 0.0; color: "red"; }
                        GradientStop { position: 1.0; color: "green"; }
                    }
                    strokeColor: "blue"
                    strokeStyle: ShapePath.DashLine
                    joinStyle: ShapePath.RoundJoin
                    strokeWidth: 4

                    PathText {
                        x: 96; y: 10
                        font.pixelSize: 150
                        text: "Qt"
                    }
                }
            }
        }
    }
}
