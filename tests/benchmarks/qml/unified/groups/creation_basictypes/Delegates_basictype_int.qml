import QtQuick
import QmlBench

// Tests the creation of the JS int type as a property.
CreationBenchmark {
    id: root;
    count: 50
    staticCount: 2500
    delegate: Item {
        property int value: 42
    }
}

