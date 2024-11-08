import QtQuick

Rectangle {
    width: 400; height: 400

    Rectangle {
        width: 100; height: 100
        color: mousetracker.containsMouse ? "lightsteelblue" : "beige"

        MouseArea {
            id: mousetracker
            anchors.fill: parent
            hoverEnabled: true
        }
    }
}
