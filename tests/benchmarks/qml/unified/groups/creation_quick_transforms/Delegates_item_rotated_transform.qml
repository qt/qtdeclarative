import QtQuick
import QmlBench

// Rotate an Item.
// Compare with delegates_item_rotated_transform.
CreationBenchmark {
    id: root;
    count: 50;
    staticCount: 2500;
    delegate: Item {
        x: QmlBench.getRandom() * (root.width - width)
        y: QmlBench.getRandom() * (root.height - height)
        width: 30
        height: 15
        transform: Rotation {
            angle: 45
        }
    }
}


