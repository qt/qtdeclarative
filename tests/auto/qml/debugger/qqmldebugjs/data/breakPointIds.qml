import QtQml 2.15
Timer {
    Component.onCompleted: {
        console.log('0')
        console.log('1')
        console.log('2')
        console.log('3')
        console.log('4')
        console.log('5')
        running = true
    }

    interval: 0
    onTriggered: Qt.quit()
}
