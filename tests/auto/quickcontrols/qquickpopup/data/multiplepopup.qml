import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Window {
    id: window
    width: 200
    height: 200
    visible: false

    Item {
        id: rootItem
        objectName: "rootItem"

        width: parent.width
        height: parent.height
        visible: true
        focus: true

        Popup {
            id: popup1
            objectName: "popup1"

            width: parent.width / 2
            height: parent.height
            focus: true
            modal: true
            closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent

            Button {
                id: buttonPopup1
                objectName: "button"

                text: "ButtonPopup"
                width: 20
                visible: true
            }
        }

        Popup {
            id: popup2
            objectName: "popup2"

            width: parent.width / 2
            height: parent.height
            focus: true
            modal: true
            closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent

            TextEdit {
                id: texteditPopup2
                objectName: "textedit"

                focus: true
                text: "Text Edit Content"
                visible: true
            }
        }

        Popup {
            id: popup3
            objectName: "popup3"

            width: parent.width / 2
            height: parent.height
            focus: true
            modal: true
            closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent

            Drawer {
                id: drawerPopup3
                objectName: "drawer"

                width: parent.width / 2
                height: parent.height
                focus: true
                visible: true
            }
        }
    }
}

