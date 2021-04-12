import AttachedProperties 1.0
QtObject {
    id: root
    TestType.object: QtObject {
        property int progress: 42
    }

    Component.onCompleted: {
        console.log(TestType.object.progress);
    }
}
