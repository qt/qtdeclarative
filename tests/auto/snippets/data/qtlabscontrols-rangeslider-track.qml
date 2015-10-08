import QtQuick 2.0
import Qt.labs.controls 1.0

RangeSlider {
    first.value: 0.25
    second.value: 0.75
    Rectangle {
        anchors.fill: track
        color: "transparent"
        border.color: "red"
    }
}
