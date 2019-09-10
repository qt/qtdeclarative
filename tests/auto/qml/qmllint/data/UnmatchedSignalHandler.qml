import QtQuick 2.12

Item {
    width: 640
    height: 480

    MouseArea {
        anchors.fill: parent
        onClicked: console.log("okok")

        Connections {
            onClicked: console.log(mouse.x)
        }
    }
}
