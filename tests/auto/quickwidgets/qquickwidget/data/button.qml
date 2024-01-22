import QtQuick
import QtQuick.Controls.Basic

Item {
    width: 100
    height: 100
    visible: true

    property bool wasPressed: false
    property bool wasReleased: false
    property bool wasClicked: false

    Popup {
        closePolicy: Popup.NoAutoClose
        visible: true

        Button {
            objectName: "button"
            text: "TAP ME"
            anchors.fill: parent

            onPressed: wasPressed = true
            onReleased: wasReleased = true
            onClicked: wasClicked = true
        }
    }
}
