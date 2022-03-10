import QtQuick

Item {
    width: 400
    height: 400
    Item {
        id: outer
        anchors.fill: parent
        Item {
            id: inner
            width: parent.width / 2
            height: parent.height / 2
            anchors.left: parent.right
            anchors.top: parent.bottom
        }
        states: [
            State {
                when: true
                AnchorChanges {
                    target: inner
                    anchors.left: outer.left
                    anchors.top: outer.top
                }
            }
        ]
        transitions: Transition {
            AnchorAnimation {}
        }
    }
}

