import QtQuick
import QtQuick.Window
import QtQuick.Controls

Window {
    width: 400
    height: 400
    Popup {
        objectName: "popup"
        visible: true
        width: 200
        height: 200
        anchors.centerIn: parent
        focus: true
        closePolicy: Popup.CloseOnEscape
    }
}
