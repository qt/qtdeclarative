import QtQuick 2.12

Item {
    width: 320
    height: 240

    Rectangle {
        color: hh.hovered ? "orange" : "gray"
        anchors.fill: container
    }

    Item {
        id: container
        anchors.fill: parent
        anchors.margins: 40

        Rectangle {
            width: parent.width
            height: 40
            color: ma.pressed ? "blue" : ma.containsMouse ? "aquamarine" : "beige"

            MouseArea {
                id: ma
                anchors.fill: parent
                hoverEnabled: true
            }
        }

        HoverHandler {
            id: hh
        }
    }
}
