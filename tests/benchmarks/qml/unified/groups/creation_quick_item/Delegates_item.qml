import QtQuick
import QmlBench

// Tests the creation of Item with a few simple props.
// Compare with delegates_item_empty, and delegates_item_empty_jscreation.
// Compare with delegates_item_script.
CreationBenchmark {
    id: root;
    count: 50
    staticCount: 5000
    delegate: Item {
        x: QmlBench.getRandom() * (root.width - width)
        y: QmlBench.getRandom() * (root.height - height)
        width: 30
        height: 15
    }
}
