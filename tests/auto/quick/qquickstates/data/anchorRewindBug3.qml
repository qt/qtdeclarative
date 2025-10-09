import QtQuick 2.0

Rectangle {
    id: root
    width: 100; height: 100

    Rectangle {
        id: rectangle
        objectName: "inner"
        color: "green"
        // Width and height end up to be 50
        // after root Component.onCompleted
        width: 75
        height: 75
        anchors.top: root.top
        anchors.left: root.left
    }

    // Start with anchored state
    state: "anchored"
    states: [
        State {
            name: "anchored"
            AnchorChanges {
                target: rectangle
                anchors.top: undefined
                anchors.left: undefined
                anchors.right: root.right
                anchors.bottom: root.bottom
            }
        }
    ]

    Component.onCompleted: {
        rectangle.width = 50
        rectangle.height = 50
    }
}
