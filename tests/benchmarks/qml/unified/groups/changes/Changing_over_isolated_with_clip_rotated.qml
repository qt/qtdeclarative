import QtQuick
import QmlBench

// Tests that a single item changing in one subtree, and many items in another
// tree, do not have bad performance when both subtrees are isolated with
// "clip: true".
//
// Different from changing_over_isolated_with_clip, we also rotate, as this
// can't just be clipped with scissoring.
//
// TODO: consider whether it makes sense to test this as a specific render
// test, changing_over_isolated_with_clip covers the clipping part well enough
// that perhaps we don't need this approach. Useful for now, though.
Item {
    id: root;
    property int count: 1000;
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
                rotation: 10

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
