import QtQuick

Rectangle {
    width: 320
    height: 320

    TapHandler {
        onTapped: color = "tomato"
    }

    Flickable {
        anchors.fill: parent
        contentWidth: content.width
        contentHeight: content.height
        Rectangle {
            id: content
            objectName: "pinchable"
            width: 150
            height: 150
            color: "wheat"
            PinchHandler {}
        }
    }
}
