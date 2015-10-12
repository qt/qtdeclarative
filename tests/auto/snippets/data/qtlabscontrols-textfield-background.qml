import QtQuick 2.0
import Qt.labs.controls 1.0

TextField {
    width: 80
    text: "TextField"
    Rectangle {
        anchors.fill: background
        color: 'transparent'
        border.color: 'red'
    }
}
