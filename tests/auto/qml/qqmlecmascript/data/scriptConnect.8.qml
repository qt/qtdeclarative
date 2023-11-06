import Qt.test
import QtQuick

Item {
    id: root
    property int count: 0
    signal someSignal

    property Item item: Item {
        id: contextItem
        function test() {
            count++;
        }
    }

    function itemDestroy() {
        contextItem.destroy()
    }

    Component.onCompleted: root.someSignal.connect(contextItem, contextItem.test);
}
