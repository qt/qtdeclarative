import QtQuick 2.0
import QmlBench 1.0
import QtQuick.Controls 2.0

CreationBenchmark {
    id: root
    count: 20
    staticCount: 1000
    delegate: RangeSlider {
        x: QmlBench.getRandom() * root.width - width
        y: QmlBench.getRandom() * root.height - height
        first.value: (index / root.staticCount) * 0.5
        second.value: 0.5 + (index / root.staticCount) * 0.5
    }
}
