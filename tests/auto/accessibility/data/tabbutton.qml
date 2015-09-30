import QtQuick 2.5
import QtQuick.Window 2.2
import Qt.labs.controls 1.0

Window {
    visible: true

    TabBar {
        id: tabbar
        objectName: "TabBar"
        TabButton {
            id: tabbutton
            objectName: "tabbutton"
            text: "TabButton"
        }
    }
}
