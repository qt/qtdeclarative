import Qt.test 1.0
ClassWithQProperty {
    property int externalValue
    value: {
        return externalValue
    }
    property int changeHandlerCount: 0
    onValueChanged: {
        changeHandlerCount++;
    }
}
