import QtQuick 2.0
import Qt.labs.controls 1.0

Rectangle {
    width: 100
    height: 100
    border.color: Theme.frameColor

    ScrollIndicator {
        size: 0.3
        position: 0.2
        active: true
        orientation: Qt.Vertical
        height: parent.height
        anchors.right: parent.right
    }

    ScrollIndicator {
        size: 0.6
        position: 0.3
        active: true
        orientation: Qt.Horizontal
        width: parent.width
        anchors.bottom: parent.bottom
    }
}
