import AttachedProperties 1.0
QtObject {
    // count has type int, so assigning QtObject to it is wrong
    TestType.count: QtObject {}
}
