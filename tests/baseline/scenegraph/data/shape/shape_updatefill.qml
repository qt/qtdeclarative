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

    LinearGradient {
        id: grad1
        x2: 60; y2: 60
        GradientStop { position: 0; color: "black" }
        GradientStop { position: 1; color: "red" }
    }

    ConicalGradient {
        id: grad2
        centerX: 15; centerY: 15
        GradientStop { position: 0; color: "yellow" }
        GradientStop { position: .5; color: "black" }
        GradientStop { position: 1; color: "yellow" }
    }

    Image {
        id: img1
        source: "../shared/world.png"
        visible: false
    }

    Image {
        id: img2
        source: "../shared/sample_1.png"
        visible: false
    }

    Row {
        padding: 10
        spacing: 20
        Repeater {
            model: renderers
            Shape {
                width: 140
                preferredRendererType: renderer

                ShapePath {
                    id: c1
                    strokeColor: "black"
                    fillColor: "cyan"
                    fillTransform: PlanarTransform.fromTranslate(rc1.x, rc1.y)

                    PathRectangle {
                        id: rc1
                        width: 60; height: 60
                    }
                }

                ShapePath {
                    id: c2
                    strokeColor: "black"
                    fillColor: "cyan"
                    fillTransform: PlanarTransform.fromTranslate(rc2.x, rc2.y)

                    PathRectangle {
                        id: rc2
                        x: 80
                        width: 60; height: 60
                    }
                }

                ShapePath {
                    id: g1
                    strokeColor: "black"
                    fillColor: "cyan"
                    fillTransform: PlanarTransform.fromTranslate(rg1.x, rg1.y)
                    fillGradient: grad1

                    PathRectangle {
                        id: rg1
                        y: 80
                        width: 60; height: 60
                    }
                }

                ShapePath {
                    id: t1
                    strokeColor: "black"
                    fillColor: "cyan"
                    fillItem: img1
                    fillTransform: PlanarTransform.fromTranslate(rt1.x, rt1.y)

                    PathRectangle {
                        id: rt1
                        x: 80; y: 80
                        width: 60; height: 60
                    }
                }

                ShapePath {
                    id: g2
                    strokeColor: "black"
                    fillColor: "cyan"
                    fillTransform: PlanarTransform.fromTranslate(rg2.x, rg2.y)
                    fillGradient: grad1

                    PathRectangle {
                        id: rg2
                        y: 2 * 80
                        width: 60; height: 60
                    }
                }

                ShapePath {
                    id: t2
                    strokeColor: "black"
                    fillColor: "cyan"
                    fillTransform: PlanarTransform.fromTranslate(rt2.x, rt2.y)
                    fillItem: img1

                    PathRectangle {
                        id: rt2
                        x: 80; y: 2 * 80
                        width: 60; height: 60
                    }
                }

                ShapePath {
                    id: g3
                    strokeColor: "black"
                    fillColor: "cyan"
                    fillTransform: PlanarTransform.fromTranslate(rg3.x, rg3.y)
                    fillGradient: grad1

                    PathRectangle {
                        id: rg3
                        y: 3 * 80
                        width: 60; height: 60
                    }
                }

                ShapePath {
                    id: t3
                    strokeColor: "black"
                    fillColor: "cyan"
                    fillTransform: PlanarTransform.fromTranslate(rt3.x, rt3.y)
                    fillItem: img1

                    PathRectangle {
                        id: rt3
                        x: 80; y: 3 * 80
                        width: 60; height: 60
                    }
                }

                ShapePath {
                    id: g4
                    strokeColor: "black"
                    fillColor: "cyan"
                    fillTransform: PlanarTransform.fromTranslate(rg4.x, rg4.y)
                    fillGradient: grad2

                    PathRectangle {
                        id: rg4
                        y: 4 * 80
                        width: 60; height: 60
                    }
                }

                ShapePath {
                    id: t4
                    strokeColor: "black"
                    fillColor: "cyan"
                    fillTransform: PlanarTransform.fromTranslate(rt4.x, rt4.y)
                    fillItem: img2

                    PathRectangle {
                        id: rt4
                        x: 80; y: 4 * 80
                        width: 60; height: 60
                    }
                }

                Timer {
                    running: true
                    interval: 150 // <200ms needed for scenegrabber; disable for manual testing
                    onTriggered: {
                        // Test all changes A->B, where A,B in {fillColor, fillGradient, fillItem}
                        // plus change of fillTransform

                        c1.fillGradient = grad1
                        g1.fillGradient = null
                        g2.fillGradient = null
                        g2.fillItem = img1
                        g3.fillGradient = grad2

                        c2.fillItem = img1
                        t1.fillItem = null
                        t2.fillGradient = grad1
                        t3.fillItem = img2

                        g4.fillTransform = g4.fillTransform.times(PlanarTransform.fromRotate(45, 30, 30))
                        t4.fillTransform = t4.fillTransform.times(PlanarTransform.fromRotate(45, 30, 30))
                    }
                }
            }
        }
    }
}

