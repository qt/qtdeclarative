import QtQuick

Item {
    width: 360
    height: 360
    Flickable {
        id: flickable
        anchors.top: parent.top
        anchors.left: parent.left
        width: 300
        height: 300
        contentWidth: rect.width
        contentHeight: rect.height
        Rectangle {
            id: rect
            color: "beige"
            width: 300
            height: 650
            Text {
                text: "flick with mouse or touch"
                anchors.centerIn: parent
            }
        }
    }
    Rectangle {
        anchors.fill: flickable
        color: tapHandler.pressed ? "lightsteelblue" : "aliceblue"
        opacity: 0.5
        TapHandler {
            id: tapHandler
        }
    }
}
