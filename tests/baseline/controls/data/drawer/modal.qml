import QtQuick
import QtQuick.Controls

Item {
    width: 200
    height: 200

    Drawer {
        anchors.centerIn: parent
        visible: true
        modal: true
        Label {
            text: "modal"
        }
    }
}