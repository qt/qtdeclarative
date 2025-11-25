import QtQuick.Controls

Pane {
    width: 400
    height: 400

    property alias editor: textField

    TextField {
        id: textField
        text: "123,456"
        width: parent.width
    }
}
