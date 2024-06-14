import QtQuick

TextEdit {
    id: te
    property int sourceChangeCount: 0
    property int modifiedChangeCount: 0
    property var statusHistory: []

    width: 320; height: 240
    text: "Hello Qt"
    focus: true

    textDocument.onSourceChanged: ++te.sourceChangeCount
    textDocument.onModifiedChanged: ++te.modifiedChangeCount
    textDocument.onStatusChanged: te.statusHistory.push(textDocument.status)
}
