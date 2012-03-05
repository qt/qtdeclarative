import QtQuick 2.0
import Test 1.0

Rectangle {
    width: 200
    height: 200
    color: "black"
    Rectangle {
        width: 200
        height: 100
        anchors.centerIn: parent
        clip: true
        color: "white"
        Rectangle {
            width: 100
            height: 100
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
