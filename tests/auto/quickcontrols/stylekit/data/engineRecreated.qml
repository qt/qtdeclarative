import QtQuick
import Qt.labs.StyleKit

Item {
    width: 200
    height: 100

    property alias button: button

    StyleKit.style: Style {
        themeName: "Light"
        button {
            background.color: "red"
            hovered.background.color: "green"
            pressed.background.color: "blue"
        }
    }

    Button {
        id: button
        anchors.centerIn: parent
        hoverEnabled: true
        text: "button"
    }
}
