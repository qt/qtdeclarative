import QtQuick 2.2

Rectangle {
    width: 400
    height: 400

    Column {
        spacing: 5
        TextInput {
            id: first
            objectName: "first"
            width: 100
            Rectangle { anchors.fill: parent; color: parent.activeFocus ? "red" : "blue"; opacity: 0.3 }
            KeyNavigation.backtab: third
            KeyNavigation.tab: second
            KeyNavigation.down: second
        }
        TextInput {
            id: second
            objectName: "second"
            width: 100
            Rectangle { anchors.fill: parent; color: parent.activeFocus ? "red" : "blue"; opacity: 0.3 }
            KeyNavigation.up: first
            KeyNavigation.backtab: first
            KeyNavigation.tab: third
        }
        TextInput {
            objectName: "third"
            id: third
            width: 100
            Rectangle { anchors.fill: parent; color: parent.activeFocus ? "red" : "blue"; opacity: 0.3 }
            KeyNavigation.backtab: second
            KeyNavigation.tab: first
        }
        Component.onCompleted: {
            first.focus = true
        }
    }
}
