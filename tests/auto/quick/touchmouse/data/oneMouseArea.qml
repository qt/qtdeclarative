import QtQuick 2.0

Item {
    width: 320; height: 240

    Rectangle {
        width: parent.width / 2; height: parent.height; x: width
        color: ma.containsPress ? "steelblue" : "lightsteelblue"
        MouseArea {
            id: ma
            anchors.fill: parent
        }
    }
}
