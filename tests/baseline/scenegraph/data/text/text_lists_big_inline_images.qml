import QtQuick 2.0

Item {
    width: 320
    height: 480

    Text {
        anchors.centerIn: parent
        font.family: "Arial"
        font.pixelSize: 16
        textFormat: Text.RichText
        text: "Unordered list:<br/><ul><li>List item 1</li><li>List <img height=32 width=32 src=\"data/logo.png\" />item 2</li><li>List<br/>item<img height=32 width=32 src=\"data/logo.png\" />3</li></ul>" +
              "Numbered list: <br/><ol><li>List item 1</li><li>List <img height=32 width=32 src=\"data/logo.png\" />item 2</li><li>List<br/>item<img height=32 width=32 src=\"data/logo.png\" />3</li></ol>" +
              "and some more text"
    }
}
