import QtQml

QtObject {
    property QtObject obj: Timer {
        interval: 1
        running: true
        repeat: true
        onTriggered: {
            if (++interval === 10)
                running = false
        }
    }
    property bool running: (obj as Timer).running
    property int interval: (obj as Timer).interval
}
