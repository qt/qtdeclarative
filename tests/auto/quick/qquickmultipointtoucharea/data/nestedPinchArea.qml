import QtQuick 2.15

MultiPointTouchArea {
    width: 240
    height: 320
    mouseEnabled: true
    property int pressedCount: 0
    property int updatedCount: 0
    property int releasedCount: 0

    onPressed: (points) => { pressedCount = points.length }
    onUpdated: (points) => { updatedCount = points.length }
    onReleased: (points) => { releasedCount = points.length }

    touchPoints: [
        TouchPoint {
            id: point1
            objectName: "point1"
        },
        TouchPoint {
            id: point2
            objectName: "point2"
        }
    ]

    PinchArea {
        anchors.fill: parent
    }

    Rectangle {
        width: 30; height: 30
        color: "green"
        x: point1.x
        y: point1.y
    }

    Rectangle {
        id: rectangle
        width: 30; height: 30
        color: "yellow"
        x: point2.x
        y: point2.y
    }
}
