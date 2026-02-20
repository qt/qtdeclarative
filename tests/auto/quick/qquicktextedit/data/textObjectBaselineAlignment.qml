import QtQuick

Item {
    width: 400
    height: 300
    TextEdit {
        objectName: "baselineText"
        x: 0; y: 0
        width: 400; height: 100
        text: "hello"
        textFormat: TextEdit.RichText
        font.pixelSize: 30
    }
    TextEdit {
        objectName: "bottomText"
        x: 0; y: 100
        width: 400; height: 100
        text: "hello"
        textFormat: TextEdit.RichText
        font.pixelSize: 30
    }
    TextEdit {
        objectName: "topText"
        x: 0; y: 200
        width: 400; height: 100
        text: "hello"
        textFormat: TextEdit.RichText
        font.pixelSize: 30
    }
}
