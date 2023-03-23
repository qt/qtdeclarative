import QtQuick 2.15

Item {
    id: root
    property int number: 2
    property int stateChangeCounter: 0

    Item {
        id: item
        onStateChanged: ++stateChangeCounter
        states: [
            State {
                name: "n1"
                when: root.number === 1
            },
            State {
                name: "n2"
                when: root.number === 2
            }
        ]
    }
}
