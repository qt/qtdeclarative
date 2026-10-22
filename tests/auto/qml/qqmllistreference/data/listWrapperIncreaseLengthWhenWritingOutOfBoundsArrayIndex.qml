import QtQuick

Item {
    id: root
    property list<QtObject> listWrapper: []
    states: [ State { name: "a" }, State { name: "b" } ]

    property int expectedLength: 14;
    property int actualLength: 0;
    property int actualLengthStates: 0;

    Component { id: stateComp; State {} }

    Component.onCompleted: {
        listWrapper[expectedLength - 1] = this
        actualLength = listWrapper.length

        states[expectedLength - 1] = stateComp.createObject(root, { name: "c" })
        actualLengthStates = states.length
    }
}
