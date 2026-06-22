import QtQuick
import QmlBench

// Test allocation of Item, with no Repeater or anything.
// Compare with delegates_qobject & delegates_item_empty.
Benchmark {
    id: root;
    count: 50;
    staticCount: 5000;

    property var items;
    onTChanged: {
        allocate();
    }

    Component {
        id: component;
        Item {
        }
    }

    function allocate() {
        if (items && items.length) {
            for (var i=0; i<items.length; ++i)
                items[i].destroy();
        }
        items = [];

        for (var i=0; i<root.count; ++i) {
            var object = component.createObject(root);
            items.push(object);
        }
    }
}
