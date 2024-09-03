import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 400
    height: 400

    property alias modalPopup: popup
    property alias comboBox: combobox

    Popup {
        id: popup
        objectName: "Modal Dialog"
        width: 300
        height: 300
        anchors.centerIn: parent
        visible: true
        modal: true
        popupType: Popup.Item

        ComboBox {
            id: combobox
            anchors.centerIn: parent
            width: 120
            model: 30

            popup: Popup {
                objectName: "Combobox Popup"
                y: combobox.height
                width: combobox.width
                height: contentItem.implicitHeight
                popupType: Popup.Item
                contentItem: ListView {
                    objectName: "Combobox ListView"
                    clip: true
                    implicitHeight: 150
                    model: combobox.delegateModel
                    currentIndex: combobox.highlightedIndex
                    ScrollBar.vertical: ScrollBar {
                        objectName: "vbar"
                    }
                }
            }
        }
    }
}
