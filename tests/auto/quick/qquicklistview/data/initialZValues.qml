import QtQuick 2.0

Rectangle {
    width: 240
    height: 320

    ListView {
        id: list

        property real initialZ: 342

        anchors.fill: parent
        objectName: "list"
        model: ListModel {}

        delegate: Text {
            objectName: "wrapper"
            font.pointSize: 20
            text: index
        }

        header: Rectangle {
            width: 240
            height: 30
            z: list.initialZ
        }

        footer: Rectangle {
            width: 240
            height: 30
            z: list.initialZ
        }
    }
}

