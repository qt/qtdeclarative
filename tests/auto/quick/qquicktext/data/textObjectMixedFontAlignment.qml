import QtQuick

Item {
    width: 400
    height: 200
    Text {
        objectName: "baselineText"
        x: 0; y: 0
        width: 400; height: 100
        text: "hello"
        textFormat: Text.RichText
        font.pixelSize: 50
    }
    Text {
        objectName: "bottomText"
        x: 0; y: 100
        width: 400; height: 100
        text: "hello"
        textFormat: Text.RichText
        font.pixelSize: 50
    }
}
