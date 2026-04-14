import QtQuick
import QtQuick.Controls

Item {
    id: root
    width: 800
    height: 600

    Popup {
        visible: true
        popupType: Popup.Item
        Column {
            TextField {
                objectName: "textField1"
            }
            TextField {
                objectName: "textField2"
            }
        }
    }
}
