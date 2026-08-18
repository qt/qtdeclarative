import QtQuick
import QtQuick.Shapes

// Test all changes A->B->C, where each state is any of the 4
// combinations of fill on/off, stroke on/off, with static colors

// The state of a shape at step k is encoded in bit pair k of its
// index: bit 0 of the pair makes the fill colored, bit 1 makes the
// stroke colored. Hence 4 * 4 * 4 = 64 shapes per renderer.

Rectangle {
    id: root
    width: 800
    height: 400
    color: "lightgray"

    readonly property int cell: 48
    property int step: 0

    Timer {
        running: true
        interval: 100
        onTriggered: root.step = 1
    }

    Timer {
        running: true
        interval: 180
        onTriggered: root.step = 2
    }

    ListModel {
        id: renderers
        ListElement { renderer: Shape.GeometryRenderer }
        ListElement { renderer: Shape.CurveRenderer }
    }

    Row {
        padding: 5
        spacing: 20
        Repeater {
            model: renderers

            Item {
                width: 8 * root.cell
                height: 8 * root.cell

                Repeater {
                    model: 64

                    Shape {
                        preferredRendererType: renderer
                        x: (index % 8) * root.cell
                        y: Math.floor(index / 8) * root.cell
                        width: root.cell
                        height: root.cell

                        ShapePath {
                            readonly property int state: (index >> (2 * root.step)) & 3

                            fillColor: (state & 1) ? "cyan" : "transparent"
                            strokeColor: (state & 2) ? "blue" : "transparent"
                            strokeWidth: 6
                            joinStyle: ShapePath.RoundJoin

                            startX: 10; startY: 40
                            PathLine { x: 24; y: 10 }
                            PathLine { x: 38; y: 40 }
                            PathLine { x: 10; y: 40 }
                        }
                    }
                }
            }
        }
    }
}
