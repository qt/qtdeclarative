import QtQuick 2.0

Rectangle {
    width: 200
    height: 200
    color: "white"
    Text {
        objectName: "text"
        textFormat: Text.RichText
        anchors.fill: parent
        color: "black"
        // display a black rectangle at the top left of the text
        text: "<span style=\"background-color:rgba(255,255,255,255);vertical-align:super;\">&#x2588;</span>This is a test"
        verticalAlignment: Text.AlignTop
        horizontalAlignment: Text.AlignLeft
        font.pixelSize: 30
    }
}
