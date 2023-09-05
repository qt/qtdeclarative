import QtQuick

Rectangle {
    id: rect
    color: "blue"
    width: 200; height: 150

    Timer {
        id: exitTimer
        running: false
        onTriggered: Qt.quit()
    }

    Window.onHeightChanged: {
        if (rect.Window.width > 0)
            console.info("window", rect.Window.width, rect.Window.height, "content", rect.width, rect.height)
        exitTimer.restart()
    }
}
