import QtQuick 2.12
import Qt.test 1.0

Item {
    id: root
    readonly property int someProp: 1234

    property bool state1Active: false
    property bool state2Active: false
    StateGroup {
        states: [
            State {
                id: state1
                name: "state1"
                when: state1Active
                changes: [
                    PropertyChanges {
                        objectName: "propertyChanges1"
                        target: ContainingObj.obj
                        prop: root.someProp
                    }
                ]
            }
    ]}
    StateGroup {
        states: [
            State {
                id: state2
                name: "state2"
                when: state2Active
                changes: [
                    PropertyChanges {
                        objectName: "propertyChanges2"
                        target: ContainingObj.obj
                        prop: 11111
                    }
                ]
            }
        ]
    }

    Component.onCompleted: {
        state1Active = true;
        state2Active = true;

        ContainingObj.reset()
    }
}
