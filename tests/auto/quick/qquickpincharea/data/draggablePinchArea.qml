import QtQuick 2.12

Rectangle {
    id: root
    width: 600
    height: 600

    Rectangle {
        objectName: "paContainer"
        width: parent.width -100
        height: parent.height - 100
        border.color: "black"
        anchors.centerIn: parent
        transformOrigin: Item.Center

        Rectangle {
            width: 300
            height: 300
            color: "tomato"
            PinchArea {
                id: pa
                anchors.fill: parent
                pinch.target: parent
                pinch.minimumScale: 0.5
                pinch.maximumScale: 2
                pinch.minimumRotation: -360
                pinch.maximumRotation: 360
                pinch.dragAxis: Pinch.XAndYAxis
                pinch.minimumX: -100
                pinch.maximumX: 300
                pinch.minimumY: -100
                pinch.maximumY: 300
            }


            Text { text: "this way up" }
        }
    }

    // only for touch feedback / troubleshooting
    Item {
        id: glassPane
        z: 10000
        anchors.fill: parent

        PointHandler {
            id: ph1
            target: Rectangle {
                parent: glassPane
                color: "green"
                visible: ph1.active
                x: ph1.point.position.x - width / 2
                y: ph1.point.position.y - height / 2
                width: 20; height: width; radius: width / 2
            }
        }

        PointHandler {
            id: ph2
            target: Rectangle {
                parent: glassPane
                color: "blue"
                visible: ph2.active
                x: ph2.point.position.x - width / 2
                y: ph2.point.position.y - height / 2
                width: 20; height: width; radius: width / 2
            }
        }
    }
}
