import QtQuick 2.0
import Test 1.0

Rectangle {
    width: 320
    height: 480
    color: "black"
    Rectangle {
        width: 320
        height: 240
        anchors.centerIn: parent
        clip: true
        color: "white"
        Rectangle {
            width: 160
            height: 240
            anchors.centerIn: parent
            rotation: 45
            color: "blue"
            clip: true
            MessUpItem {
                anchors.fill: parent
            }
            Rectangle {
                anchors.fill: parent
                anchors.margins: -50
                color: "red"
                opacity: 0.5
            }
        }
    }
}
