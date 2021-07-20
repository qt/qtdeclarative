import QtQuick
import QmlBench
import QtQuick.Controls

CreationBenchmark {
    id: root
    count: 20
    staticCount: 2000
    delegate: ToolSeparator {
        x: QmlBench.getRandom() * root.width - width
        y: QmlBench.getRandom() * root.height - height
    }
}
