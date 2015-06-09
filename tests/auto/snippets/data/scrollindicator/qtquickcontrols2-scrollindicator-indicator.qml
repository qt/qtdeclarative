import QtQuick 2.0
import QtQuick.Controls 2.0

ScrollIndicator {
    size: 0.5
    position: 0.5
    active: true
    height: 100
    Rectangle {
        parent: indicator
        anchors.fill: parent
        color: "transparent"
        border.color: "red"
    }
}
