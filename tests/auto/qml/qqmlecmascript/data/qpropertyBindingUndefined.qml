import Qt.test 1.0
ClassWithQProperty {
    property int anotherValue: 1
    property bool toggle: false
    value: toggle ? undefined : anotherValue
}
