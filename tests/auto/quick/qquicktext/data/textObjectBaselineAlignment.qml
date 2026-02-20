import QtQuick

Item {
    width: 400
    height: 300
    Text {
        objectName: "baselineText"
        x: 0; y: 0
        width: 400; height: 100
        text: "hello"
        textFormat: Text.RichText
        font.pixelSize: 30
    }
    Text {
        objectName: "bottomText"
        x: 0; y: 100
        width: 400; height: 100
        text: "hello"
        textFormat: Text.RichText
        font.pixelSize: 30
    }
    Text {
        objectName: "topText"
        x: 0; y: 200
        width: 400; height: 100
        text: "hello"
        textFormat: Text.RichText
        font.pixelSize: 30
    }
}
