import QtQuick

Row {
    width: 480; height: 200
    TextEdit {
        objectName: "plain"
        width: parent.width / 2 - 1
        textFormat: TextEdit.PlainText
        textDocument.source: "hello.md"
    }
    Rectangle {
        width: 2; height: parent.height
        color: "lightsteelblue"
    }
    TextEdit {
        objectName: "markdown"
        width: parent.width / 2 - 1
        textFormat: TextEdit.MarkdownText
        textDocument.source: "hello.md"
    }
}
