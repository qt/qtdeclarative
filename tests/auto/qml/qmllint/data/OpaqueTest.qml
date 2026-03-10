import QtQuick
import opaque

Window {
    width: 640
    height: 480
    visible: true
    title: qsTr("Hello World")
    Container {id: container }
    Container {
        fullyOpaque: container.fullyOpaque
    }

    Text {
        anchors.centerIn: parent
        text: `Contained.x: ${container.contained.x}`
    }
}
