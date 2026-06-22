import QtQuick
import QmlBench

// Compare with delegates_item_bindings
CreationBenchmark {
    id: root;
    count: 50
    staticCount: 5000
    delegate: Item {
        id: itemInstance
        x: QmlBench.getRandom() * (root.width - width)
        y: QmlBench.getRandom() * (root.height - height)
        width: 30
        height: 15
        states: [
            State {
                name: "visible"
                when: 1 == 1
                PropertyChanges { target: itemInstance; visible: true }
            },
            State {
                name: "invisible"
                when: 0 == 1
                PropertyChanges { target: itemInstance; visible: false }
            }
        ]
    }
}

