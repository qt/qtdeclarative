import QtQuick
import Qt.test 1.0

Item {
    width: 200
    height: 200

    Rectangle {
        id: circle
        y: 0
        width: 100
        height: width
        radius: width/2
        color: "#3e1"
        clip: true

        // Rectangle contains() is not affected by its 'radius' property
        containmentMask: QtObject {
            property alias radius: circle.radius
            function contains(point: point) : bool {
                return (Math.pow(point.x - radius, 2) + Math.pow(point.y - radius, 2)) < Math.pow(radius, 2)
            }
        }
        EventHandler {
            objectName: "circle eventHandler"
        }
        Rectangle {
            width: circle.width/2
            height: width
            color: "red"
            EventHandler {
                objectName: "eventHandler"
            }
        }
    }
}
