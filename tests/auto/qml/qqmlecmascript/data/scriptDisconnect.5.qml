import Qt.test
import QtQuick

Item {
    id: root
    property int count: 0
    signal someSignal
    signal disconnectSignal

    property Item item: Item {
        id: contextItem
        function test() {
            count++;
        }
    }

    Component.onCompleted: root.someSignal.connect(contextItem, contextItem.test);
    onDisconnectSignal: { root.someSignal.disconnect(contextItem, contextItem.test); }
}
