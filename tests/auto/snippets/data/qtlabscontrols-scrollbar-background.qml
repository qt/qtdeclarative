import QtQuick 2.0
import Qt.labs.controls 1.0

ScrollBar {
    size: 0.5
    position: 0.5
    active: true
    height: 100
    background: Rectangle {
        color: "transparent"
        border.color: "red"
    }
}
