import QtQuick

Item {
    id: root
    property Cycle2 peer

    Rectangle {
        id: box
        width: 16
        height: 16
        anchors.right: parent.right
        anchors.rightMargin: 1
    }

    Rectangle {
        width: 4
        height: 4
        anchors.left: parent.left
        anchors.leftMargin: root.peer.item.anchors.rightMargin + 1
    }

    property alias item: box
}
