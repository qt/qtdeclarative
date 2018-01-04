import QtQuick 2.0
import QmlBench 1.0
import QtQuick.Controls 2.1

CreationBenchmark {
    id: root
    count: 20
    staticCount: 2000
    delegate: ToolSeparator {
        x: QmlBench.getRandom() * root.width - width
        y: QmlBench.getRandom() * root.height - height
    }
}
