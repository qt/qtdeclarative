import QtQuick 2.0
import Qt.labs.controls 1.0

GroupBox {
    width: 100
    height: 100
    title: "GroupBox"
    Rectangle {
        parent: frame
        anchors.fill: parent
        color: 'transparent'
        border.color: 'red'
    }
}
