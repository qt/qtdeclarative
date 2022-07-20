import QtQuick 2.15

Item {
    id: root
    property var prop: null
    property bool works: false
    states: [
        State {
            name: "mystate"
            when: root.prop
            PropertyChanges {
                target: root
                works: "works"
            }
        }
    ]
    Component.onCompleted: root.prop = new Object
}
