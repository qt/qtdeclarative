import QtQuick

Rectangle {
    id: root
    width: 400
    height: 400
    objectName: "root"

    HoverHandler {
        objectName: "rootHoverHandler"
        cursorShape: Qt.OpenHandCursor
    }

    Rectangle {
        id: innerRect
        objectName: "innerRect"
        anchors.fill: parent

        HoverHandler {
            objectName: "innerHoverHandler"
            cursorShape: Qt.PointingHandCursor
        }
    }
}
