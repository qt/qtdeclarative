import QtQml

QtObject {
    id: root

    property alias bar: root.foo.objectName
    property alias foo: fooId

    foo {}

    property QtObject o: QtObject {
        id: fooId
        objectName: "bar"
    }
}
