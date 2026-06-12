import QtQuick

Rectangle {
    id: root
    // QtObject { objectName: parent }

    MouseArea {
        onObjectNameChanged: root.objectName = "bye"
        anchors.left: parent.left
    }
}
