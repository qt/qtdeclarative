import QtQuick
import QmlBench
import QtQuick.Controls

// Tests the creation of QQC2's Button type.
CreationBenchmark {
    id: root
    count: 20
    staticCount: 1000
    delegate: Button {
        x: QmlBench.getRandom() * root.width - width
        y: QmlBench.getRandom() * root.height - height
        text: "Button"
        down: index % 2
    }
}
