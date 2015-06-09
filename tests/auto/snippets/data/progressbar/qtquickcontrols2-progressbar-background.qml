import QtQuick 2.0
import QtQuick.Controls 2.0

ProgressBar {
    value: 0.5
    Rectangle {
        anchors.fill: background
        color: "transparent"
        border.color: "red"
    }
}
