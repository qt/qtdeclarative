import QtQuick

ListView {
    objectName: "list"
    anchors.fill: parent
    snapMode: ListView.SnapOneItem
    model: 10
    delegate: Rectangle {
        width: ListView.view.width
        height: ListView.view.height
        color: ListView.isCurrentItem ? "lightsteelblue" : "#e0e0e0"
    }
}
