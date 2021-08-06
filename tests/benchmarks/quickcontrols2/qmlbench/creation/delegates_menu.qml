import QtQuick
import QmlBench
import QtQuick.Controls

CreationBenchmark {
    id: root
    count: 20
    staticCount: 200
    delegate: Item {
        x: QmlBench.getRandom() * root.width - width
        y: QmlBench.getRandom() * root.height - height
        width: menu.width
        height: menu.height
        Menu {
            id: menu
            visible: true
            MenuItem { text: "MenuItem1" }
            MenuItem { text: "MenuItem2" }
            MenuItem { text: "MenuItem3" }
        }
    }
}
