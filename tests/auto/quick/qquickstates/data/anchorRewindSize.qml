import QtQuick

Item {
    id: root
    width: 400
    height: 400
    property bool changeState: false
    Item {
        id: outer
        anchors.fill: parent
        Item {
            id: inner
            width: 100
            height: 100
            anchors.left: outer.left
            anchors.top: outer.top
        }
        states: [
            State {
                when: root.changeState
                AnchorChanges {
                    target: inner
                    anchors.right: outer.right
                    anchors.bottom: outer.bottom
                }
            }
        ]
    }
}

