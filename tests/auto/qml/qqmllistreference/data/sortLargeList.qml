import QtQml

QtObject {
    id: root

    property Component rec: QtObject { property int k }

    property list<QtObject> items: {
        const tmp = [];
        for (let i = 0; i < 30000; ++i)
            tmp.push(rec.createObject(root, { k: i % 5 }));
        return tmp
    }

    function sort() {
        root.items.sort((a, b) => a.k - b.k)
    }
}
