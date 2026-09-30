import QtQml

QtObject {
    id: root

    property list<QtObject> items
    property int count: -1
    property QtObject first: null

    Component.onCompleted: {
        root.items = [ MySingleton ];
        root.count = root.items.length;
        root.first = root.items.length > 0 ? root.items[0] : null;
    }
}
