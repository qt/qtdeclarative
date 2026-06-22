import QtQuick
import QmlBench

// Tests the creation of rectangles with an alpha channel.
// Compare with delegates_rect & delegates_rect_radius
CreationBenchmark {
    id: root;
    count: 50
    staticCount: 2500
    delegate: Rectangle {
        x: QmlBench.getRandom() * (root.width - width)
        y: QmlBench.getRandom() * (root.height - height)
        width: 30
        height: 15
        color: Qt.hsla(0.7, 0.3, 0.6, 0.5);
    }
}

