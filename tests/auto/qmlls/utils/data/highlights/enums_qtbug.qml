import QtQuick
Rectangle {
    MouseArea {
        id: handler
        anchors.fill: parent
        drag.axis: Drag.XAxis
        drag.minimumX: 0
        }
}
