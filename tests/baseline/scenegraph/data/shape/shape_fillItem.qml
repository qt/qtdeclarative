import QtQuick
import QtQuick.Shapes

Item {
    width: 640
    height: 840

    ListModel {
        id: renderers
        ListElement { renderer: Shape.GeometryRenderer; rotationAmount: 0 }
        ListElement { renderer: Shape.GeometryRenderer; rotationAmount: 30 }
        ListElement { renderer: Shape.CurveRenderer; rotationAmount: 0 }
        ListElement { renderer: Shape.CurveRenderer; rotationAmount: 30 }
    }

    Image {
        id: image
        visible: false
        source: "../shared/col320x480.jpg"
    }

    Image {
        id: tiledImage
        visible: false
        source: "../shared/col320x480.jpg"
        layer.enabled: true
        layer.smooth: true
        layer.wrapMode: ShaderEffectSource.Repeat
    }

    Image {
        id: asynchronousImage
        visible: false
        source: "../shared/col320x480.jpg"
        layer.enabled: true
        layer.smooth: true
        layer.wrapMode: ShaderEffectSource.Repeat
        asynchronous: true
    }

    Rectangle {
        id: item
        visible: false
        layer.enabled: true
        layer.smooth: true
        layer.wrapMode: ShaderEffectSource.Repeat
        color: "cyan"
        width: 20
        height: 20
        Text {
            anchors.centerIn:  parent
            text: "😊"
        }
    }

    Rectangle {
        id: sourceItem
        color: "cyan"
        width: 20
        height: 20
        Text {
            anchors.centerIn:  parent
            text: "😁"
        }
    }

    ShaderEffectSource {
        id: shaderEffectSource
        sourceItem: sourceItem
        width: 20
        height: 20
        wrapMode: ShaderEffectSource.Repeat
        visible: false
        hideSource: true
        smooth: true
    }

    Row {
        anchors.fill: parent
        Repeater {
            model: renderers
            Column {
                Shape {
                    id: shape
                    preferredRendererType: renderer
                    width: 160
                    height: 700
                    property real rotate: rotationAmount

                    ShapePath {
                        strokeColor: "transparent"
                        fillItem: image
                        fillTransform: PlanarTransform.fromRotate(shape.rotate)

                        PathRectangle {
                            x: 10; y: 10
                            width: 140
                            height: 100
                        }

                        // startX: 10; startY: 10
                        // PathLine { relativeX: 140; relativeY: 0 }
                        // PathLine { relativeX: 0; relativeY: 100 }
                        // PathLine { relativeX: -140; relativeY: 0 }
                        // PathLine { relativeX: 0; relativeY: -100 }
                    }

                    ShapePath {
                        strokeColor: "transparent"
                        fillItem: tiledImage

                        PathRectangle {
                            x: 10; y: 10 + 1 * 140
                            width: 140
                            height: 100
                        }
                        fillTransform: PlanarTransform.fromRotate(shape.rotate)
                    }

                    ShapePath {
                        strokeColor: "transparent"
                        fillItem: item
                        fillTransform: PlanarTransform.fromRotate(shape.rotate)

                        PathRectangle {
                            x: 10; y: 10 + 2 * 140
                            width: 140
                            height: 100
                        }
                    }

                    ShapePath {
                        strokeColor: "transparent"
                        fillItem: asynchronousImage
                        fillTransform: PlanarTransform.fromRotate(shape.rotate)

                        PathRectangle {
                            x: 10; y: 10 + 3 * 140
                            width: 140
                            height: 100
                        }
                    }

                    ShapePath {
                        strokeColor: "transparent"
                        fillItem: shaderEffectSource
                        fillTransform: PlanarTransform.fromRotate(shape.rotate)

                        PathRectangle {
                            x: 10; y: 10 + 4 * 140
                            width: 140
                            height: 100
                        }
                    }
                }

                Shape {
                    preferredRendererType: renderer
                    width: 160
                    height: 200
                    x: 10

                    ShapePath {
                        strokeColor: "transparent"
                        fillItem: image
                        fillTransform: PlanarTransform.fromRotate(shape.rotate)

                        PathRectangle {
                            width: 140
                            height: 100
                        }
                    }
                }
            }
        }
    }
}
