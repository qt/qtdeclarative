import QtQuick
import QmlBench
import QmlBench

// Tests the creation of Items bound to a QML-defined singleton
CreationBenchmark {
    id: root;
    count: 50;
    staticCount: 2500;
    delegate: Item {
        x: Globals.realProp
        y: Globals.intProp
        smooth: Globals.boolProp
    }
}
