import QtQuick

Rectangle {
    id: root
    width: 240
    height: 320

    property bool gestureStarted: false

    Flickable {
        id: flick
        anchors.fill: parent
        anchors.margins: 50
        clip: true
        contentWidth: width
        contentHeight: height * 2

        scale: 0.5

        MultiPointTouchArea {
            anchors.fill: parent
            onGestureStarted: (gesture) => {
                root.gestureStarted = true
            }
        }
    }
}
