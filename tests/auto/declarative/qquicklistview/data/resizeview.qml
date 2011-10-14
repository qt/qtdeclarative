import QtQuick 2.0

Rectangle {
    id: root

    property real initialHeight

    ListView {
        id: list
        objectName: "list"
        width: 240
        height: initialHeight
        model: testModel
        delegate: Rectangle {
            objectName: "wrapper"
            width: 240
            height: 20
            border.width: 1
        }
    }
}

