import QtQuick
import QmlBench

// Tests that a single item changing in one subtree, and many items in another
// tree, do not have bad performance when both subtrees are isolated with
// "clip: true".
Item {
    id: root;
    property int count: 100;
    property int staticCount: 2000

    property real t;
    NumberAnimation on t { from: 0; to: 1; duration: 2347; loops: Animation.Infinite }

    Item {
        anchors.fill: parent
        clip: true
        Repeater {
            id: repeater
            model: root.count

            Rectangle {
                color: Qt.hsla(QmlBench.getRandom(), 0.9, 0.4)
                width: 20
                height: 20
                x: QmlBench.getRandom() * root.width
                y: QmlBench.getRandom() * root.height
                clip: true

                Text {
                    color: "white"
                    text: "X"
                    anchors.centerIn: parent
                    font.pixelSize: 10
                }
            }
        }
    }

    Item {
        anchors.fill: parent
        clip: true
        Text {
            anchors.centerIn: parent
            text: "test: " + root.t;
        }
    }

}
