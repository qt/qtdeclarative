import QtQuick.Controls

Pane {
    width: 400
    height: 400

    property alias editor: textArea

    TextArea {
        id: textArea
        text: "123,456"
        width: parent.width
    }
}
