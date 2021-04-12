import AttachedProperties 1.0
QtObject {
    TestType.object: Component {}

    Component.onCompleted: {
        console.log(TestType.object.progress);
    }

    property QtObject child: QtObject {
        Component.onCompleted: {
            // doesn't work: this type's TestType.object is not set
            console.log(TestType.object.progress);
        }
    }
}
