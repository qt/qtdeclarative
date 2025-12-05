import QtQuick.Controls

Pane {
    width: 400
    height: 400

    property alias editor: comboBox.contentItem

    ComboBox {
        id: comboBox
        model: ["123,456"]
        editable: true
        width: parent.width
    }
}
