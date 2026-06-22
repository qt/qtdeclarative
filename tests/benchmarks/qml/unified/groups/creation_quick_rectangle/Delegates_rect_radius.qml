import QtQuick
import QmlBench

// Tests the creation of rectangles with rounded corners.
// Compare with delegates_rect_blended & delegates_rect
CreationBenchmark {
    id: root;
    count: 50;
    staticCount: 2500;
    delegate: Rectangle {
        x: QmlBench.getRandom() * (root.width - width)
        y: QmlBench.getRandom() * (root.height - height)
        width: 30
        height: 15
        color: "steelblue"
        radius: 10
    }
}

