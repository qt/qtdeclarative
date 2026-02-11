import QtQml

QtObject {
    component FooObject : QtObject {}
    property list<FooObject> objects : [
        FooObject {},
        FooObject {},
    ]

    property list<QtObject> objects2 : [
        FooObject {},
        FooObject {},
    ]

    objectName: objects.length + " " + objects2.length
}
