import QtQuick 2.0

Rectangle {
    id: root
    width: 600
    height: 600

    PinchArea {
        objectName: "pincharea"
        pinch.target: rect
        anchors.fill: parent

        pinch.minimumScale: 1.0
        pinch.maximumScale: 10.0

        Flickable {
            objectName: "flickable"
            anchors.fill: parent
            contentHeight: 1000
            contentWidth: 1000

            MouseArea {
                anchors.fill: parent
                property int nextTimestamp: 0
                Component {
                    id: clickFeeback
                    Text {
                        text: parent.timestamp
                        Rectangle {
                            color: "red"
                            width: 5; height: 5; radius: 2
                            x: -2; y: -2
                        }
                    }
                }

                onClicked: clickFeeback.createObject(parent, { x: mouse.x - 2, y: mouse.y - 2, text: nextTimestamp++ })
            }

            Rectangle {
                objectName: "rect"
                id: rect
                color: "lightsteelblue"
                x: 200
                y: 200
                width: 400
                height: 400
                Text {
                    text: "pinch, click or flick anywhere\nthis rectangle is the pinch target"
                }
            }
        }
    }
}
