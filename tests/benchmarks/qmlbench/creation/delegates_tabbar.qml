import QtQuick
import QmlBench
import QtQuick.Controls

CreationBenchmark {
    id: root
    count: 20
    staticCount: 250
    delegate: TabBar {
        x: QmlBench.getRandom() * root.width - width
        y: QmlBench.getRandom() * root.height - height
        currentIndex: index / root.staticCount * count
        TabButton {
            text: "Tab1"
        }
        TabButton {
            text: "Tab2"
        }
        TabButton {
            text: "Tab3"
        }
    }
}
