import QtQuick
import QmlBench

// Testing the performance of anchors against w/h bindings (item_size).
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
            anchors.fill: parent
        }
    }
}
