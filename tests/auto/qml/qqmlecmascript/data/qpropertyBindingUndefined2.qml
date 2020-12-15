import Qt.test 1.0
ClassWithQObjectProperty {
    property int anotherValue: 1
    property bool toggle: false
    value: toggle ? undefined : anotherValue
}
