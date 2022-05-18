import QtQuick 2.15

ListView {
    id: flick
    width: 640
    height: 480
    model: 100

    delegate: Rectangle {
        border.color: "#81e889"
        width: 640; height: 100
        Text { text: "Row " + index }
    }

    Rectangle {
        anchors.right: parent.right
        anchors.margins: 2
        color: ma.pressed ? "#81e889" : "#c2f4c6"
        width: 50; height: 50
        radius: 5
        MouseArea {
            id: ma
            anchors.fill: parent
            preventStealing: true
            drag {
                target: parent
                axis: Drag.YAxis
            }
        }
    }
}
