import QtQuick

Rectangle {
    id: root
    objectName: "root"
    color: ph.active ? "coral" : "cadetblue"
    border.color: "black"
    width: 100; height: 100
    PointHandler {
        id: ph
        objectName: root.objectName + "PointHandler"
    }
    Text {
        anchors.centerIn: parent
        text: ph.point.position.x.toFixed(1) + ", " + ph.point.position.y.toFixed(1)
    }
}
