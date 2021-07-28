import QtQuick
import QtQuick.Controls

RangeSlider {
    from: 0
    to: 100
    first.value: 25
    second.value: 75
    stepSize: 1
    orientation: Qt.Horizontal
}
