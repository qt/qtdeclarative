import QtQuick 2.0

Rectangle {
    width: 200
    height: 100

    TextEdit {
        objectName: "textEditItem"
        text: "AA\nBBBBBBB\nCCCCCCCCCCCCCCCC"
        anchors.centerIn: parent
        horizontalAlignment: TextEdit.AlignLeft
        font.pointSize: 12
        font.family: "Times New Roman"
    }
}
