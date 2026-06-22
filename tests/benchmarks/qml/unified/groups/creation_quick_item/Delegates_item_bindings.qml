import QtQuick
import QmlBench

// Compare with delegates_item_states
CreationBenchmark {
    id: root;
    count: 50
    staticCount: 5000
    delegate: Item {
        id: itemInstance
        x: QmlBench.getRandom() * (root.width - width)
        y: QmlBench.getRandom() * (root.height - height)
        width: 30
        height: 15
        visible: 1 == 1 ? true : false
    }
}

