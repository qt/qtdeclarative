import QtQuick
import QmlBench
import QtQuick.Controls

CreationBenchmark {
    id: root
    count: 20
    staticCount: 1000
    delegate: GroupBox {
        x: QmlBench.getRandom() * root.width - width
        y: QmlBench.getRandom() * root.height - height
        title: "GroupBox"
        Item {
            implicitWidth: 100
            implicitHeight: 100
        }
    }
}
