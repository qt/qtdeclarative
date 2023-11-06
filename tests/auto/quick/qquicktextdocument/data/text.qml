import QtQuick

TextEdit {
    id: te
    property int sourceChangeCount: 0
    property int modifiedChangeCount: 0

    text: "" // this is not a document modification

    textDocument.onSourceChanged: ++te.sourceChangeCount
    textDocument.onModifiedChanged: ++te.modifiedChangeCount
}
