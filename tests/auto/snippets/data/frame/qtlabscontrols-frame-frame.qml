import QtQuick 2.0
import Qt.labs.controls 1.0

Frame {
    width: 100
    height: 100
    Rectangle {
        parent: frame
        anchors.fill: parent
        color: 'transparent'
        border.color: 'red'
    }
}
