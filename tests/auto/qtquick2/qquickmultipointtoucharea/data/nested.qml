import QtQuick 2.0

MultiPointTouchArea {
    width: 240
    height: 320

    property bool grabInnerArea: true

    minimumTouchPoints: 2
    maximumTouchPoints: 3
    touchPoints: [
        TouchPoint { objectName: "point11" },
        TouchPoint { objectName: "point12" }
    ]

    MultiPointTouchArea {
        anchors.fill: parent
        minimumTouchPoints: 3
        maximumTouchPoints: 3
        onGestureStarted: if (grabInnerArea) gesture.grab()
        touchPoints: [
            TouchPoint { objectName: "point21" },
            TouchPoint { objectName: "point22" },
            TouchPoint { objectName: "point23" }
        ]
    }
}
