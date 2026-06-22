import QtQuick
import QmlBench

// Tests the creation involving Item's childrenRect property.
// This is an unusually expensive property.
CreationBenchmark {
    id: root;
    count: 50
    staticCount: 5000
    delegate: Item {
        x: QmlBench.getRandom() * (root.width - width)
        y: QmlBench.getRandom() * (root.height - height)
        width: childrenRect.width
        height: childrenRect.height

        Item {
            width: 30
            height: 15
        }
    }
}

