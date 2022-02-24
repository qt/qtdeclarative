import QtQuick
import QtQuick.Layouts

ColumnLayout {
    height: 200
    width: 400
    spacing: -6
    Rectangle {
        border.color: top.activeFocus ? "steelblue" : "lightgrey"
        Layout.fillHeight: true
        Layout.fillWidth: true
        Layout.margins: 6
        TextEdit {
            id: top
            objectName: "top"
            text: "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            anchors.fill: parent
        }
    }
    Rectangle {
        border.color: bottom.activeFocus ? "steelblue" : "lightgrey"
        Layout.fillHeight: true
        Layout.fillWidth: true
        Layout.margins: 6
        TextEdit {
            id: bottom
            objectName: "bottom"
            text: "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            anchors.fill: parent
        }
    }
}
