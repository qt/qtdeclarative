import QtQuick
import QtQuick.Controls

Item {
    width: 640
    height: 480
    visible: true

    property alias outerButton: outerButton
    property alias innerButton: innerButton

    Button {
        id: outerButton
        x: 5
        y: 5
        width: 200
        height: 200
        hoverEnabled: true
        text: hovered ? "hovered" : ""

        Button {
            id: innerButton
            width: parent.width - 20
            height: parent.height - 20
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            text: hovered ? "hovered" : ""
            opacity: 0.5
        }
    }
}
