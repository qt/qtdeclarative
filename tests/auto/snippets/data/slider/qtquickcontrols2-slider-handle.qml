import QtQuick 2.0
import QtQuick.Controls 2.0

Slider {
    value: 0.5
    Rectangle {
        anchors.fill: handle
        color: "transparent"
        border.color: "red"
    }
}
