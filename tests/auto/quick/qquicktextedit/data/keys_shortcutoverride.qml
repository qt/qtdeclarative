import QtQuick 2.10

Item {
    width: 320
    height: 200
    property string who : "nobody"

    Shortcut {
        sequence: "Esc"
        onActivated: who = "Shortcut"
    }

    TextEdit {
        id: txt
        x: 100
        text: "enter text"
        Keys.onShortcutOverride: {
            who = "TextEdit"
            event.accepted = (event.key === Qt.Key_Escape)
        }
    }

    Rectangle {
        objectName: "rectangle"
        width: 90
        height: width
        focus: true
        color: focus ? "red" : "gray"
        Keys.onShortcutOverride: {
            who = "Rectangle"
            event.accepted = (event.key === Qt.Key_Escape)
        }
    }
}
