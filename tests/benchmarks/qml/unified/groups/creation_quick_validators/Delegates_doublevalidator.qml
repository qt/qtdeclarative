import QtQuick
import QmlBench

// Tests the creation of DoubleValidator
CreationBenchmark {
    id: root;
    count: 50;
    staticCount: 2500;
    delegate: Item {
        property DoubleValidator validator: DoubleValidator {
            top: 100
            bottom: 0
            decimals: 2
        }
    }
}

