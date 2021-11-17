//main.qml

import QtQuick 2.12
import QtQuick.Window 2.12

Item {
    id: root
    width: 640
    height: 480
    visible: true
    property bool running : false

    Rectangle {
        id: rect
        anchors.fill: parent
        color: "red"

        Component.onCompleted: {
            anim.start()
            running = true
        }
    }

    OpacityAnimator {
        id: anim

        target: rect
        from: 1
        to: 0
        duration: 20000
    }
}
