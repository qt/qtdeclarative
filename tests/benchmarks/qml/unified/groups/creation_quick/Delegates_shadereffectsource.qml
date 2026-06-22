import QtQuick
import QmlBench

// Tests the creation of ShaderEffectSource with a few simple props.
CreationBenchmark {
    id: root;
    count: 50
    staticCount: 500
    delegate: ShaderEffectSource {
        x: QmlBench.getRandom() * (root.width - width)
        y: QmlBench.getRandom() * (root.height - height)
        width: 30
        height: 15
        sourceItem: sourceRectItem
    }

    Rectangle {
        id: sourceRectItem
        color: "#ff0000"
        width: 30
        height: 15
    }
}

