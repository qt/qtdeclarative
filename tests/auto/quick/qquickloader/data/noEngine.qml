import QtQuick 2

Item {
    id: root
    property bool a: false
    property int changes: 0
    onAChanged: {
        m.model = 0
        m.model = 1
        ++changes;
    }

    Repeater {
        id: m
        model: 1

        Item {
            Timer {
                onTriggered: {
                    root.a = true
                    l.source = "BlueRect.qml"
                }
                interval: 0
                running: true
            }

            Loader {
                id: l
            }
        }
    }
}
