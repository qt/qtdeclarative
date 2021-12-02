import QtQuick 2.15
import QtQuick.Window 2.15

Window {
    width: 320
    height: 240
    visible: true
    color: hh.hovered ? "lightsteelblue" : "beige"
    HoverHandler {
        id: hh
        cursorShape: Qt.OpenHandCursor
    }
}
