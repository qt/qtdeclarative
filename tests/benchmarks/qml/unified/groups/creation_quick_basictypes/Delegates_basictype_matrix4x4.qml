import QtQuick
import QmlBench

// Tests the creation of the JS matrix4x4 type as a property.
CreationBenchmark {
    id: root;
    count: 50
    staticCount: 2500
    delegate: Item {
        property matrix4x4 value: Qt.matrix4x4(1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16);
    }
}

