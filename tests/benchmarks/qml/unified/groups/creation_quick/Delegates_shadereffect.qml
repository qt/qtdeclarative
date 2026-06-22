import QtQuick
import QmlBench

// Tests the creation of ShaderEffect
CreationBenchmark {
    id: root;
    count: 50;
    staticCount: 2500;
    delegate: ShaderEffect {
        x: QmlBench.getRandom() * (root.width - width)
        y: QmlBench.getRandom() * (root.height - height)
        width: 30
        height: 15
        fragmentShader: "red.frag.qsb"
    }
}


