import QtQuick

Window {
    id: window
    width: 640
    height: 480
    visible: true
    property alias testModel: repeater.model

    Repeater {
        id: repeater
        model: 1
        delegate: Item {
            Component.onCompleted: repeater.model = 0
        }
    }
}

