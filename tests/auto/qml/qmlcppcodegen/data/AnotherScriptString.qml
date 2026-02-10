import QtQuick
import QtQml

    Item {
        id: container

        transitions: [
            // transition for global
            Transition {
                enabled: !container.animates
                from: ""
                to: "open"
                ScriptAction {
                    script: container.isOpen = true
                }
            }
        ]
        Keys.onPressed: a_event => {}
    }
