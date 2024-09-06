import QtQuick 2.15
import QtQuick.Shapes 6.6

Item {
    id: topItem
    width: 480
    height: 320
    property color outlineColor: "blue"

    ListModel {
        id: renderers
        ListElement { renderer: Shape.CurveRenderer; async: true }
        ListElement { renderer: Shape.CurveRenderer; async: false }
        ListElement { renderer: Shape.GeometryRenderer; async: true }
        ListElement { renderer: Shape.GeometryRenderer; async: false }
    }

    Column {
        Repeater {
            model: renderers
            Shape {
                id: shape
                width: topItem.width
                height: topItem.height / 4
                preferredRendererType: renderer
                asynchronous: async

                property int offset: 30
                property string label: ""
                Timer {
                    interval: 17
                    running: true
                    repeat: true
                    onTriggered: {
                        const str = 'Qt Rocks! 012345678901234567890123456789'
                        if (offset > 0) {
                            offset = offset - 1;
                            label = str.substring(offset);
                        } else {
                            stop();
                        }
                    }
                }

                ShapePath {
                    fillColor: "orange"
                    strokeColor: topItem.outlineColor
                    strokeWidth: 1.5

                    PathText {
                        y: 15
                        font.pixelSize: 64
                        text: shape.label
                    }
                }
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: { topItem.outlineColor = "red"; }
    }
}
