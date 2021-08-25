import Qt.test

MyImmediateObject {
    id: root
    objectName: "immediate"
    value: 10
    objectProperty: MyQmlObject {
        value: root.value
    }
    objectProperty2: ObjectWithId {}
}
