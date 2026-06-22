import QtQuick
import QmlBench

// Tests the creation of Text with a short piece of hindi text
CreationBenchmark {
    id: root;
    count: 50;
    staticCount: 1000;
    delegate: Text {
        x: QmlBench.getRandom() * (root.width - width)
        y: QmlBench.getRandom() * (root.height - height)
        text: "गोपनीयता लाभान्वित"
        font.pixelSize: 10
    }
}


