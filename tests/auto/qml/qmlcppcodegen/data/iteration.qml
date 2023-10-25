pragma Strict

import QtQml

QtObject {
    property list<int> ints: [3, 4, 5]
    property list<QtObject> objects: [
        QtObject { objectName: "a" },
        QtObject { objectName: "b" },
        QtObject { objectName: "c" }
    ]

    Component.onCompleted: {
        for (var a in objects) {
            objectName += objects[a].objectName;
            for (var b in ints)
                objectName += ints[b];
        }
    }
}
