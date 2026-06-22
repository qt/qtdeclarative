import QtQuick
import QmlBench
import QtQuick.Layouts

// Tests the creation of RowLayout, to be compared with Row
CreationBenchmark {
    id: root;
    count: 20;
    staticCount: 1000;
    delegate: RowLayout {
        x: QmlBench.getRandom() * (root.width - width)
        y: QmlBench.getRandom() * (root.height - height)
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
