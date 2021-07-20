import QtQuick
import QtQuick.Controls

Slider {
    from: 0
    to: 100
    value: 50
    stepSize: 1
    orientation: Qt.Horizontal
    Accessible.name: "SliderOverride"
}
