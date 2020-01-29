import QtQuick 2.14

Item {
    width: 600
    height: 480
    property InlineComponentProvider.StyledRectangle myProp: InlineComponentProvider.StyledRectangle {}
    InlineComponentProvider.StyledRectangle {
        objectName: "icInstance"
        anchors.centerIn: parent
        color: "blue"
    }

}
