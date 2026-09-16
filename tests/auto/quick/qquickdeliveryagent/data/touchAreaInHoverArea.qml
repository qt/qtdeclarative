import QtQuick

Item {
    width: 200
    height: 260

    MouseArea {
        objectName: "hoverArea"
        width: 200
        height: 200
        hoverEnabled: true

        MultiPointTouchArea {
            anchors.fill: parent
        }
    }
}
