import QtQuick 2.0
import Test 1.0

Rectangle {
    id: root

    width: 200
    height: 200
    color: "black"

    Rectangle {
        width: 100
        height: 100
        anchors.top: parent.top
        anchors.left: parent.left
        color: "red"
        opacity: 0.5
    }

    Rectangle {
        width: 100
        height: 100
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        color: "red"
    }

    ClearItem {
        width: 100
        height: 100
        anchors.centerIn: parent
        color: "white"
        clip: true
    }

    Rectangle {
        width: 100
        height: 100
        anchors.top: parent.top
        anchors.right: parent.right
        color: "blue"
    }

    Rectangle {
        width: 100
        height: 100
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        color: "blue"
        opacity: 0.5
    }

}
