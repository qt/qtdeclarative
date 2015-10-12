import QtQuick 2.0
import Qt.labs.controls 1.0

ProgressBar {
    value: 0.5
    Rectangle {
        anchors.fill: background
        color: "transparent"
        border.color: "red"
    }
}
