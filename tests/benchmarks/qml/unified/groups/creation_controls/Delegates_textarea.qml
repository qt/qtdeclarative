import QtQuick
import QmlBench
import QtQuick.Controls

// Tests the creation of QQC2's TextArea type.
CreationBenchmark {
    id: root
    count: 20
    staticCount: 500
    delegate: TextArea {
        x: QmlBench.getRandom() * root.width - width
        y: QmlBench.getRandom() * root.height - height
        text: "Text\nArea"
    }
}
