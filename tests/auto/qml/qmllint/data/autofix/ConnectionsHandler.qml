import QtQuick

Window {
    visible: true
    width: 500
    height: 500
    id: root

    Connections {
        target: root
        onWidthChanged: console.log("new width:", width)
        onColorChanged: (collie) => console.log("new color:", collie)
    }
}
