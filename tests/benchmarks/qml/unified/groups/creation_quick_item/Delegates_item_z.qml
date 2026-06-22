import QtQuick
import QmlBench

// Tests the creation of Item with a non-zero z.
// This is an unusually expensive property.
CreationBenchmark {
    id: root;
    count: 50
    staticCount: 5000
    delegate: Item {
        x: QmlBench.getRandom() * (root.width - width)
        y: QmlBench.getRandom() * (root.height - height)
        z: index
        width: 30
        height: 15
    }
}

