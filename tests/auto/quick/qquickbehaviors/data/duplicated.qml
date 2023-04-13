import QtQuick 2.0
Rectangle {
    id: main

    width: 400
    height: 400

    property bool behavior1Triggered
    property bool behavior2Triggered

    Behavior on x {
        ScriptAction { script: behavior1Triggered = true }
    }
    Behavior on x {
        ScriptAction { script: behavior2Triggered = true }
    }

    MouseArea {
        id: clicker
        anchors.fill: parent
    }

    states: State {
        name: "moved"
        when: clicker.pressed
        PropertyChanges {
            target: main
            x: 200
        }
    }
}
