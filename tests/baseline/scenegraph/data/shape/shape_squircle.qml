import QtQuick
import QtQuick.Shapes

Rectangle {
    width: 320
    height: 480
    color: "lightgray"

    ListModel {
        id: renderers
        ListElement { renderer: Shape.GeometryRenderer }
        ListElement { renderer: Shape.CurveRenderer }
    }

    Row {
        padding: 10
        Repeater {
            model: renderers
            Column {
                spacing: 10
                Shape {
                    width: 160
                    preferredRendererType: renderer

                    ShapePath {
                        fillColor: "transparent"
                        strokeColor: "blue"
                        strokeWidth: 1

                        PathRectangle {
                            x: 20; y: 0
                            width: 100; height: 60
                            radius: 30
                            cornerShape: PathRectangle.Squircle
                        }

                        PathRectangle {
                            x: 20.5; y: 70.5
                            width: 100; height: 60
                            radius: 15
                            cornerShape: PathRectangle.Squircle
                        }
                    }
                }

                Shape {
                    width: 160
                    preferredRendererType: renderer

                    ShapePath {
                        fillColor: "yellow"
                        strokeColor: "transparent"

                        PathRectangle {
                            x: 20; y: 0
                            width: 100; height: 60
                            radius: 20
                            cornerShape: PathRectangle.Squircle
                        }

                        PathRectangle {
                            x: 20.5; y: 70.5
                            width: 60; height: 60
                            radius: 8
                            cornerShape: PathRectangle.Squircle
                        }
                    }
                }

                Shape {
                    width: 160
                    preferredRendererType: renderer

                    ShapePath {
                        fillColor: "yellow"
                        strokeColor: "green"
                        strokeWidth: 5
                        joinStyle: ShapePath.RoundJoin

                        PathRectangle {
                            x: 20; y: 0
                            width: 100; height: 60
                            cornerShape: PathRectangle.Squircle
                        }

                        PathRectangle {
                            x: 20; y: 70
                            width: 100; height: 60
                            radius: 25
                            cornerShape: PathRectangle.Squircle
                        }
                    }
                }

                Shape {
                    width: 160
                    preferredRendererType: renderer

                    ShapePath {
                        id: myPath
                        fillColor: "white"
                        strokeColor: "blue"
                        strokeWidth: 20
                        joinStyle: ShapePath.MiterJoin

                        PathRectangle {
                            width: 120
                            height: 60
                            radius: 20
                            strokeAdjustment: myPath.strokeWidth
                            cornerShape: PathRectangle.Squircle
                        }
                    }
                }

                Item {
                    width: 100
                    height: width

                    Shape {
                        anchors.fill: parent
                        preferredRendererType: renderer

                        ShapePath {
                            strokeColor: "black"
                            strokeWidth: 2
                            joinStyle: ShapePath.RoundJoin

                            PathRectangle {
                                width: 100; height: 100
                                radius: 0
                                cornerShape: PathRectangle.Squircle
                            }
                        }
                    }

                    Shape {
                        anchors.fill: parent
                        preferredRendererType: renderer

                        ShapePath {
                            strokeColor: "red"
                            strokeWidth: 2
                            joinStyle: ShapePath.RoundJoin

                            PathRectangle {
                                width: 100; height: 100
                                radius: 10
                                cornerShape: PathRectangle.Squircle
                            }
                        }
                    }

                    Shape {
                        anchors.fill: parent
                        preferredRendererType: renderer

                        ShapePath {
                            strokeColor: "green"
                            strokeWidth: 2
                            joinStyle: ShapePath.RoundJoin

                            PathRectangle {
                                width: 100; height: 100
                                radius: 25
                                cornerShape: PathRectangle.Squircle
                            }
                        }
                    }

                    Shape {
                        anchors.fill: parent
                        preferredRendererType: renderer

                        ShapePath {
                            strokeColor: "blue"
                            strokeWidth: 2
                            joinStyle: ShapePath.RoundJoin

                            PathRectangle {
                                width: 100; height: 100
                                radius: 50
                                cornerShape: PathRectangle.Squircle
                            }
                        }
                    }

                    Rectangle {
                        anchors.fill: parent
                        radius: width / 2
                        color: "transparent"
                        border.color: "gray"
                    }
                }
            }
        }
    }
}
