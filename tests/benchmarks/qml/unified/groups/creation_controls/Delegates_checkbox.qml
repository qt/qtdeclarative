import QtQuick
import QmlBench
import QtQuick.Controls

// Tests the creation of QQC2's CheckBox type.
CreationBenchmark {
    id: root
    count: 20
    staticCount: 1000
    delegate: CheckBox {
        x: QmlBench.getRandom() * root.width - width
        y: QmlBench.getRandom() * root.height - height
        text: "CheckBox"
        checked: index % 2
    }
}
