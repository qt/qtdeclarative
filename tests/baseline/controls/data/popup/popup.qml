import QtQuick
import QtQuick.Controls

Item {
    width: 200
    height: 200

    Popup {
        anchors.centerIn: parent
        visible: true
        Text {
            text: "visible"
        }
    }
}
