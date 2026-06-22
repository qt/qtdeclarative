import QtQuick
import QmlBench

// Tests the creation of Item with a few simple props (*not* bindings!)
// Compare with delegates_item.
CreationBenchmark {
    id: root;
    count: 50
    staticCount: 5000
    delegate: Item {
        Component.onCompleted: {
            x = QmlBench.getRandom() * (root.width - width)
            y = QmlBench.getRandom() * (root.height - height)
            width = 30
            height = 15
        }
    }
}

