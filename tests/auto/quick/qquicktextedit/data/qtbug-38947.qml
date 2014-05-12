import QtQuick 2.0

TextEdit {
    text: "Hello\nWorld!"
    selectByMouse: true
    cursorDelegate: Rectangle {
        width: 10
        color: "transparent"
        border.color: "red"
    }
}
