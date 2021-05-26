import QtQuick

Rectangle {
    width: 320
    height: 480
    color: "white"
    Column {
        anchors.centerIn: parent
        spacing: 8
        Text { font.pointSize: 8; text: "No mirror" }
        Image {
            width: 128
            height: 120
            source: "../shared/winter.png"
        }
        Text { font.pointSize: 8; text: "Horizontal / Vertical" }
        Row {
            spacing: 8
            Image {
                width: 128
                height: 120
                source: "../shared/winter.png"
                mirror: true
            }
            Image {
                width: 128
                height: 120
                source: "../shared/winter.png"
                mirrorVertically: true
            }
        }
        Text { font.pointSize: 8; text: "Horizontal + Vertical" }
        Image {
            width: 128
            height: 120
            source: "../shared/winter.png"
            mirror: true
            mirrorVertically: true
        }
    }
}
