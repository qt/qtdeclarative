import QtQuick 2.0

Rectangle {
    id: root
    width: 240
    height: 320

    property int cancelCount: 0
    property int touchCount: 0

    Rectangle {
        id: verticalScrollDecorator
        anchors.right: parent.right
        anchors.margins: 2
        color: flick.moving ? "goldenrod" : "cyan"
        border.color: "black"
        border.width: 1
        width: 5
        radius: 2
        antialiasing: true
        height: flick.height * (flick.height / flick.contentHeight) - (width - anchors.margins) * 2
        y:  -flick.contentY * (flick.height / flick.contentHeight)
    }

    Flickable {
        id: flick
        anchors.fill: parent
        contentWidth: width
        contentHeight: height * 2

        MultiPointTouchArea {
            anchors.fill: parent
            minimumTouchPoints: 2
            maximumTouchPoints: 2
            onGestureStarted: (gesture) => {
                if ((Math.abs(point2.x - point2.startX) > gesture.dragThreshold/2) &&
                        (Math.abs(point1.x - point1.startX) > gesture.dragThreshold/2)) {
                    gesture.grab()
                }
            }
            touchPoints: [
                TouchPoint { id: point1; objectName: "point1" },
                TouchPoint { id: point2; objectName: "point2" }
            ]

            onCanceled: (touchPoints) => root.cancelCount = touchPoints.length
            onTouchUpdated: (touchPoints) => root.touchCount = touchPoints.length

            Text {
                text: "①"
                font.pixelSize: 30
                visible: point1.pressed
                x: point1.x - width / 2; y: point1.y - height / 2
            }

            Text {
                text: "②"
                font.pixelSize: 30
                color: "darkgreen"
                visible: point2.pressed
                x: point2.x - width / 2; y: point2.y - height / 2
            }
        }
    }
}
