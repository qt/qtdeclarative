import QtQuick
import QmlBench

// Tests the creation of the JS bool type as a property.
CreationBenchmark {
    id: root;
    count: 50
    staticCount: 2500
    delegate: Item {
        property bool value: true
    }
}

