import QtQuick
import QmlBench

// Test allocation of QObject, with no Repeater or anything.
// Compare with delegates_qobject.
Benchmark {
    id: root;
    count: 50;
    staticCount: 10000;

    onTChanged: {
        allocate();
    }

    Component {
        id: component;
        QtObject {
        }
    }

    function allocate() {
        for (var i=0; i<root.count; ++i) {
            var object = component.createObject();
            object.destroy();
        }
    }
}
