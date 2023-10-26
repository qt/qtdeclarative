import QtQuick

Flickable {
    flickableDirection: Flickable.VerticalFlick
    width: 480
    height: 320
    contentWidth: 480
    contentHeight: 400
    pressDelay: 100

    property real pressX
    property real pressY
    property real releaseX
    property real releaseY

    MouseArea {
        id: ma
        y: 100
        anchors.horizontalCenter: parent.horizontalCenter
        width: 240
        height: 100

        onPressed: {
            pressX = mouse.x
            pressY = mouse.y
        }
        onReleased: {
            releaseX = mouse.x
            releaseY = mouse.y
        }

        Rectangle {
            anchors.fill: parent
            color: parent.pressed ? 'blue' : 'lightsteelblue'
        }
    }
}

