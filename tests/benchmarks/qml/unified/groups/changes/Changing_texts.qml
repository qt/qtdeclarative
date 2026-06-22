import QtQuick
import QmlBench

// Testing the performance of changing all texts in a scene vs 1 (text_oneofmany)
Item {
    id: root;
    property int count: 100;
    property int staticCount: 1000;

    property real t;
    NumberAnimation on t { from: 0; to: 1; duration: 2347; loops: Animation.Infinite }

    Repeater {
        id: repeater
        model: root.count
        Text {
            x: QmlBench.getRandom() * root.width
            y: QmlBench.getRandom() * root.height
            text: Math.floor( root.t * 1000 ) / 1000;
        }
    }
}
