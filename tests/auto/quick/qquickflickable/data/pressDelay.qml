import QtQuick 2.0

Flickable {
    flickableDirection: Flickable.VerticalFlick
    width: 480
    height: 320
    contentWidth: 480
    contentHeight: 400
    pressDelay: 100

    MouseArea {
        id: ma
        objectName: "mouseArea"
        y: 100
        anchors.horizontalCenter: parent.horizontalCenter
        width: 240
        height: 100

        Rectangle {
            anchors.fill: parent
            color: parent.pressed ? 'blue' : 'green'

            Text {
                anchors.fill: parent
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
                text: 'Hello'
            }
        }
    }
}

