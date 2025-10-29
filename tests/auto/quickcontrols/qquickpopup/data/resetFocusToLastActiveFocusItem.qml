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

        Button {
           id: button1
           objectName: "button1"
           width: 20
           visible: true
        }

        Button {
           id: button2
           objectName: "button2"
           width: 20
           visible: true
        }

        Popup {
            id: popup1
            objectName: "popup1"

            width: parent.width / 2
            height: parent.height
            modal: true
            closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent
            popupType: Popup.Item
            focus: true

            Button {
                id: popupButton
                width: 20
                visible: true
                focus: true
            }
        }

        Popup {
            id: popup2
            objectName: "popup2"
            width: parent.width / 2
            height: parent.height
            modal: true
            closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent
            popupType: Popup.Item
            focus: true

            TextEdit {
                id: textEdit
                objectName: "textEdit"
                text: "Text Edit"
                visible: true
                focus: true
            }
        }
    }
}

