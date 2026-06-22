import QtQuick
import QmlBench
import QtQuick.Controls

// Tests the creation of QQC2's ComboBox type.
CreationBenchmark {
    id: root
    count: 20
    staticCount: 250
    delegate: ComboBox {
        x: QmlBench.getRandom() * root.width - width
        y: QmlBench.getRandom() * root.height - height
        model: 5
        currentIndex: index % count
    }
}
