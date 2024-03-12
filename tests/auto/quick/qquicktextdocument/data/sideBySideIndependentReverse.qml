import QtQuick

Row {
    width: 480; height: 200
    TextEdit {
        objectName: "plain"
        width: parent.width / 2 - 1
        textDocument.source: "hello.md"
        textFormat: TextEdit.PlainText
    }
    Rectangle {
        width: 2; height: parent.height
        color: "lightsteelblue"
    }
    TextEdit {
        objectName: "markdown"
        width: parent.width / 2 - 1
        textDocument.source: "hello.md"
        textFormat: TextEdit.MarkdownText
    }
}
