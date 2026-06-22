import QtQuick
import QmlBench
import QtQuick.Controls

// Tests the creation of QQC2's Tumbler type.
CreationBenchmark {
    id: root
    count: 20
    staticCount: 250
    delegate: Tumbler {
        x: QmlBench.getRandom() * root.width - width
        y: QmlBench.getRandom() * root.height - height
        model: 5
        currentIndex: index % count
    }
}
