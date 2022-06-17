import QtQuick

TextEdit {
    id: te
    width: 200
    height: 200
    textFormat: TextEdit.MarkdownText
    Component.onCompleted: te.insert(te.length, "*whee*")
}
