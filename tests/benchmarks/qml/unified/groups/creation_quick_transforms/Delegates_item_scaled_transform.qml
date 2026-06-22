import QtQuick
import QmlBench

// Scale an Item.
// Compare with delegates_item_scaled.
CreationBenchmark {
    id: root;
    count: 50;
    staticCount: 2500;
    delegate: Item {
        x: QmlBench.getRandom() * (root.width - width)
        y: QmlBench.getRandom() * (root.height - height)
        width: 30
        height: 15
        transform: Scale {
            xScale: 10
            yScale: 10
        }
    }
}



