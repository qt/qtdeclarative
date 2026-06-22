import QtQuick
import QmlBench

// Tests access of bool properties from a C++ model
Benchmark {
    id: root;
    count: 50
    staticCount: 2500

    onTChanged: {
        testModel.rowNumber = 0
        testModel.rowNumber = root.count
    }

    Repeater {
        model: TestModel {
            id: testModel
        }
        delegate: Item {
            property bool value: model.boolRole
        }
    }
}

