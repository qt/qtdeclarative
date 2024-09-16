import QtQuick 2.12

ListView {
    id: root
    objectName: "view"
    width: 600
    height: 600
    model: 3
    snapMode: ListView.SnapOneItem
    boundsBehavior: Flickable.StopAtBounds
    highlightRangeMode: ListView.StrictlyEnforceRange
    preferredHighlightBegin: 0
    preferredHighlightEnd: 0
    highlightMoveDuration: 100
    delegate: Rectangle {
        id: delegateRect
        width: 500
        height: 500
        color: Qt.rgba(Math.random(), Math.random(), Math.random(), 1)
        border.width: 4
        border.color: ma.pressed ? "red" : "white"
        Text {
            text: index
            font.pixelSize: 128
            anchors.centerIn: parent
        }
        MouseArea {
            id: ma
            anchors.fill: parent
        }
    }
}
