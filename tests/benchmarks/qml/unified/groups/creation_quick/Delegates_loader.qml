import QtQuick
import QmlBench

// Tests the creation of Loader with a few simple props.
CreationBenchmark {
    id: root;
    count: 50
    staticCount: 2500
    delegate: Loader {
        x: QmlBench.getRandom() * (root.width - width)
        y: QmlBench.getRandom() * (root.height - height)
        width: 30
        height: 15
        sourceComponent: anItemComponent
    }

    Component {
        id: anItemComponent
        Item {
        }
    }
}

