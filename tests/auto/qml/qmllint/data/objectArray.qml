import QtQml 2.15

QtObject {
    property list<QtObject> objects
    objects: [
        QtObject { property string foo: "bar" },
        QtObject { property string bar: "foo" }
    ]
}
