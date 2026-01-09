pragma ComponentBehavior: Bound
import QtQuick

Item {
    id: root

    property Component a: Item {
        Timer {
            interval: 1
            running: root.choice < 8

            function doit() {}
            onTriggered: {
                ++root.choice
                doit()
            }
        }
    }

    property Component b: Item {
        Timer {
            id: timer2
            interval: 1
            running: root.choice < 8

            function doit() {}
            onTriggered: {
                ++root.choice
                timer2.doit()
            }
        }
    }

    property int choice: 0

    Loader {
        sourceComponent: switch (root.choice % 4) {
            case 0:
            case 1:
                return root.a
            case 2:
            case 3:
                return root.b
        }
    }
}
