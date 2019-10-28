import QtQuick 2.13
import QtQuick.Window 2.13
import QtQuick.Controls 2.13

Window {
    width: 400
    height: 400
    Popup {
        objectName: "popup"
        visible: true
        width: 200
        height: 200
        anchors.centerIn: parent
        closePolicy: Popup.CloseOnEscape
    }
}
