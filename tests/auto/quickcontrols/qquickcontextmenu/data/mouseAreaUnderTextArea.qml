import QtQuick
import QtQuick.Controls

ApplicationWindow {
    id: window
    width: 600
    height: 400

    MouseArea {
        width: 50
        height: 50
        acceptedButtons: Qt.LeftButton

        onPressed: (mouse) => window.color = "red"

        TextArea {
            anchors.fill: parent
            text: "Hello, World!"
            color: "black"
        }
    }
}
