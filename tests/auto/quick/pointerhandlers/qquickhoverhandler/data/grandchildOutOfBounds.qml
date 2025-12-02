import QtQuick
import QtQuick.Shapes

Item {
    objectName: "declared root item"
    width: 240
    height: 320

    Rectangle {
        id: pool
        objectName: "pool"
        anchors.fill: parent
        anchors.margins: 40
        color: "aqua"
        border.color: "darkgrey"
        radius: 5

        Rectangle {
            objectName: "stand"
            width: 100
            height: 50
            color: "grey"
            anchors.bottom: parent.bottom
            anchors.margins: 5
            anchors.horizontalCenter: parent.horizontalCenter

            Rectangle {
                objectName: "no diving"
                x: 25
                y: -35
                width: 50
                height: 120
                border.color: "black"
                color: hh.hovered ? "navajowhite" : "whitesmoke"
                HoverHandler {
                    id: hh
                    cursorShape: Qt.ForbiddenCursor
                }
            }
        }
    }
}
