import QtQml

QtObject {
    id: window

    property int doneClicks: 0

    property UIToolBar t1: UIToolBar {
        objectName: window.objectName
        onDoneClicked: window.doneClicks++
    }

    property UIToolBar t2: UIToolBar {
        objectName: window.objectName
        onDoneClicked: window.doneClicks++
    }

    property Timer timer: Timer {
        interval: 10
        running: true
        onTriggered: window.objectName = "bar"
    }
}
