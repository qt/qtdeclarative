import QtQuick
import QmlBench

// Tests the creation of an empty Item.
// Compare with delegates_item_empty_jscreation
CreationBenchmark {
    id: root
    count: 50;
    staticCount: 5000;
    delegate: Item {
    }
}
