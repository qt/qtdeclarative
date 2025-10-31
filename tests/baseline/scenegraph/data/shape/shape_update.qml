import QtQuick 2.15
import QtQuick.Shapes 6.6

Item {
    id: topItem
    width: 480
    height: 320
    property color outlineColor: "blue"
    property bool suspendGrabbing: true

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
                NumberAnimation on offset {
                    to: 0
                    duration: 1000
                }
                onOffsetChanged: {
                    const str = 'Qt Rocks! ABCDEFGHIJKLMNOPQRSTUVWXYZ0123'
                    shape.label = str.substring(offset);
                    if (offset === 0)
                        topItem.suspendGrabbing = false;
                }

                ShapePath {
                    fillColor: "orange"
                    strokeColor: topItem.outlineColor
                    strokeWidth: 1.5
                    fillRule: ShapePath.WindingFill

                    PathText {
                        y: 15
                        font.family: "Arial"
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
