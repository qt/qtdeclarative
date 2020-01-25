import QtQuick 2.15

Item {
    property color myColor: "red"
    component StyledRectangle: Rectangle {
        width: 24
        height: 24
        color: myColor
    }
}
