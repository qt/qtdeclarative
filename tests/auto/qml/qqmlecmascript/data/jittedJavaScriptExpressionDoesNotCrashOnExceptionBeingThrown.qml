import QtQml
import QtQuick

Window {
    id: root

    Item {
        id: child

        Timer {
            id: timer

            property int fuel: 10

            interval: 1
            repeat: true
            running: true
            onTriggered: {
                if (--fuel == 0)
                    running = false;
                parent.state = parent.state === "inactive" ? "active" : "inactive";
            }
        }

        states: [
            State {
                name: "active"
                StateChangeScript {
                    script: root.active = true;
                }
            },
            State {
                name: "inactive"
                StateChangeScript {
                    script: root.active = false;
                }
            }
        ]
    }
}
