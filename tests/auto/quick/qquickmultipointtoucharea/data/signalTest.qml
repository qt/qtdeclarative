import QtQuick 2.0

MultiPointTouchArea {
    width: 240
    height: 320

    function clearCounts() {
        touchPointPressCount = 0;
        touchPointUpdateCount = 0;
        touchPointReleaseCount = 0;
        touchCount = 0;
        touchUpdatedHandled = false;
    }

    property int touchPointPressCount: 0
    property int touchPointUpdateCount: 0
    property int touchPointReleaseCount: 0
    property int touchCount: 0
    property bool touchUpdatedHandled: false

    maximumTouchPoints: 5

    // recommended syntax for a signal handler
    onPressed: (points) => { touchPointPressCount = points.length }

    // one with "touchPoints" being the signal argument rather than the property
    onUpdated: (touchPoints) => { touchPointUpdateCount = touchPoints.length }

    // one without the formal parameter, to test that it still works (with a warning)
    onReleased: { touchPointReleaseCount = touchPoints.length }

    onTouchUpdated: (points) => {
        touchCount = points.length
        touchUpdatedHandled = true
    }
}
