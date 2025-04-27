import QtQuick 2.15

Rectangle {
    color: "#333"
    height: 480
    objectName: "dragRotation"
    rotation: 90
    width: 480

    Rectangle {
        id: ballX

        color: dragHandlerX.active ? "blue" : "lightsteelblue"
        height: 80
        objectName: "ballX"
        radius: width / 2
        width: 80
        x: 100
        y: 200

        DragHandler {
            id: dragHandlerX

            cursorShape: Qt.IBeamCursor
            xAxis.enabled: true
            yAxis.enabled: false
        }
        Text {
            anchors.centerIn: parent
            color: "white"
            horizontalAlignment: Text.AlignHCenter
            text: ballX.objectName + "\n"
                + dragHandlerX.centroid.position.x.toFixed(1) + ","
                + dragHandlerX.centroid.position.y.toFixed(1) + "\n"
                + ballX.x.toFixed(1) + "," + ballX.y.toFixed(1)
        }
    }
    Rectangle {
        id: ballY

        color: dragHandlerY.active ? "blue" : "lightsteelblue"
        height: 80
        objectName: "ballY"
        radius: width / 2
        width: 80
        x: 300
        y: 200

        DragHandler {
            id: dragHandlerY

            cursorShape: Qt.IBeamCursor
            xAxis.enabled: false
            yAxis.enabled: true
        }
        Text {
            anchors.centerIn: parent
            color: "white"
            horizontalAlignment: Text.AlignHCenter
            text: ballY.objectName + "\n"
                + dragHandlerY.centroid.position.x.toFixed(1) + ","
                + dragHandlerY.centroid.position.y.toFixed(1) + "\n"
                + ballY.x.toFixed(1) + "," + ballY.y.toFixed(1)
        }
    }
}
