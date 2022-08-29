import QtQuick

Item {
    id: root
    property bool toggle: true
    property int state1Width: 500

    states: [
        State {
            when: root.toggle
            PropertyChanges { root.width: root.state1Width }
        },
        State {
            when: !root.toggle
            PropertyChanges { root.width: 300 }
        }
    ]

    transitions: Transition {
        id: transition
        SmoothedAnimation { target: root; property: "width"; velocity: 200 }
    }
}
