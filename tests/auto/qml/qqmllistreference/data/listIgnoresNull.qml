import QtQuick 2.7
import Test

Item {
    id: root

    TestItem {
        id: testItem
    }

    Component.onCompleted : {
        testItem.data.push(null);
        testItem.data.length = 5;
        testItem.data.unshift(null);

        var state = Qt.createQmlObject( "import QtQuick 2.7; State{ name: 'MyState' }", root, "dynamicState" );
        root.states.length = 5;
        root.states.push(null);
        root.states.unshift(state);
    }
}
