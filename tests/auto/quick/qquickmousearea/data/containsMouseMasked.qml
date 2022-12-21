import QtQuick

Rectangle {
    width: 200
    height: 200
    visible: true
    MouseArea {
        id: mouseArea1
        objectName: "mouseArea1"
        anchors.fill: parent
        hoverEnabled: true
        visible: true
    }

    MouseArea {
        id: mouseArea2
        objectName: "mouseArea2"
        anchors.centerIn: mouseArea1
        width: 50
        height: 50
        hoverEnabled: true
        visible: true
    }
}
