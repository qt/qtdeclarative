import QtQuick 2.0
import Qt.labs.controls 1.0

Tumbler {
    model: 5

    Rectangle {
        anchors.fill: parent.contentItem
        color: 'transparent'
        border.color: 'red'
    }
}
