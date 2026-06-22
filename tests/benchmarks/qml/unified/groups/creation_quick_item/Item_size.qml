import QtQuick
import QmlBench

// Testing the performance of w/h bindings against anchors (item_anchors).
CreationBenchmark {
    id: root;
    count: 50;
    staticCount: 2500;
    delegate: Item {
        x: QmlBench.getRandom() * (root.width - width)
        y: QmlBench.getRandom() * (root.height - height)
        width: 30
        height: 15

        Item {
            width: parent.width
            height: parent.height
        }
    }
}

