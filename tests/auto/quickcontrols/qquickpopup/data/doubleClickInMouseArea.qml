import QtQuick
import QtQuick.Controls
import QtQuick.Window

Rectangle {
    width: 200; height: 200
    color: mouseArea.pressed ? "red" : "orange"

    Popup {
        visible: true
        closePolicy: Popup.NoAutoClose
        width: 100
        height: 100
        contentItem: MouseArea {
            id: mouseArea

            anchors.fill: parent
        }
        background: Rectangle {
            color: "green"
        }
    }
}
