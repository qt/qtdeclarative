import QtQuick
import QmlBench
import QtQuick.Controls

CreationBenchmark {
    id: root
    count: 20
    staticCount: 1000
    delegate: SwitchDelegate {
        x: QmlBench.getRandom() * root.width - width
        y: QmlBench.getRandom() * root.height - height
        text: "SwitchDelegate"
        checked: index % 2
        down: index % 3
    }
}
