import QtQuick
import QmlBench
import QtQuick.Controls

// Tests the creation of QQC2's RadioButton type.
CreationBenchmark {
    id: root
    count: 20
    staticCount: 1000
    delegate: RadioButton {
        x: QmlBench.getRandom() * root.width - width
        y: QmlBench.getRandom() * root.height - height
        text: "CheckBox"
        autoExclusive: false
        checked: index % 2
    }
}
