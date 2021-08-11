import AttachedProperties 1.0
QtObject {
    id: root
    TestType.object: QtObject {
        property int progress: 42
    }

    Component.onCompleted: {
        // NB: this currently fails as TestType.object is recognized as QtObject
        // *exactly*, ignoring the QML code above and thus there's no 'progress'
        // property on TestType.object
        console.log(TestType.object.progress);
    }
}
