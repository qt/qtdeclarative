import QtQuick 2.0
import Qt.labs.controls 1.0

Dial {
    id: dial

    Rectangle {
        parent: dial.handle
        anchors.fill: parent
        color: 'transparent'
        border.color: 'red'
        antialiasing: true
    }
}
