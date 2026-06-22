import QtQuick
import QmlBench

// Tests the creation of Text with a long string of text that is elided on the right
CreationBenchmark {
    id: root
    count: 50
    staticCount: 1000
    delegate: Text {
        x: QmlBench.getRandom() * (root.width - width)
        y: QmlBench.getRandom() * (root.height - height)
        width: 40
        text: "Some very very very long text"
        font.pixelSize: 10
        elide: Text.ElideRight
    }
}
