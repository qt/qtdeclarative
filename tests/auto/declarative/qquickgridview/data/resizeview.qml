import QtQuick 2.0

Rectangle {
    id: root

    property real initialHeight

    GridView {
        id: grid
        objectName: "grid"
        width: 240
        height: initialHeight
        cellWidth: 80
        cellHeight: 60
        model: testModel
        delegate: Rectangle {
            objectName: "wrapper"
            width: 80
            height: 60
            border.width: 1
        }
    }
}

