import QtQuick 2.0
import Qt.labs.controls 1.0

SpinBox {
    value: 50
    Rectangle {
        anchors.fill: up.indicator
        color: "transparent"
        border.color: "red"
    }
}
