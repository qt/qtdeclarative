import QtQuick.Controls

Pane {
    width: 400
    height: 400

    property alias editor: spinBox.contentItem

    SpinBox {
        id: spinBox
        value: 123456
        to: 999999999
        editable: true
        width: parent.width
    }
}
