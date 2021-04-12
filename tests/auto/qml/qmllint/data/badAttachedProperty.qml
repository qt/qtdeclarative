import AttachedProperties 1.0
QtObject {
    TestType.object: Component {}

    Component.onCompleted: {
        console.log(TestType.progress) // wrong, as progress should be a property of TestType.object
    }
}
