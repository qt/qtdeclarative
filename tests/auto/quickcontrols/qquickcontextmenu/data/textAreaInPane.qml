import QtQuick.Controls

Pane {
    width: 400
    height: 400

    property alias editor: textArea

    TextArea {
        id: textArea
        text: "Memento mori"
        width: parent.width
    }
}
