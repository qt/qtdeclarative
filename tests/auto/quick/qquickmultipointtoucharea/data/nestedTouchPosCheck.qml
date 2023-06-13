import QtQuick
import QtTest

Item {
    width: 300; height: 200
    id: topLevelItem
    MultiPointTouchArea {
        objectName: "topMPTA"
        anchors.fill: parent
        Column {
            width: parent.width
            height: parent.height

            Rectangle {
                width: parent.width
                height: 100
                color: "green"
            }
            Rectangle {
                id: rect
                width: parent.width
                height: 600
                color: "red"

                MultiPointTouchArea {

                    property var xPressed: 0
                    property var yPressed: 0
                    property var xReleased: 0
                    property var yReleased: 0

                    objectName: "bottomMPTA"
                    anchors.fill: parent
                    onPressed: (touchPoints) => {
                       for (let tp in touchPoints) {
                           let touch = touchPoints[tp]
                           xPressed = touch.x
                           yPressed = touch.y
                       }
                    }
                    onReleased: (touchPoints) => {
                        for (let tp in touchPoints) {
                            let touch = touchPoints[tp]
                            xReleased = touch.x
                            yReleased = touch.y
                        }
                    }
                }
            }
        }
    }
}
