import QtQuick
import QmlBench

// Tests the creation of the JS color type as a property.
CreationBenchmark {
    id: root;
    count: 50
    staticCount: 2500
    delegate: Item {
        property color value: "#ff0000"
    }
}

