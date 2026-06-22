import QtQuick
import QmlBench

// Tests the creation of Text with strikeout decoration
CreationBenchmark {
    id: root;
    count: 50;
    staticCount: 1000;
    delegate: Text {
        x: QmlBench.getRandom() * (root.width - width)
        y: QmlBench.getRandom() * (root.height - height)
        text: "Qt Quick!"
        font.pixelSize: 10
        font.strikeout: true
        textFormat: Text.PlainText
    }
}

