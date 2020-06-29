import QtQuick 2.15
import QtQuick.Window 2.15

Item {
    visible: true
    width: 200
    height: 200

    Rectangle {
        anchors.centerIn: parent
        width: 50
        height: 50
        color: hh.hovered ? "tomato" : "wheat"
        HoverHandler {
            id: hh
            margin: 20
            cursorShape: Qt.OpenHandCursor
        }
    }
}
