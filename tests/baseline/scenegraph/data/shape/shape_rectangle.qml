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
                            width: 100; height: 20
                        }

                        PathRectangle {
                            x: 20.5; y: 30.5
                            width: 100; height: 20
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
                            width: 100; height: 20
                        }

                        PathRectangle {
                            x: 20.5; y: 30.5
                            width: 100; height: 20
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
                            x: 20; y: 00
                            width: 100; height: 20
                        }

                        PathRectangle {
                            x: 20; y: 30
                            width: 100; height: 20
                            radius: 5
                        }
                    }

                    ShapePath {
                        fillColor: "yellow"
                        strokeColor: "green"
                        strokeWidth: 5
                        joinStyle: ShapePath.MiterJoin

                        PathRectangle {
                            x: 20; y: 60
                            width: 100; height: 20
                        }

                        PathRectangle {
                            x: 20; y: 90
                            width: 100; height: 20
                            radius: 5
                        }

                        PathRectangle {
                            x: 20; y: 120
                            width: 100; height: 20
                            radius: 50
                        }

                        PathRectangle {
                            x: 20; y: 150
                            width: 100; height: 30
                            radius: 10
                            topLeftRadius: 50
                            bottomRightRadius: 5
                            bottomLeftRadius: 0
                        }
                    }
                }

                Rectangle {
                    id: rect
                    width: 120
                    height: 60
                    color: "white"
                    border.width: 20
                    border.color: "blue"
                    topRightRadius: 30
                }

                Shape {
                    width: 160
                    preferredRendererType: renderer

                    ShapePath {
                        id: myPath
                        fillColor: rect.color
                        strokeColor: rect.border.color
                        strokeWidth: rect.border.width
                        joinStyle: ShapePath.MiterJoin

                        PathRectangle {
                            width: rect.width
                            height: rect.height
                            topRightRadius: rect.topRightRadius
                            strokeAdjustment: myPath.strokeWidth
                        }
                    }
                }
            }
        }
    }
}

