import qt.test 1.0
TwoRequiredProperties {
    id: test
    index: 1 // this property is fine
    property string name: "overwritten and not required"
}
