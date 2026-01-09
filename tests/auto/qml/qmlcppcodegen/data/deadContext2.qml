pragma ComponentBehavior: Bound
import QtQuick

Item {
    id: root

    property Component a: Item {
        Timer {
            id: timer1
            interval: 1
            running: root.choice < 4

            function doit() {}
            onTriggered: {
                ++root.choice
                timer1.doit()
            }
        }
    }

    property Component b: Item {
        Timer {
            id: timer2
            interval: 1
            running: root.choice < 4

            function doit() {}
            onTriggered: {
                ++root.choice
                timer2.doit()
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
