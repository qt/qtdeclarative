import QtQuick 2.0

Flickable {
    id: flick
    width: 320; height: 120; contentHeight: text.height
    TextEdit {
        id: text
        objectName: "text"
        font.pixelSize: 20
        text: "several\nlines\nof\ntext\n-\ntry\nto\nflick"
    }
    Text {
        color: "red"
        parent: flick // stay on top
        anchors.right: parent.right
        text: flick.contentY.toFixed(1)
    }
}
