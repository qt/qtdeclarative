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

    ListModel {
        id: cornerShapes
        ListElement { cornerShape: PathRectangle.Rounded }
        ListElement { cornerShape: PathRectangle.Squircle }
        ListElement { cornerShape: PathRectangle.Bevel }
    }

    Row {
        padding: 10
        Repeater {
            model: renderers
            Column {
                id: rendererColumn
                required property int renderer
                spacing: 10
                Repeater {
                    model: cornerShapes
                    Shape {
                        id: shape
                        required property int cornerShape
                        width: 150
                        height: 140
                        preferredRendererType: rendererColumn.renderer

                        ShapePath {
                            fillColor: "yellow"
                            strokeColor: "green"
                            strokeWidth: 3
                            joinStyle: ShapePath.MiterJoin

                            // Uniform inverted corners
                            PathRectangle {
                                x: 5; y: 0
                                width: 120; height: 40
                                cornerShape: shape.cornerShape
                                radius: -12
                            }

                            // Mixed inverted, convex and sharp corners
                            PathRectangle {
                                x: 5; y: 50
                                width: 120; height: 40
                                cornerShape: shape.cornerShape
                                radius: 8
                                topLeftRadius: -15
                                bottomRightRadius: -5
                                bottomLeftRadius: 0
                            }

                            // Clamped to half of the height
                            PathRectangle {
                                x: 5; y: 100
                                width: 120; height: 40
                                cornerShape: shape.cornerShape
                                radius: -100
                            }
                        }
                    }
                }
            }
        }
    }
}
