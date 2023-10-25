import QtQuick

Rectangle {
    id: rect
    color: "green"

    Timer {
        id: exitTimer
        running: false
        onTriggered: Qt.quit()
    }

    Timer {
        id: resizeTimer
        running: false
        onTriggered: {
            rect.width = 100
            rect.height = 50
            exitTimer.start()
        }
    }

    Window.onHeightChanged: {
        if (rect.Window.width > 0)
            console.info("window", rect.Window.width, rect.Window.height, "content", rect.width, rect.height)
        resizeTimer.start()
    }
}
