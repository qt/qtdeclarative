import QtQml

QtObject {
    id: outer
    objectName: "outer"

    property Component c: QtObject {
        id: inner1
        objectName: inner2.objectName + "a"
    }

    property QtObject inner1: c.createObject()

    property QtObject inner2: QtObject {
        id: inner2
        objectName: "a"
    }
}
