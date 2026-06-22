import QtQuick
import QmlBench
import QtQuick.Controls

// Tests the creation of QQC2's MenuItem type.
CreationBenchmark {
    id: root
    count: 20
    staticCount: 1000
    delegate: MenuItem {
        x: QmlBench.getRandom() * root.width - width
        y: QmlBench.getRandom() * root.height - height
        text: "MenuItem"
        checkable: index % 2
        checked: index % 2
        down: index % 3
    }
}
