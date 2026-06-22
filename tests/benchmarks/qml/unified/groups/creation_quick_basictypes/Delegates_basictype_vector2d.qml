import QtQuick
import QmlBench

// Tests the creation of the JS vector2d type as a property.
CreationBenchmark {
    id: root;
    count: 50
    staticCount: 2500
    delegate: Item {
        property vector2d value: Qt.vector2d(1, 2)
    }
}

