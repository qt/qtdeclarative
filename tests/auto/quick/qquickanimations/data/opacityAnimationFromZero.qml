import QtQuick 2.14
import QtQuick.Window 2.14

Window {
    width: 640
    height: 480
    Rectangle {
        id: rect
        width: 200
        height: 200
        color: "black"
        OpacityAnimator {
            target: rect
            from: 0
            to: 1
            duration: 1000
            running: true
        }
    }
}
