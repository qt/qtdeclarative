import QtQuick 2.0

Rectangle {
    width: 100
    height: 62

    Timer {
        running: true
        repeat: true
        interval: 50
        onTriggered: height = (2 * height) % 99;
    }
}

