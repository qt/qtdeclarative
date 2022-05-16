import QtQml 2.15

Timer {
    Component.onCompleted: {
        var a = 97
        var b = 98
        var c = 99
        let d = 100
        const e = 101
        console.log("onClicked") // Set breakpoint
        running = true
    }

    interval: 0
    onTriggered: Qt.quit()
}
