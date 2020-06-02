import QtQuick 2.10

Rectangle {
    width: 640
    height: 480
    color: "red"
    TextEdit {
        id: textEdit
        anchors.top: parent.top
        anchors.left: parent.left
        width: contentWidth
        height: contentHeight
        font.pixelSize: 50
        text: "          "
        selectionColor: "transparent"
    }
}
