import QtQuick 2.0

TextEdit {
    focus: true

    cursorDelegate: Item {
        objectName: "cursor"
    }
}
