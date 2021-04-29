import QtQuick

Rectangle {
    id: root
    objectName: "root"
    color: th.pressed ? "goldenrod" : "khaki"
    border.color: "black"
    width: 100; height: 100
    TapHandler {
        id: th
        objectName: root.objectName + "Tap"
        onTapped: console.log(this, objectName)
    }
}
