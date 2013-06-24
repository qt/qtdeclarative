import QtQuick 2.0

Rectangle {
    Rectangle {
        width: 10
        height: 10
        color: "blue"
    }

    Component.onCompleted: timer.start();

    Timer {
        id: timer
        interval: 100 // 100 ms, enough for at least one frame
        running: false
        onTriggered: console.log("tick")
    }
}
