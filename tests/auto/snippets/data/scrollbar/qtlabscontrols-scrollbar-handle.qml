import QtQuick 2.0
import Qt.labs.controls 1.0

ScrollBar {
    size: 0.5
    position: 0.5
    active: true
    height: 100
    Rectangle {
        parent: handle
        anchors.fill: parent
        color: "transparent"
        border.color: "red"
    }
}
