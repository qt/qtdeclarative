import QtQuick
import QmlBench
import QtQuick.Controls

CreationBenchmark {
    id: root
    count: 20
    staticCount: 250
    delegate: Item {
        width: root.width
        height: root.height
        Drawer {
            id: drawer
            visible: true
            position: 1.0
            edge: index % 2 ? Qt.LeftEdge : Qt.RightEdge
            width: root.width / 3
            height: parent.height
        }
    }
}
