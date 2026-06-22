import QtQuick
import QmlBench

// Testing the performance of changing 1 Text in a scene vs all (changing_texts)
Item {
    id: root;
    property int count: 100
    property int staticCount: 1000;

    property real t;
    NumberAnimation on t { from: 0; to: 1000; duration: 1000; loops: Animation.Infinite }

    Repeater {
        id: repeater
        model: root.count
        Text {
            x: QmlBench.getRandom() * root.width
            y: QmlBench.getRandom() * root.height
            text: index == 0 ? Math.round(t) : index;
        }
    }
}
