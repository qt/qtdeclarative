import QtQuick
import QmlBench

// Tests the creation of the JS quaternion type as a property.
CreationBenchmark {
    id: root;
    count: 50
    staticCount: 2500
    delegate: Item {
        property quaternion value: Qt.quaternion(2, 17, 0.9, 0.6)
    }
}

