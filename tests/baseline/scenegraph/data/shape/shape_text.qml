import QtQuick 2.15
import QtQuick.Shapes 6.6

Item {
    width: 320
    height: 480

    ListModel {
        id: renderers
        ListElement { renderer: Shape.GeometryRenderer }
        ListElement { renderer: Shape.CurveRenderer }
    }

    Row {
        Repeater {
            model: renderers
            Column {
                Item {
                    width: 160
                    height: 160

                    Shape {
                        anchors.fill: parent
                        preferredRendererType: renderer

                        ShapePath {
                            fillColor: "transparent"
                            strokeColor: "blue"
                            strokeStyle: ShapePath.DashLine
                            strokeWidth: 4

                            PathText {
                                x: 24
                                y: 10
                                font.pixelSize: 96
                                text: "Qt"
                            }
                        }
                    }
                }

                Item {
                    width: 100
                    height: 160

                    Rectangle {
                        anchors.fill: parent
                        color: "blue"
                    }

                    Shape {
                        anchors.fill: parent
                        preferredRendererType: renderer

                        ShapePath {
                            fillColor: "red"
                            strokeColor: "blue"
                            strokeStyle: ShapePath.DashLine
                            capStyle: ShapePath.RoundCap
                            strokeWidth: 4

                            PathText {
                                x: 24; y: 10
                                font.pixelSize: 96
                                text: "Qt"
                            }
                        }
                    }
                }

                Item {
                    width: 160
                    height: 160

                    Shape {
                        anchors.fill: parent
                        preferredRendererType: renderer

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
                                x: 24; y: 10
                                font.pixelSize: 96
                                text: "Qt"
                            }
                        }
                    }
                }
            }
        }
    }
}
