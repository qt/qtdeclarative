import QtQuick 2.12

Flickable {
    id: flick
    width: 320
    height: 320
    contentWidth: 500
    contentHeight: 500
    Text {
        anchors.centerIn: parent
        font.pixelSize: 50
        text: "🐉"
        color: flick.dragging ? "orange" : "grey"
    }
}
