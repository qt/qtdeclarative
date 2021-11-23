import QtQuick

Item {
    id: root
    signal foo()

    Timer {
        interval: 10
        running: true
        onTriggered: root.foo()
    }
}
