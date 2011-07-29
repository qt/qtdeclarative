import QtQuick 2.0

Rectangle {
    id: root
    color: "#ffffff"
    width: 320; height: 240
    property bool pressed:mouse.pressed
    property bool canceled: false
    property bool released: false

    MouseArea {
        id: mouse
        anchors.fill: parent
        onPressed: { root.canceled = false }
        onCanceled: {root.canceled = true}
        onReleased: {root.released = true; root.canceled = false}
    }
}