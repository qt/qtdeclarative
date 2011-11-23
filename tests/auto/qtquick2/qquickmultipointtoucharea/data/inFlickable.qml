import QtQuick 2.0

Flickable {
    width: 240
    height: 320

    contentWidth: width
    contentHeight: height * 2

    MultiPointTouchArea {
        anchors.fill: parent
        minimumTouchPoints: 2
        maximumTouchPoints: 2
        onGestureStarted: {
            if ((Math.abs(point2.x - point2.startX) > gesture.dragThreshold/2) && (Math.abs(point1.x - point1.startX) > gesture.dragThreshold/2)) {
                gesture.grab()
            }
        }
        touchPoints: [
            TouchPoint { id: point1; objectName: "point1" },
            TouchPoint { id: point2; objectName: "point2" }
        ]
    }
}

