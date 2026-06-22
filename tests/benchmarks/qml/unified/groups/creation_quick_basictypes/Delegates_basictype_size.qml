import QtQuick
import QmlBench

// Tests the creation of the JS size type as a property.
CreationBenchmark {
    id: root;
    count: 50
    staticCount: 2500
    delegate: Item {
        property size value: Qt.size(0, 150)
    }
}

