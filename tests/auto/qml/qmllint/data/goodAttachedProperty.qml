import AttachedProperties 1.0
QtObject {
    TestType.object: Component {}
    TestType.count: 42 + 2
    Component.onCompleted: {
        TestType.count = 42;
        // TestType here is an attached type, not this object's instance of it.
        // This attached type has an unchanged property object of type QtObject,
        // so accessing Component's specific property 'progress' is only
        // possible with a cast to Component type
        console.log((TestType.object as Component).progress);
    }
}
