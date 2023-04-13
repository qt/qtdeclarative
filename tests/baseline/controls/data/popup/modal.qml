import QtQuick
import QtQuick.Controls

Item {
    width: 200
    height: 200

    Popup {
        anchors.centerIn: parent
        modal: true
        visible: true
        Label {
            text: "visible"
        }
    }
}
