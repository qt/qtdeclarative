import QtQuick

// Regression scene for QTBUG-149576: loading via textDocument.source must emit
// textChanged, so a binding on the text property reflects the loaded content.
TextEdit {
    width: 320; height: 240
    textDocument.source: "hello.txt"
    property int textChangedCount: 0
    property string boundText: text     // must reflect the loaded file content
    onTextChanged: ++textChangedCount
}
