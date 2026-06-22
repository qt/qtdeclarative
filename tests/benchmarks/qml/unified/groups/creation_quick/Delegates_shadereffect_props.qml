import QtQuick
import QmlBench

// Tests the creation of ShaderEffect with custom properties,
// as these add additional cost
CreationBenchmark {
    id: root;
    count: 50;
    staticCount: 2500;

    Image {
        id: sourceImage
        source: "qrc:///shared/butterfly-wide.png"
    }

    delegate: ShaderEffect {
        x: QmlBench.getRandom() * (root.width - width)
        y: QmlBench.getRandom() * (root.height - height)
        width: sourceImage.width
        height: sourceImage.height
        supportsAtlasTextures: true
        property var source: sourceImage
    }
}


