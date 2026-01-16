pragma ComponentBehavior: Bound
import QtQuick

Item {
    id: root

    property Component a: QtObject {
        property Connections c: Connections {
            id: ca
            function onObjectNameChanged() {}
        }

        property Timer t: Timer {
            id: timer1
            interval: 1
            running: root.choice < 4
            onTriggered: {
                ++root.choice
                ca.target = timer1
            }
        }
    }

    property Component b: QtObject {
        property Connections c: Connections {
            id: cb
            function onObjectNameChanged() {}
        }

        property Timer t: Timer {
            id: timer2
            interval: 1
            running: root.choice < 4

            onTriggered: {
                ++root.choice
                cb.target = timer2
            }
        }
    }

    property int choice: 0

    Loader {
        sourceComponent: switch (root.choice % 2) {
            case 0: return root.a
            case 1: return root.b
        }
    }
}
