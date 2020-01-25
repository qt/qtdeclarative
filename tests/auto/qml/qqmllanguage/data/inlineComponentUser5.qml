import QtQuick 2.15

Item {
    width: 600
    height: 480
    property var test: InlineComponentProvider3.StyledRectangle {
        objectName: "icInstance"
        anchors.centerIn: parent
    }

}
