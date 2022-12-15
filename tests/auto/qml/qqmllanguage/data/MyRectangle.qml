import QtQuick

Item {
    property alias rectangle1AnchorsleftMargin: rectangle1.anchors.leftMargin

    Rectangle {
        id: rectangle1
        anchors.leftMargin: 250
    }
}
