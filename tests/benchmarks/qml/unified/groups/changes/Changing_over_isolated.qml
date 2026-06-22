import QtQuick
import QmlBench

// This benchmark determines if clipping two subtrees isolates them,
// such that one changing subtree won't affect performance because
// of the other.
CreationBenchmark {
    id: root;
    count: 1000;
    staticCount: 2000

    // Boolean because: isolating via clip either keeps performance good, or
    // performance is awful if we break it.
    isBooleanResult: true;

    // Create a static subtree of items. This tree does not change.
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

                Text {
                    color: "white"
                    text: "X"
                    anchors.centerIn: parent
                    font.pixelSize: 10
                }
            }
        }
    }

    // Create a subtree containing a single item. This tree does change.
    Item {
        anchors.fill: parent
        clip: true
        Text {
            anchors.centerIn: parent
            text: "test: " + root.t;
        }
    }
}
