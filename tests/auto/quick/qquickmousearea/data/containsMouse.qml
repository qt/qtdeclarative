import QtQuick

Rectangle {
    width: 200
    height: 200
    visible: true
    MouseArea {
        id: mouseArea
        objectName: "mouseArea"
        anchors.fill: parent
        hoverEnabled: true
        visible: false
    }
}
