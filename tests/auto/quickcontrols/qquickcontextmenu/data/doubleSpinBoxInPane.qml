import QtQuick.Controls

Pane {
    width: 400
    height: 400

    property alias editor: doubleSpinBox.contentItem

    DoubleSpinBox {
        id: doubleSpinBox
        value: 123456
        to: 999999999
        decimals: 0
        editable: true
        width: parent.width
        anchors.centerIn: parent
    }
}
