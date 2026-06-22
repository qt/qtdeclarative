import QtQuick
import QmlBench
import QtQuick.Controls

// Tests the creation of QQC2's Dial type.
CreationBenchmark {
    id: root
    count: 20
    staticCount: 500
    delegate: Dial {
        x: QmlBench.getRandom() * root.width - width
        y: QmlBench.getRandom() * root.height - height
        value: index / root.staticCount
    }
}
