import QtQuick

Rectangle {
    width: 200
    height: 60
    color: "blue"

    TextInput {
        anchors.fill: parent
        focus: true
        persistentSelection: true
        font.pixelSize: 30
        text: "mmmmm"
        color: "black"
        selectionColor: "red"
        selectedTextColor: "white"
    }
}
