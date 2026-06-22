import QtQuick
import QmlBench

// Test allocation of QObject, with no Repeater or anything, stored in a JS
// array. The array is interesting as it's a large GC'd allocation.
// Compare with delegates_qobject.
Benchmark {
    id: root;
    count: 50;
    staticCount: 10000;

    property var items;
    onTChanged: {
        allocate();
    }

    Component {
        id: component;
        QtObject {
        }
    }

    function allocate() {
        if (items && items.length) {
            for (var i=0; i<items.length; ++i)
                items[i].destroy();
        }
        items = [];

        for (var i=0; i<root.count; ++i) {
            var object = component.createObject();
            items.push(object);
        }
    }
}

