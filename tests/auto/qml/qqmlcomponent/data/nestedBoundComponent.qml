pragma ComponentBehavior: Bound

import QtQml

QtObject {
    property Component c: QtObject {
        objectName: "bound"
    }

    property QtObject o: c.createObject()
}
