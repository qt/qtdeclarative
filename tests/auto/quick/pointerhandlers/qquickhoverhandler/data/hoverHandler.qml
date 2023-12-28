import QtQuick

Item {
    width: 320
    height: 240

    Rectangle {
        width: 100
        height: 100
        anchors.centerIn: parent
        color: hh.hovered ? "lightsteelblue" : "beige"

        HoverHandler {
            id: hh
        }
    }
}
