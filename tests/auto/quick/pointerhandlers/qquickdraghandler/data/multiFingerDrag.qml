import QtQuick

Rectangle {
    id: root
    color: "#333"
    width: 480; height: 480

    PointHandler {
        id: ph1
        acceptedDevices: PointerDevice.TouchScreen
        target: Rectangle {
            parent: root
            color: "cyan"
            visible: ph1.point.pressure > 0
            x: ph1.point.position.x - 3
            y: ph1.point.position.y - 3
            width: 6; height: 6; radius: 3
        }
    }

    PointHandler {
        id: ph2
        acceptedDevices: PointerDevice.TouchScreen
        target: Rectangle {
            parent: root
            color: "lightgreen"
            visible: ph2.point.pressure > 0
            x: ph2.point.position.x - 3
            y: ph2.point.position.y - 3
            width: 6; height: 6; radius: 3
        }
    }

    Rectangle {
        color: dragHandler.active ? "steelblue" : "#112"
        width: 100
        height: 100
        x: 50; y: 50

        DragHandler {
            id: dragHandler
            minimumPointCount: 2
            maximumPointCount: 5
        }
    }
}
