import QtQuick
import QtQuick.Shapes

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
                Repeater {
                    model: 3
                    Shape {
                        preferredRendererType: renderer
                        width: 160
                        height: 150
                        ShapePath {
                            strokeColor: "transparent"

                            fillGradient: LinearGradient {
                                id: grad
                                y1: 50; y2: 80
                                spread: model.index === 0 ? ShapeGradient.PadSpread : (model.index === 1 ? ShapeGradient.RepeatSpread : ShapeGradient.ReflectSpread)
                                GradientStop { position: 0; color: "black" }
                                GradientStop { position: 1; color: "red" }
                            }
                            fillTransform: PlanarTransform.fromShear(0, 0.2)

                            startX: 10; startY: 10
                            PathLine { relativeX: 140; relativeY: 0 }
                            PathLine { relativeX: 0; relativeY: 100 }
                            PathLine { relativeX: -140; relativeY: 0 }
                            PathLine { relativeX: 0; relativeY: -100 }
                        }
                    }
                }
            }
        }
    }
}
