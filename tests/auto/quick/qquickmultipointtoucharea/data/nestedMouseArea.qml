import QtQuick 2.12

Item {
    id: root
    width: 300; height: 300
    property point mptaPoint
    property point maPoint
    MultiPointTouchArea {
        anchors.fill : parent
        onPressed: function(touchPoints) {
            root.mptaPoint = Qt.point(touchPoints[0].x, touchPoints[0].y)
        }
        MouseArea {
            id: ma
            width: 100; height: 100
            anchors.centerIn: parent
            onPressed: function(mouse) {
                root.maPoint = Qt.point(mouse.x, mouse.y)
            }
        }
        Rectangle {
            anchors.fill: ma
            border.color: "grey"
        }
    }
}
