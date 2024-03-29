import QtQuick

Item {
    width: 400
    height: 400
    objectName: "root Item"

    Loader {
        sourceComponent: Window {
            objectName: "red transient Window"
            width: 100
            height: 100
            visible: true // makes it harder, because it wants to become visible before root has a window
            color: "red"
            title: "red"
            flags: Qt.Dialog
            onVisibilityChanged: (visibility) => console.log("visibility " + visibility)
            onVisibleChanged: console.log("visible " + visible)
        }
    }
}
