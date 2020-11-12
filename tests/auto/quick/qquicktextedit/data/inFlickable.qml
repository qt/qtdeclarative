import QtQuick 2.0

Flickable {
    width: 320; height: 120; contentHeight: text.height
    TextEdit {
        id: text
        objectName: "text"
        font.pixelSize: 20
        text: "several\nlines\nof\ntext\n-\ntry\nto\nflick"
    }
}
