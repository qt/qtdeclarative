import QtQuick

Item {
    width: 300
    height: 300

    Flickable {
        anchors.fill: parent
        anchors.topMargin: 100
        contentWidth: 1000
        contentHeight: 1000

        Rectangle {
            objectName: "childItem"
            x: 20
            y: 50
            width: 20
            height: 20
            color: "red"
            TapHandler {
            }
        }
    }
}
