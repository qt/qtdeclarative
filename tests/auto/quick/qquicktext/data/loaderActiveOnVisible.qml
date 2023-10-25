import QtQuick

Rectangle {
    id: root
    visible: false
    width: 320
    height: 240
    Flickable {
        id: flickable
        anchors {
            fill: root
            margins: 10
        }
        contentHeight: loader.height

        Loader {
            id: loader
            width: flickable.width
            active: root.visible
            source: "long.qml"
        }
    }
    Component.onCompleted: visible = true
}
