import QtQuick 2.12

Rectangle {
    width: 200; height: 200

    DragHandler { }

    Rectangle {
        objectName: "button"
        width: 100; height: 40; x: 10; y: 10
        border.color: "orange"
        color: ma.pressed ? "lightsteelblue" : "beige"

        MouseArea {
            id: ma
            anchors.fill: parent
            onDoubleClicked: console.log("__")
        }
    }
}
