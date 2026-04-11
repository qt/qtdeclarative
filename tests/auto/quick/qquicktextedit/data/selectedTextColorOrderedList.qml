import QtQuick

Rectangle {
    width: 320
    height: 240
    color: "white"
    TextEdit {
        id: textEdit
        objectName: "textEdit"
        anchors.fill: parent
        textFormat: TextEdit.RichText
        selectedTextColor: "red"
        selectionColor: "green"
        font.pixelSize: 20
        text: "<ol><li>abc</li><li>def</li></ol>"
    }
}
