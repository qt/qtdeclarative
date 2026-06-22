import QtQuick
import QmlBench

// Tests the creation of the JS real type as a property.
CreationBenchmark {
    id: root;
    count: 50
    staticCount: 2500
    delegate: Item {
        property real value: 42.0
    }
}

