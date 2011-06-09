import QtQuick 2.0

Rectangle {
    id: dropRectangle

    property string colorKey

    color: colorKey

    width: 100; height: 100

    DragTarget {
        id: dragTarget

        anchors.fill: parent

        keys: [ colorKey ]
        dropItem: dropRectangle
    }

    states: [
        State {
            when: dragTarget.containsDrag
            PropertyChanges {
                target: dropRectangle
                color: "grey"
            }
        }
    ]
}
