
import QtQuick

Rectangle {
    x: 20
    y: 20
    width: 300
    height: 200

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
    }

    MultiPointTouchArea {
        id: touchArea
        anchors.fill: parent
        enabled: false
    }
}
