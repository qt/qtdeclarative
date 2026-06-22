import QtQuick
import QmlBench
import QtQuick.Controls

// Tests the creation of QQC2's TextField type.
CreationBenchmark {
    id: root
    count: 20
    staticCount: 1000
    delegate: TextField {
        x: QmlBench.getRandom() * root.width - width
        y: QmlBench.getRandom() * root.height - height
        text: "TextField"
    }
}
