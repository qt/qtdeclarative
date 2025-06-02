import QtQuick

Item {
    width: 100
    height: 100

    Flickable {
        objectName: "flickable"
        anchors.fill: parent
        contentWidth: content.width
        contentHeight: content.height
        Rectangle {
            id: content
            height: 300
            width: 300
            color: "red"
        }
    }
}
