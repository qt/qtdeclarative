import Test 1.0
MyContainer {
    property variant object : myObjectId

    MyTypeObject {
        id: "myObjectId"
    }

    MyTypeObject {
        selfGroupedProperty.id: "name.with.dots"
    }
}
