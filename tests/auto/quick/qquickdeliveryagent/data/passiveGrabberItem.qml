import QtQuick

import Test

Item {
    width: 320
    height: 240
    property alias exclusiveGrabber: exGrabber
    property alias passiveGrabber: psGrabber
    ExclusiveGrabber {
        id: exGrabber
        width: parent.width
        height: 30
        color: "blue"
        anchors.verticalCenter: parent.verticalCenter
        Text {
            anchors.centerIn: parent
            text: "Exclusive Grabber"
        }
    }
    PassiveGrabber {
        id: psGrabber
        width: parent.width * 0.3
        height: parent.height
        color: "yellow"
        opacity: 0.5
        Text {
            anchors.centerIn: parent
            text: "Passive Grabber"
            rotation: 90
        }
    }
}

