import QtQuick
import QtQuick.Layouts

ColumnLayout {
    height: 100
    TextInput {
        objectName: "top"
        text: "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    }
    TextInput {
        objectName: "bottom"
        text: "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    }
}
