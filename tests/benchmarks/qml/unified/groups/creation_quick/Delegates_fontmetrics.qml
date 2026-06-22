import QtQuick
import QmlBench

// Tests the creation of FontMetrics with some simple props
CreationBenchmark {
    id: root;
    count: 50;
    staticCount: 2500;

    property string fontName: {
        if (Qt.platform.os == "osx") {
            return "Helvetica"
        } else if (Qt.platform.os == "linux") {
            return "Open Sans"
        } else if (Qt.platform.os == "windows") {
            return "Arial"
        }
    }

    delegate: Item {
        FontMetrics {
            font.family: root.fontName
        }
    }
}


