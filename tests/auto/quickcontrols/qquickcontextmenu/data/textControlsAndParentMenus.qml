import QtQuick
import QtQuick.Controls

ApplicationWindow {
    id: window
    width: 600
    height: 400

    TextArea {
        id: textArea
        objectName: "textArea"
        text: qsTr("Some, well, text here (surprise!)")
        width: parent.width
        height: parent.height / 3
    }

    TextField {
        objectName: "textField"
        text: qsTr("A not-so-vast partially-open field")
        width: parent.width
        y: parent.height * 2 / 3
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
