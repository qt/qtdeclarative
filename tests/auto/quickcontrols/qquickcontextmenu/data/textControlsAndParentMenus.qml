import QtQuick
import QtQuick.Controls

ApplicationWindow {
    id: window
    width: 800
    height: 800

    Column {
        x: 50
        y: 50

        TextArea {
            id: textArea
            objectName: "textArea"
            text: qsTr("Some, well, text here (surprise!)")
            // Don't let it go past the horizontal center so that we can test
            // right-clicking the center of the window.
            width: 200
            // Ensure that they don't consume too much vertical space so that
            // the context menu doesn't get repositioned.
            height: 30
        }

        TextField {
            objectName: "textField"
            text: qsTr("A not-so-vast partially-open field")
            width: 200
            height: 30
        }
    }

    contentItem.ContextMenu.menu: Menu {
        id: windowMenu
        objectName: "windowMenu"

        MenuItem {
            text: qsTr("Open window")
        }
        MenuItem {
            text: qsTr("Wash window")
        }
        MenuItem {
            text: qsTr("Admire the view")
        }
    }
}
