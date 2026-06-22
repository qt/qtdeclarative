import QtQuick
import QmlBench

// Tests the performance of getting the year out of a Date object
CreationBenchmark {
    id: root;
    count: 50;
    staticCount: 1500;
    property date dt: new Date()
    delegate: Item {
        property int year: root.dt.getYear()
    }
}
