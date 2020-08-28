import QtQuick
import QmlBench
import QtQuick.Controls

CreationBenchmark {
    id: root
    count: 20
    staticCount: 500
    delegate: Item {
        x: QmlBench.getRandom() * root.width - width
        y: QmlBench.getRandom() * root.height - height
        width: tooltip.width
        height: tooltip.height
        ToolTip {
            id: tooltip
            visible: true
            text: "ToolTip"
        }
    }
}
