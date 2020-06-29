import QtQuick 2.0

Rectangle {
    width: 400
    height: 400

    MouseArea {
        id: ma
        objectName: "ma"
        width: 200
        height: 200

        drag.target: ma
        drag.minimumX: 0
        drag.maximumX: 200

        Rectangle {
            objectName: "fill"
            anchors.fill: parent
            color: ma.drag.active ? "tomato" : "wheat"
            border.color: ma.pressed ? "red" : "transparent"
            Text { text: ma.mouseX + ", " + ma.mouseY }
        }
    }
}
