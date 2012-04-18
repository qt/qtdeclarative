import QtQuick 2.0

TextInput {
    focus: true
    autoScroll: false

    cursorDelegate: Item { objectName: "cursor" }
}
