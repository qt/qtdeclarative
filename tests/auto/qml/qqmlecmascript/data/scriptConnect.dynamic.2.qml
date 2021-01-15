import Qt.test 1.0
import QtQuick 2.0
import "scriptConnect.6.js" as Script

Item {
    id: root

    property int test: 0
    property var dynamicObjectProperty: undefined

    signal outer
    Component {
        id: comp
        Item {
            signal inner
            onInner: { Script.testFunction(); root.disconnectAndDestroyDynamicObject(); }
        }
    }

    function disconnectAndDestroyDynamicObject() {
        if (dynamicObjectProperty) {
            root.outer.disconnect(dynamicObjectProperty.inner);
            dynamicObjectProperty.destroy();
            dynamicObjectProperty = undefined;
            gc();
        }
    }

    Component.onCompleted: {
        dynamicObjectProperty = comp.createObject(root);
        root.outer.connect(dynamicObjectProperty.inner);
    }
}
