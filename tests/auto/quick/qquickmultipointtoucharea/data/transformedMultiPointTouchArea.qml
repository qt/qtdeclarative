import QtQuick 2.0

Rectangle {
    width: 400
    height: 400

    Rectangle {
        x: 100
        y: 100
        width: 200
        height: 200
        rotation: 45

        MultiPointTouchArea {
            scale: 0.5
            anchors.fill: parent
            maximumTouchPoints: 5
            objectName: "touchArea"

            property int pointCount: 0
            property point startPosition : Qt.point(0,0)

            onPressed: (points) => pointCount = points.length;
            onTouchUpdated: (points) => {
                pointCount = points.length
                if (pointCount > 0) {
                    let p = points[0]
                    startPosition = Qt.point(p.startX, p.startY)
                }
            }
        }
    }
}
