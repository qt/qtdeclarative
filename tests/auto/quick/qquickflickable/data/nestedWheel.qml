import QtQuick

MouseArea {
    width: 100
    height: 100

    Flickable {
        id: inner
        objectName: "innerFlickable"
        property bool isInverted: false
        anchors.fill: parent
        contentWidth: 200
        contentHeight: 200
        contentX: isInverted ? 100 : 0
        contentY: isInverted ? 100 : 0
        boundsBehavior: Flickable.StopAtBounds
    }
}
