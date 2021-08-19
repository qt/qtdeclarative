import QtQuick
import Test 1.0

Item {
    width: 300
    height: 300

    Rectangle {
        objectName: "outerViewport"

        x: 40
        y: 40
        width: 220
        height: 220
        border.color: "green"
        color: "transparent"

        Rectangle {
            objectName: "innerViewport"
            width: parent.width
            height: parent.height
            x: 20
            y: 20
            border.color: "cyan"
            color: "transparent"

            Rectangle {
                objectName: "innerRect"
                color: "wheat"
                opacity: 0.5
                x: -55
                y: -55
                width: 290
                height: 290
                ViewportTestItem {
                    anchors.fill: parent
                    Rectangle {
                        id: viewportFillingRect
                        color: "transparent"
                        border.color: "red"
                        border.width: 3
                        x: parent.viewport.x
                        y: parent.viewport.y
                        width: parent.viewport.width
                        height: parent.viewport.height
                    }
                }
            }
        }
    }
    Text {
        text: "viewport " + viewportFillingRect.width + " x " + viewportFillingRect.height
    }
}
