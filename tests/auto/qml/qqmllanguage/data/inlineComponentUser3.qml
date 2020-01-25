import QtQuick 2.14

Item {
    width: 600
    height: 480
    component StyledRectangle: Rectangle {
        width: 24
        height: 24
        color: "blue"
    }
    StyledRectangle {
        objectName: "icInstance"
        anchors.centerIn: parent
    }

}
