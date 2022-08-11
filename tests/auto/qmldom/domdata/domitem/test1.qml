import QtQuick
import QtQuick as QQ

Window {
    visible: true
    width: 640
    height: 480
    title: qsTr("Scroll")

    Rectangle {
        anchors.fill: parent

        ListView {
            width: parent.width
            model: {
                MySingleton.mySignal()
                20
            }
            delegate: ItemDelegate {
                id: root
                text: "Item " + (index + 1)
                width: parent.width
                Rectangle {
                    text: "bla"
                }
                MyComponent {
                    text: root.text
                }
            }
        }
    }
}
