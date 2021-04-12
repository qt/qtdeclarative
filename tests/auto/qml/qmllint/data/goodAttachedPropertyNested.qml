import AttachedProperties 1.0
QtObject {
    id: root
    TestType.object: Component {}
    TestType.count: 42 + 2

    Component.onCompleted: {
        TestType.count = 42;
        console.log(TestType.object.progress);
    }

    QtObject {
        TestType.count: 43
        Component.onCompleted: {
            console.log(TestType.count);
            console.log(root.TestType.count);
            console.log(root.TestType.object.progress);
        }
    }
}
