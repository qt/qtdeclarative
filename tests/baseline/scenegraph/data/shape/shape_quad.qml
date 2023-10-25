import QtQuick 2.9
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
                Repeater {
                    model: 4
                    Item {
                        width: 160
                        height: 100

                        Shape {
                            anchors.fill: parent
                            preferredRendererType: renderer

                            ShapePath {
                                strokeWidth: (model.index + 2) * 2
                                strokeColor: "black"
                                fillColor: "lightBlue"

                                startX: 30; startY: 100
                                PathQuad {
                                    x: 130; y: 100
                                    controlX: model.index * 10; controlY: model.index * 5
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
