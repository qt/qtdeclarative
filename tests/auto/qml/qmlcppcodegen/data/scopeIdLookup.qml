pragma ComponentBehavior: Bound

import QtQml

QtObject {
    id: root

    property QtObject b: QtObject {
        id: bar
        objectName: "outer"
    }

    property Instantiator i: Instantiator {
        model: 1
        delegate: QtObject {
            property QtObject bar: QtObject { objectName: "inner" }
            Component.onCompleted: root.objectName = bar.objectName
        }
    }
}
