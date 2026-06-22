import QtQuick
import QmlBench

// Tests the creation of the JS rect type as a property.
CreationBenchmark {
    id: root;
    count: 50
    staticCount: 2500
    delegate: Item {
        property rect value: Qt.rect(0, 0, 10, 10)
    }
}

