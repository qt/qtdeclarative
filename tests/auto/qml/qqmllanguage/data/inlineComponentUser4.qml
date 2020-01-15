import QtQuick 2.14

Item {
    width: 600
    height: 480
    property color myColor: "blue"
    InlineComponentProvider2.StyledRectangle {
        objectName: "icInstance"
        anchors.centerIn: parent
    }

}
