import QtQuick

Window {
    Component {
        id: factory
        Item {}
    }

    property Item keepAliveProperty;

    function createItemWithoutParent() {
        return factory.createObject(/*parent*/ null);
    }
}
