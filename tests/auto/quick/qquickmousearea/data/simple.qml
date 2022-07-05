import QtQuick 2.0

Rectangle {
    id: whiteRect
    width: 200
    height: 200
    color: ma.pressed ? "lightsteelblue" : "white"
    MouseArea {
        id: ma
        objectName: "mousearea"
        anchors.fill: parent
    }
}
