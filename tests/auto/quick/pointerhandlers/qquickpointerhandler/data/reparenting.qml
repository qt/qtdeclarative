import QtQuick
import Qt.test

Item {
    id: root
    objectName: "root Item"
    width: 320
    height: 480

    Rectangle {
        id: top
        objectName: "top"
        width: parent.width - 12
        height: parent.height / 2 - 9
        x: 6; y: 6
        color: "teal"
        border.width: 3
        border.color: handler.parent == top ? "brown" : "transparent"
        EventHandler {
            id: handler
            objectName: "eventHandler"
        }
    }

    Rectangle {
        id: bottom
        objectName: "bottom"
        width: parent.width - 12
        height: parent.height / 2 - 9
        x: 6; y: parent.height / 2 + 3
        color: "darkolivegreen"
        border.width: 3
        border.color: handler.parent == bottom ? "brown" : "transparent"
    }

    Timer {
        interval: 1000; running: true; repeat: true
        onTriggered: handler.parent = (handler.parent == top ? bottom : top)
    }
}
