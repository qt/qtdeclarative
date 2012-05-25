import QtQuick 2.0

Rectangle {
    width: 240
    height: 320

    GridView {
        id: grid

        property real initialZ: 342

        anchors.fill: parent
        objectName: "grid"
        model: ListModel {}

        delegate: Text {
            objectName: "wrapper"
            font.pointSize: 20
            text: index
        }

        header: Rectangle {
            width: 240
            height: 30
            z: grid.initialZ
        }

        footer: Rectangle {
            width: 240
            height: 30
            z: grid.initialZ
        }
    }
}

