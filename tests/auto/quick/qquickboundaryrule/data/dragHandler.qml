import QtQuick 2.14
import Qt.labs.animation 1.0

Rectangle {
    id: root
    width: 240; height: 120
    color: "green"

    DragHandler {
        id: dragHandler
        yAxis.minimum: -1000
        xAxis.minimum: -1000
        onActiveChanged: if (!active) xbr.returnToBounds();
    }

    BoundaryRule on x {
        objectName: "boundaryRule"
        id: xbr
        minimum: -50
        maximum: 100
        minimumOvershoot: 40
        maximumOvershoot: 40
    }
}
