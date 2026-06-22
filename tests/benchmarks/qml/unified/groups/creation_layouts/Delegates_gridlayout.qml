import QtQuick
import QmlBench
import QtQuick.Layouts

// Tests the creation of GridLayout, to be compared with Grid
CreationBenchmark {
    id: root;
    count: 20;
    staticCount: 1000;
    delegate: GridLayout {
        x: QmlBench.getRandom() * (root.width - width)
        y: QmlBench.getRandom() * (root.height - height)
        columns: 2
        Rectangle {
            width: 50
            height: 10
            color: "red"
        }
        Rectangle {
            width: 50
            height: 10
            color: "red"
        }
        Rectangle {
            width: 50
            height: 10
            color: "red"
        }
        Rectangle {
            width: 50
            height: 10
            color: "red"
        }
        Rectangle {
            width: 50
            height: 10
            color: "red"
        }
    }
}
